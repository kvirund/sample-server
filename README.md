# Overview

This repository contains a couple of simple socket-based echo server implementations.

* `multi-accept.cpp`: implementation with thread pool where each thread accepts client connections on its own;
* `multi-accept-std-thread.cpp`: similar to the `multi-accept.cpp` but the threads are created using standard `std::thread`;
* `queue.cpp`: incoming connections are getting added to a blocking queue and the threads are taking them from there.

# Building

```
$ mkdir build
$ cd build
$ cmake ..
$ cmake --build .
```

# Running

```
./multi-accept
```

# Testing

```plain
$ for i in $(seq 1 20); do (echo -n "test" | nc localhost 8080 -q 5 &); done;
Reply from the worker #0: test
Reply from the worker #1: test
Reply from the worker #2: test
Reply from the worker #3: test
Reply from the worker #4: test
Reply from the worker #0: test
Reply from the worker #1: test
Reply from the worker #2: test
Reply from the worker #3: test
Reply from the worker #4: test
Reply from the worker #0: test
Reply from the worker #1: test
Reply from the worker #2: test
Reply from the worker #3: test
Reply from the worker #4: test
Reply from the worker #0: test
Reply from the worker #1: test
Reply from the worker #2: test
$ Reply from the worker #3: test
Reply from the worker #4: test

$
```

On the server:

```plain
$ ./multi-accept
The server has started...
Worker #0 has started.
Worker #1 has started.
Worker #2 has started.
Worker #3 has started.
Worker #4 has started.
Worker #0 accepted connection
Received data on the worker #0: test
The client has closed connection on the worker #0
Worker #1 accepted connection
Received data on the worker #1: test
The client has closed connection on the worker #1
Worker #2 accepted connection
Received data on the worker #2: test
The client has closed connection on the worker #2
Worker #3 accepted connection
Received data on the worker #3: test
The client has closed connection on the worker #3
Worker #4 accepted connection
Received data on the worker #4: test
The client has closed connection on the worker #4
Worker #0 accepted connection
Received data on the worker #0: test
The client has closed connection on the worker #0
Worker #1 accepted connection
Received data on the worker #1: test
The client has closed connection on the worker #1
Worker #2 accepted connection
Received data on the worker #2: test
The client has closed connection on the worker #2
Worker #3 accepted connection
Received data on the worker #3: test
The client has closed connection on the worker #3
Worker #4 accepted connection
Received data on the worker #4: test
The client has closed connection on the worker #4
Worker #0 accepted connection
Received data on the worker #0: test
The client has closed connection on the worker #0
Worker #1 accepted connection
Received data on the worker #1: test
The client has closed connection on the worker #1
Worker #2 accepted connection
Received data on the worker #2: test
The client has closed connection on the worker #2
Worker #3 accepted connection
Received data on the worker #3: test
The client has closed connection on the worker #3
Worker #4 accepted connection
Received data on the worker #4: test
The client has closed connection on the worker #4
Worker #0 accepted connection
Received data on the worker #0: test
The client has closed connection on the worker #0
Worker #1 accepted connection
Received data on the worker #1: test
The client has closed connection on the worker #1
Worker #2 accepted connection
Received data on the worker #2: test
The client has closed connection on the worker #2
Worker #3 accepted connection
Received data on the worker #3: test
The client has closed connection on the worker #3
Worker #4 accepted connection
Received data on the worker #4: test
The client has closed connection on the worker #4
```