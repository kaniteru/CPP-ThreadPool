# CPP-ThreadPool
A simple thread pool written in C++ standard 11

## Features
- Simple Usage
- Single Header
- Cross Platform

## Usage
### Constructor
```cpp
#include <kani/thread_pool.hpp>
using namespace kani;

// Uses the maximum available worker threads.
ThreadPool();
// Specifies the number of worker threads to use.
ThreadPool(size_t lenThreads);
// Number of worker threads fixed at 1, and no other tasks will proceed until the current task is completed.
OrderedThreadPool();
```

### Example
```cpp
ThreadPool tp;

if (!tp.start()) { // start workers
    return;
} 

tp.enqueue([] { // Enqueue a task
    // your task
});

tp.stop(); // stop workers
```

## Todo
- support c++ 98