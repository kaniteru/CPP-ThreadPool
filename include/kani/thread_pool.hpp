// thread_pool.hpp
//
// This file is part of the CPP-ThreadPool project.
//
// Portions of the code in this file were derived or inspired by the following open-source projects:
// 1. ThreadPool by Henning Peters, Viktor Peppiatt (Original Repository: https://github.com/progschj/ThreadPool)
// 2. ThreadPool by Jan Niklas Hasse (Fork Repository: https://github.com/jhasse/ThreadPool)
//
// Both projects are licensed under the zlib License.
// The license text is included in this project's LICENSE file, as required by its terms.
//
// Specifically, the implementation of the `kani::ThreadPool::enqueue` method has been derived or inspired by the two projects,
// with improvements and modifications suited to the needs of this project.
//
// For details of these changes, refer to the commit history of this project at:
// https://github.com/kaniteru/CPP-ThreadPool
#ifndef KANITERU_THREAD_POOL_HPP
#define KANITERU_THREAD_POOL_HPP
#include <queue>
#include <vector>
#include <atomic>
#include <thread>
#include <future>
#include <memory>
#include <functional>
#include <type_traits>
#include <condition_variable>

/* thread_pool.hpp
 *  Included classes:
 *      - kani::ThreadPool
 *      - kani::OrderedThreadPool
 */

#ifdef _MSVC_LANG
    /* C++ Standard version in msvc. */
    #define KANI_CXX_VER _MSVC_LANG
#else
    /* C++ Standard version. */
    #define KANI_CXX_VER __cplusplus
#endif //_MSVC_LANG
#define KANI_CXX14 201402L
#define KANI_CXX17 201703L
#define KANI_CXX20 202002L
#if KANI_CXX_VER >= KANI_CXX17
    #define KANI_INVOKE_RESULT(R, A) std::invoke_result<R, A...>
#else
    #define KANI_INVOKE_RESULT(R, A) std::result_of<R(A...)>
#endif //KANI_CXX_VER >= KANI_CXX17

namespace kani {

// ======================== C L A S S ========================
// ===    ThreadPool
// ======================== C L A S S ========================

class ThreadPool {
public:
    using worker_task_t = std::packaged_task<void()>;

    /**
     * @return Returns true if worker running.
     */
    bool is_running() const;

    /**
     * @return Returns true if worker has no tasks.
     */
    bool is_empty() const;

    /**
     * @brief Adds a task to the worker for execution.
     * @note Tasks are processed in the order they were enqueued (FIFO).
     *
     * @tparam T Type of the callable object.
     * @tparam Args Types of the arguments for the callable.
     * @param t The callable object (e.g., lambda, function pointer, etc.).
     * @param args Arguments to pass to the callable object.
     *
     * @return A std::future object that can be used to retrieve the result of the task.
     *
     * @code
     * Threadpool tp(...);
     *
     * auto res = tp.enqueue([](int a1, int a2) {
     *      return a1 + a2;
     * }, 1, 2);
     *
     * int add = res.get(); // add == 3
     * @endcode
     */
    template <typename T, typename... Args>
#if KANI_CXX_VER >= KANI_CXX14
    decltype(auto) enqueue(T&&t, Args&&... args);
#else
    auto enqueue(T&& t, Args&&... args) -> std::future<typename KANI_INVOKE_RESULT(T, Args)::type>;
#endif //KANI_CXX_VER >= KANI_CXX14

    /**
     * @brief Clear enqueued tasks in thread pool.
     */
    void clear();

    /**
     * @return Returns true if worker started successfully.
     */
    bool start();

