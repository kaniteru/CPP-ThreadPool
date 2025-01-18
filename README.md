# CPP-ThreadPool
A simple thread pool written in the C++11 standard

## Features
- Simple Usage
- Single Header
- Cross Platform

## Usage
### Constructor
```cpp
#include <kani/thread_pool.hpp>
using namespace kani;

struct ThreadPool::Config {
    // The number of workers(threads) to use.
    // You can check the maximum number of workers that can be used with 'ThreadPool::MAX_WORKER_THREADS'.
    size_t thread_count;

    // If true, m_numThreads is fixed to 1, and no other tasks will be executed until the current task is completed.
    bool   ordered_task;
};

// Uses the maximum available workers.
ThreadPool();
// Initializes the thread pool according to the provided config.
ThreadPool(const ThreadPool::Config&);
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
- support c++98