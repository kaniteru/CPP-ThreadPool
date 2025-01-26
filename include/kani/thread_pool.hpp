#ifndef KANITERU_THREAD_POOL_HPP
#define KANITERU_THREAD_POOL_HPP
#include <vector>
#include <queue>
#include <atomic>
#include <thread>
#include <functional>
#include <condition_variable>

/* thread_pool.hpp
 *  Included classes:
 *      - kani::ThreadPool
 *      - kani::OrderedThreadPool
 */

namespace kani {

// ======================== C L A S S ========================
// ===    kani::ThreadPool
// ======================== C L A S S ========================

class ThreadPool {
public:
    using worker_task_t = std::function<void()>;

    /**
     * @return Returns true if worker running.
     */
    bool is_running() const;

    /**
     * @return Returns true if stopped working.
     */
    bool is_stopped() const;

    /**
     * @brief Enqueue a task into worker.
     *
     * @param [in] task A task.
     *
     * @code
     * ThreadPool tp(...);
     * tp.enqueue([]() {
     *     uint32_t i = 0;
     *
     *     while (i < 99) {
     *         std::cout << "your main thread never stops! yay!! << std::endl;
     *         i++;
     *     }
     * });
     * @endcode
     */
    void enqueue(worker_task_t&& task);

    /**
     * @brief Enqueue a task into worker.
     *
     * @param [in] task A task.
     *
     * @code
     * ThreadPool tp(...);
     *
     * ThreadPool::worker_task_t task = []() {
     *     uint32_t i = 0;
     *
     *     while (i < 99) {
     *         std::cout << "your main thread never stops! yay!! << std::endl;
     *         i++;
     *     }
     * };
     *
     * tp.enqueue(task);
     * @endcode
     */
    void enqueue(const worker_task_t& task);

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
     * <br> To check if the workers have completely stopped, use ThreadPool::is_stopped().
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
     * You can check the maximum available worker threads with ThreadPool::MAX_WORKER_THREADS.
     */
    explicit ThreadPool(size_t lenWorkers = ThreadPool::MAX_WORKER_THREADS);
    ~ThreadPool();
protected:
    const size_t m_lenWorkers;
    const bool m_orderedTask;
    std::atomic_bool m_running;
    std::atomic_bool m_requestedStop;
    std::vector<std::thread> m_workers;
    std::queue<worker_task_t> m_tasks;
    std::condition_variable m_cv;
    mutable std::mutex m_mtx;
public:
    static const size_t MAX_WORKER_THREADS; /* The maximum number of workers that can be created. */
};

// ======================== C L A S S ========================
// ===    kani::ThreadPool
// ======================== C L A S S ========================

inline
bool ThreadPool::is_running() const {
    return m_running;
}

inline
bool ThreadPool::is_stopped() const {
    return !m_running && m_requestedStop;
}

inline
void ThreadPool::enqueue(worker_task_t&& task) {
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        m_tasks.push(std::move(task));
    }

    m_cv.notify_one();
}

inline
void ThreadPool::enqueue(const worker_task_t& task) {
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        m_tasks.push(task);
    }

    m_cv.notify_one();
}

inline
void ThreadPool::clear() {
    std::unique_lock<std::mutex> lock(m_mtx);
    m_tasks = std::queue<worker_task_t>();
}

inline
bool ThreadPool::start() {
    if (m_running) {
        return false;
    }

    std::unique_lock<std::mutex> lock(m_mtx);

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
        std::unique_lock<std::mutex> lock(m_mtx);
        m_cv.notify_all();
    }

    for (auto& it : m_workers) {
        while (it.joinable()) {
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

#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
inline
#endif //>=201703L
const size_t ThreadPool::MAX_WORKER_THREADS = std::thread::hardware_concurrency();

// ======================== C L A S S ========================
// ===    kani::OrderedThreadPool
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
protected:
    mutable std::mutex m_orderedMtx;
};

// ======================== C L A S S ========================
// ===    kani::OrderedThreadPool
// ======================== C L A S S ========================

inline
bool OrderedThreadPool::start() {
    if (m_running) {
        return false;
    }

    std::unique_lock<std::mutex> lock(m_mtx);

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

        std::unique_lock<std::mutex> lock(m_orderedMtx);
        task();
    }
}

inline
OrderedThreadPool::OrderedThreadPool() :
    ThreadPool(1) { }
} //namespace kani


#endif //KANITERU_THREAD_POOL_HPP
