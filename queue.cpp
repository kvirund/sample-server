#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <iostream>
#include <utility>
#include <vector>
#include <memory>
#include <sstream>
#include <mutex>
#include <list>
#include <condition_variable>

const int PORT = 8080;
const std::size_t NUMBER_OF_THREADS = 5;
const std::size_t READ_BUFFER_SIZE = 1024;
const std::size_t MAX_QUEUE_SIZE = 5;

template<typename T>
class BlockingQueue {
private:
    using item_t = T;
    std::list<item_t> storage;
    std::mutex mutex;
    std::condition_variable push_signal;
    std::condition_variable pop_signal;

public:
    BlockingQueue() = default;

    void push(const item_t &item) {
        std::unique_lock<std::mutex> lock(mutex);

#pragma clang diagnostic push
#pragma ide diagnostic ignored "LoopDoesntUseConditionVariableInspection"
        while (MAX_QUEUE_SIZE == storage.size()) {
            pop_signal.wait(lock);
        }
#pragma clang diagnostic pop

        storage.push_front(item);
        push_signal.notify_one();
    }

    item_t pop() {
        std::unique_lock<std::mutex> lock(mutex);

        while (storage.empty()) {
            push_signal.wait(lock);
        }

        const auto result = storage.back();
        storage.pop_back();

        pop_signal.notify_one();

        return result;
    }
};

using queue_t = BlockingQueue<int>;

template<typename F>
void lock(F f) {
    static std::mutex call_lock;

    std::lock_guard<std::mutex> guard(call_lock);
    f();
}

int create_server() {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address{0};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 12) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    return server_socket;
}

class ThreadParameters {
public:
    using shared_ptr = std::shared_ptr<ThreadParameters>;

    queue_t &queue;
    const std::size_t thread_number;

    ThreadParameters(queue_t &queue, std::size_t thread_number) : queue(queue), thread_number(thread_number) {}
};

class ThreadDescriptor {
public:
    const pthread_t &thread_id;
    const ThreadParameters::shared_ptr parameters;

    ThreadDescriptor(const pthread_t &thread_id, ThreadParameters::shared_ptr parameters) : thread_id(thread_id),
                                                                                            parameters(std::move(
                                                                                                    parameters)) {}
};

void serve(const int server_socket, queue_t &queue);

void handle_connection(const ThreadParameters &parameters, const int client) {
    char buffer[READ_BUFFER_SIZE];
    bool done = false;
    while (!done) {
        const ssize_t size = recv(client, buffer, READ_BUFFER_SIZE - 1, 0);
        if (0 < size) {
            buffer[size] = '\0';
            lock([&] {
                std::cout << "Received data on the worker #" << parameters.thread_number << ": " << buffer << std::endl;
            });
            std::stringstream ss;
            ss << "Reply from the worker #" << parameters.thread_number << ": " << buffer << std::endl;
            const auto &message = ss.str();
            const auto send_result = send(client, message.c_str(), message.size(), 0);
            if (-1 == send_result) {
                perror("send");
            }
        } else {
            if (0 == size) {
                lock([&] {
                    std::cout << "The client has closed connection on the worker #" << parameters.thread_number
                              << std::endl;
                });
            } else {
                perror("recv");
                lock([&] {
                    std::cout << "Error happened on the worker #" << parameters.thread_number << ", closing the socket"
                              << std::endl;
                });
            }
            close(client);
            done = true;
        }
    }
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantFunctionResult"

void *worker(void *parameters_ptr) {
    const ThreadParameters &parameters = *(ThreadParameters *) parameters_ptr;
    lock([&] { std::cout << "Worker #" << parameters.thread_number << " has started." << std::endl; });

    auto &queue = parameters.queue;
    while (const auto client = queue.pop()) {
        lock([&] {
            std::cout << "Worker #" << parameters.thread_number << " started handling incoming connection" << std::endl;
        });
        handle_connection(parameters, client);
    }

    return nullptr;
}

#pragma clang diagnostic pop

void join_threads(const std::vector<ThreadDescriptor> &threads) {
    for (const auto &thread: threads) {
        void *result = nullptr;
        int join_result = pthread_join(thread.thread_id, &result);
        if (0 != join_result) {
            perror("pthread_join");
            exit(EXIT_FAILURE);
        }
    }
}

void start_threads(queue_t &queue, std::vector<ThreadDescriptor> &threads, std::size_t number_of_threads) {
    threads.reserve(number_of_threads);
    for (std::size_t thread_number = 0; thread_number != number_of_threads; ++thread_number) {
        const ThreadParameters::shared_ptr parameters = std::make_shared<ThreadParameters>(queue, thread_number);

        pthread_t thread_id = 0;
        const auto result = pthread_create(&thread_id, nullptr, worker, parameters.get());
        if (0 != result) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }

        threads.emplace_back(thread_id, parameters);
    }
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char *args[]) {
    std::cout << "The server has started..." << std::endl;

    const int server_socket = create_server();

    queue_t queue;
    std::vector<ThreadDescriptor> threads;
    start_threads(queue, threads, NUMBER_OF_THREADS);
    serve(server_socket, queue);
    join_threads(threads);

    return 0;
}

void serve(const int server_socket, queue_t &queue) {
    struct sockaddr_in client_address{0};
    socklen_t address_length = 0;
    while (const auto client = accept(server_socket, (struct sockaddr *) &client_address, &address_length)) {
        if (0 > client) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        lock([&] { std::cout << "Accepted connection" << std::endl; });

        queue.push(client);
    }
}
