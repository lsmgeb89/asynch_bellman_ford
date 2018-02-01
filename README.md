# Asynch [Bellman–Ford][bf]

## Summary
  * Utilized multi-threading and concurrent primitives ([threads][th], [mutexes][mu] & [condition variables][cv] in C++11 [Thread Support Library][tsl]) to develop a simple simulator which simulates an asynchronous distributed system composed of one master thread and `n` slave threads.
  * Implemented coordinated behaviors between the master thread and `n` slave threads by using [monitors][mo] consisting of [mutexes][mu] and [condition variables][cv]
  * Implemented [message passing][mp] in bidirected [FIFO][fifo] links between two slave threads by using [mutexes][mu] and [queues][qu]
  * Implemented Asynch [Bellman–Ford][bf] for [shortest paths][sp] from a single source node <code>i<sub>0</sub></code> to all of the other nodes in a weighted undirected graph under the framework of this simulator.

## Project Information
  * Course: [Distributed Computing (CS 6380)][dc]
  * Professor: [Subbarayan Venkatesan][venky]
  * Semester: Fall 2016
  * Programming Language: C++
  * Build Tool: [CMake][cmake]

## Full Requirements

### Part One: Simulator
  * You will develop a simple simulator that simulates an asynchronous distributed system using multithreading.
  * There are `n + 1` processes in this asynchronous distributed system: one master process and `n` slave processes. Each process will be simulated by one thread.
  * The master thread will "inform" all slave threads as when one round starts.
  * You should view duration of one round as one "time unit" ticks in this asynchronous distributed system.
  * Each slave threads must wait for the master thread for a "go ahead" signal before it can begin round `r`.
  * The master thread can give the signal to start round `r` only if it is sure that all the `n` slave threads have completed their previous round `r - 1`.
  * The message transimission time for each link for each message is to be randomly chosen using a uniform distribution in the range `1` to `15` "time units".
  * All links are bidirectional and [FIFO][fifo]. ([FIFO][fifo]: If I send two messages <code>m<sub>1</sub></code> and then <code>m<sub>2</sub></code> to you, then you receive <code>m<sub>1</sub></code> first and then <code>m<sub>2</sub></code>.)

### Part Two: [Bellman–Ford][bf]
  * You will implement the asynchronous [Bellman–Ford][bf] algorithm for shortest paths.
  * The nodes (processes) and links operate in such a way that message processing time at a node is in one system "time unit" viewed as instantaneous.
  * As soon as a message is received, it is processed by that node.
  * Your program will read in the network infomation by using [adjacency-like matrix][am] from an input file called connectivity.txt:
    1. The first line is `n,x`: `n` is number of nodes and they are labeled as `1..n` and `x (<= n)` is the id of the root aka (<code>i<sub>0</sub></code>) of the shortest paths tree.
    2. The remaining is [adjacency-like matrix][am] denoted as `M` indicating connectivity infomation.
    3. Specifically, `M[i][j]` shows the weight of link between node `i` and `j`. A weight of `-1` signifies no link.
  * The master thread reads connectivity.txt and then spawns `n` threads.
  * No process knows the value of `n`. No slave threads knows who is <code>i<sub>0</sub></code>.
  * All processes must terminate properly after shortest path is found.

[bf]: https://en.wikipedia.org/wiki/Bellman%E2%80%93Ford_algorithm
[th]: http://en.cppreference.com/w/cpp/thread/thread
[mu]: http://en.cppreference.com/w/cpp/thread/mutex
[cv]: http://en.cppreference.com/w/cpp/thread/condition_variable
[tsl]: http://en.cppreference.com/w/cpp/thread
[mo]: https://en.wikipedia.org/wiki/Monitor_(synchronization)
[mp]: https://en.wikipedia.org/wiki/Message_passing
[fifo]: https://en.wikipedia.org/wiki/FIFO_(computing_and_electronics)
[qu]: http://en.cppreference.com/w/cpp/container/queue
[sp]: https://en.wikipedia.org/wiki/Shortest_path
[dc]: https://catalog.utdallas.edu/2016/graduate/courses/cs6380
[venky]: http://cs.utdallas.edu/people/faculty/venkatesan-s/
[cmake]: https://cmake.org/
[am]: https://en.wikipedia.org/wiki/Adjacency_matrix
