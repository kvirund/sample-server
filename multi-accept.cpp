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

const int PORT = 8080;
const std::size_t NUMBER_OF_THREADS = 10;
const std::size_t READ_BUFFER_SIZE = 1024;

template <typename F>
void lock(F f) {
    static std::mutex call_lock;

    call_lock.lock();
    f();
    call_lock.unlock();
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

    const int server_socket;
    const std::size_t thread_number;

    ThreadParameters(int server_socket, std::size_t thread_number) : server_socket(server_socket),
                                                                     thread_number(thread_number) {}
};

class ThreadDescriptor {
public:
    const pthread_t &thread_id;
    const ThreadParameters::shared_ptr parameters;

    ThreadDescriptor(const pthread_t &thread_id, ThreadParameters::shared_ptr parameters) : thread_id(thread_id),
                                                                                            parameters(std::move(
                                                                                                    parameters)) {}
};

void handle_connection(const ThreadParameters &parameters, const int client);

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantFunctionResult"
void *worker(void *parameters_ptr) {
    const ThreadParameters &parameters = *(ThreadParameters *) parameters_ptr;
    lock([&] { std::cout << "Worker #" << parameters.thread_number << " has started." << std::endl; });

    const int &server_socket = parameters.server_socket;
    struct sockaddr_in client_address{0};
    socklen_t address_length = 0;
    while (const auto client = accept(server_socket, (struct sockaddr *) &client_address, &address_length)) {
        if (0 > client) {
            perror("accept");
            return nullptr;
        }

        lock ([&] { std::cout << "Worker #" << parameters.thread_number << " accepted connection" << std::endl; });

        handle_connection(parameters, client);
    }

    return nullptr;
}
#pragma clang diagnostic pop

void handle_connection(const ThreadParameters &parameters, const int client) {
    char buffer[READ_BUFFER_SIZE];
    bool done = false;
    while (!done) {
        const ssize_t size = recv(client, buffer, READ_BUFFER_SIZE - 1, 0);
        if (0 < size) {
            buffer[size] = '\0';
            lock([&] { std::cout << "Received data on the worker #" << parameters.thread_number << ": " << buffer << std::endl; });
            std::stringstream ss;
            ss << "Reply from the worker #" << parameters.thread_number << ": " << buffer << std::endl;
            const auto& message = ss.str();
            const auto send_result = send(client, message.c_str(), message.size(), 0);
            if (-1 == send_result) {
                perror("send");
            }
        } else {
            if (0 == size) {
                lock([&] { std::cout << "The client has closed connection on the worker #" << parameters.thread_number << std::endl; });
            } else {
                perror("recv");
                lock([&] { std::cout << "Error happened on the worker #" << parameters.thread_number << ", closing the socket" << std::endl; });
            }
            close(client);
            done = true;
        }
    }
}

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

void start_threads(const int &server_socket, std::vector<ThreadDescriptor> &threads, std::size_t number_of_threads) {
    threads.reserve(number_of_threads);
    for (std::size_t thread_number = 0; thread_number != number_of_threads; ++thread_number) {
        const ThreadParameters::shared_ptr parameters = std::make_shared<ThreadParameters>(server_socket,
                                                                                           thread_number);

        pthread_t thread_id = 0;
        const auto result = pthread_create(&thread_id, nullptr, worker, parameters.get());
        if (0 != result) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }

        threads.emplace_back(thread_id, parameters);
    }
}

int main(int argc, char *args[]) {
    std::cout << "The server has started..." << std::endl;

    const int server_socket = create_server();

    std::vector<ThreadDescriptor> threads;
    start_threads(server_socket, threads, NUMBER_OF_THREADS);
    join_threads(threads);

    return 0;
}