    /**
     * @brief Stop the workers.
     * @note To check if the workers have completely stopped, use ThreadPool::is_running().
     * <br>Use ThreadPool::clear() to remove queued tasks that haven't yet been processed.
     *
     * @return Returns true if workers stopped successfully.
     */
    bool stop();
protected:
    /**
     * @brief A worker thread.
     */
    void worker_thread();

public:
    /**
     * @param [in] lenWorkers Specifies the number of worker threads to use.
     */
    explicit ThreadPool(size_t lenWorkers = std::thread::hardware_concurrency());
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
protected:
    const size_t m_lenWorkers;
    const bool m_orderedTask;
    std::atomic_bool m_running;
    std::atomic_bool m_requestedStop;
    std::vector<std::thread> m_workers;
    std::queue<worker_task_t> m_tasks;
    std::condition_variable m_cv;
    mutable std::mutex m_mtx;
};

// ======================== C L A S S ========================
// ===    ThreadPool
// ======================== C L A S S ========================

inline
bool ThreadPool::is_running() const {
    return m_running;
}

inline
bool ThreadPool::is_empty() const {
    std::lock_guard<std::mutex> lock(m_mtx);
    return m_tasks.empty();
}

template <typename T, typename... Args>
#if KANI_CXX_VER >= KANI_CXX14
decltype(auto) ThreadPool::enqueue(T&&t, Args&&... args) {
#else
auto ThreadPool::enqueue(T&& t, Args&&... args) -> std::future<typename KANI_INVOKE_RESULT(T, Args)::type> {
#endif //KANI_CXX_VER >= KANI_CXX14
    using ret_type = typename KANI_INVOKE_RESULT(T, Args)::type;

    std::packaged_task<ret_type()> task(
        std::bind(std::forward<T>(t), std::forward<Args>(args)...)
    );

    std::future<ret_type> res = task.get_future();

    {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_tasks.emplace(std::move(task));
    }

    m_cv.notify_one();
    return res;
}

inline
void ThreadPool::clear() {
    std::lock_guard<std::mutex> lock(m_mtx);
    m_tasks = std::queue<worker_task_t>();
}

inline
bool ThreadPool::start() {
    if (m_running) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mtx);

    m_running = true;
    m_requestedStop = false;

    for (size_t i = 0; i < m_lenWorkers; i++) {
        std::thread thread(&ThreadPool::worker_thread, this);
        m_workers.emplace_back(std::move(thread));
    }

    return true;
}

inline
bool ThreadPool::stop() {
    if (!m_running || m_requestedStop) {
        return false;
    }

    m_requestedStop = true;

    {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_cv.notify_all();
    }

    for (auto& it : m_workers) {
        if (it.joinable()) {
            it.join();
        }
    }

    m_workers.clear();
    m_running = false;
    return true;
}

inline
void ThreadPool::worker_thread() {
    while (true) {
        worker_task_t task { };

        {
            std::unique_lock<std::mutex> lock(m_mtx);
            m_cv.wait(lock, [&] {
                return !m_tasks.empty() || m_requestedStop;
            });

            if (m_requestedStop) {
                return;
            }

            task = std::move(m_tasks.front());
            m_tasks.pop();
        }

        task();
    }
}

inline
ThreadPool::ThreadPool(const size_t lenWorkers) :
    m_lenWorkers(lenWorkers),
    m_orderedTask(false),
    m_running(false),
    m_requestedStop(true) { }

inline
ThreadPool::~ThreadPool() {
    this->stop();
}

// ======================== C L A S S ========================
// ===    OrderedThreadPool
// ======================== C L A S S ========================

class OrderedThreadPool : public ThreadPool {
public:
    /**
     * @return Returns true if worker started successfully.
     */
    bool start();
protected:
    /**
     * @brief A worker thread.
     */
    void worker_thread();

public:
    /**
     * @brief The number of worker threads is fixed at 1, and no other tasks will proceed until the current task is completed.
     */
    OrderedThreadPool();
    ~OrderedThreadPool() = default;

    OrderedThreadPool(const OrderedThreadPool&) = delete;
    OrderedThreadPool& operator=(const OrderedThreadPool&) = delete;
};

// ======================== C L A S S ========================
// ===    OrderedThreadPool
// ======================== C L A S S ========================

inline
bool OrderedThreadPool::start() {
    if (m_running) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mtx);

    m_running = true;
    m_requestedStop = false;

    std::thread thread(&OrderedThreadPool::worker_thread, this);
    m_workers.emplace_back(std::move(thread));
    return true;
}

inline
void OrderedThreadPool::worker_thread() {
    while (true) {
        worker_task_t task { };
        std::unique_lock<std::mutex> lock(m_mtx);

        m_cv.wait(lock, [&] {
            return !m_tasks.empty() || m_requestedStop;
        });

        if (m_requestedStop) {
            return;
        }

        task = std::move(m_tasks.front());
        m_tasks.pop();
        lock.unlock();

        task();
    }
}

inline
OrderedThreadPool::OrderedThreadPool() :
    ThreadPool(1) { }
} //namespace kani


#endif //KANITERU_THREAD_POOL_HPP
