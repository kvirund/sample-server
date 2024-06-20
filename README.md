This repository contains a couple of simple socket-based echo server implementations.

* `multi-accept.cpp`: implementation with thread pool where each thread accepts client connections on its own;
* `multi-accept-std-thread.cpp`: similar to the `multi-sccept.cpp` but the threads are created using standard `std::thread`;
* `queue.cpp`: incoming connections are getting added to a blocking queue and the threads are taking them from there.
