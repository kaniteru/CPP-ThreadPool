#ifndef KANITERU_THREAD_POOL_HPP
#define KANITERU_THREAD_POOL_HPP
#include <vector>
#include <queue>
#include <atomic>
#include <thread>
#include <functional>
#include <condition_variable>

/* thread_pool.hpp
 * Included classes:
 *      - kani::ThreadPool
 *
 * Included structs:
 *      - kani::ThreadPool::Config
 */

namespace kani {

// ======================== C L A S S ========================
// ===    kani::ThreadPool
// ======================== C L A S S ========================

class ThreadPool {
public:
    using worker_task_t = std::function<void()>;

    struct Config {
        /* Sets the number of threads for the workers.
             You can check the maximum number of workers that can be used with 'ThreadPool::MAX_WORKER_THREADS'. */
        size_t m_numThreads;
        /* If true, m_numThreads is fixed to 1, and no other tasks will be executed until the current task is completed. */
        bool    m_orderedTask;
    };

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
private:
    /**
     * @brief A worker thread.
     */
    void worker_thread();

public:
    /**
     * @brief Constructor.<br>
     * Uses the maximum available workers.
     */
    ThreadPool();

    /**
     * @brief Constructor.<br>
     * Initializes the thread pool according to the provided config.
     *
     * @param [in] config Config for the thread pool.
     */
    explicit ThreadPool(const ThreadPool::Config& config);
    ~ThreadPool();
private:
    const size_t m_lenThreads;
    const bool m_orderedTask;
    std::atomic_bool m_running;
    std::atomic_bool m_requestedStop;
    std::vector<std::thread> m_workers;
    std::queue<worker_task_t> m_tasks;
    std::condition_variable m_cv;
    mutable std::mutex m_mtx;
    mutable std::mutex m_orderedMtx;
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

    for (size_t i = 0; i < m_lenThreads; i++) {
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

        if (m_orderedTask) {
            std::unique_lock<std::mutex> lock(m_orderedMtx);
            task();
        }
        else {
            task();
        }
    }
}

inline
ThreadPool::ThreadPool() :
    m_lenThreads(ThreadPool::MAX_WORKER_THREADS),
    m_orderedTask(false),
    m_running(false),
    m_requestedStop(true) { }

inline
ThreadPool::ThreadPool(const ThreadPool::Config& config) :
    m_lenThreads(config.m_orderedTask ? 1 : config.m_numThreads),
    m_orderedTask(config.m_orderedTask),
    m_running(false),
    m_requestedStop(true) { }

inline
ThreadPool::~ThreadPool() {
    this->stop();
}

const size_t ThreadPool::MAX_WORKER_THREADS = std::thread::hardware_concurrency();
} //namespace kani


#endif //KANITERU_THREAD_POOL_HPP
