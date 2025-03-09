# CPP-ThreadPool
A lightweight thread pool written in C++11, designed for simplicity and cross-platform usage.

---

## Features
- Easy to use
- Single Header
- Cross Platform

---

## Usage
```cpp
#include <kani/thread_pool.hpp>

using namespace kani;
```
### Constructor
```cpp
// Uses the maximum available worker threads.
ThreadPool();
// Specifies the number of worker threads to use.
ThreadPool(size_t lenThreads);

// Single-threaded thread pool; tasks run one at a time.
OrderedThreadPool();
```

### Example
```cpp
ThreadPool tp;

if (!tp.start()) { // start workers
    return;
} 
```
```cpp
tp.enqueue([]() { // enqueue a task
    // your task
});
```
```cpp
auto res = tp.enqueue([](int32_t a1, int32_t a2) { // enqueue a task and
    return a1 + a2;                                // capture the result as std::future
}, 1, 2);

std::cout << "Task result: " << res.get(); // output: "Task result: 3"
```
```cpp
tp.stop(); // stop workers
```

---

## Installation

### Option 1: CMake (Recommended)
1. Add the following to your `CMakeLists.txt` to fetch and include the library:
   ```cmake
   include(FetchContent)
   FetchContent_Declare(
       ThreadPool
       GIT_REPOSITORY https://github.com/kaniteru/CPP-ThreadPool
       GIT_TAG main # You can replace 'main' with a specific tag or branch if needed.
   )
   FetchContent_MakeAvailable(ThreadPool)
   ```
2. Link the library to your target:
   ```cmake
   target_link_libraries(MyProject PRIVATE ThreadPool)
   ```

### Option 2: Manual Installation
1. Clone the repository or download the source files:
   ```bash
   git clone https://github.com/kaniteru/CPP-ThreadPool
   ```
2. Add the `include` directory to your project manually or using CMake:
   ```cmake
   target_include_directories(MyExecutable PRIVATE path/to/clone/include)
   ```

---

## Todo
- support c++ 98

---

## License
This project is licensed under the MIT License. Portions of the code were derived or inspired by
projects licensed under the zlib License. For more details, refer to the [LICENSE](./LICENSE) file.