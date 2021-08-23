// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

#include <string>
#include <memory>
#include <future>
#include <atomic>
#include <queue>
#include <mutex>
#include <functional>
#include <vector>
#include <condition_variable>

#ifdef _WIN32
#pragma comment(lib, "Synchronization.lib")
#include <windows.h>
#endif

namespace mcts {

#ifdef _WIN32
class SRWMutex final {
public:
    SRWMutex() = default;

	void lock() noexcept {
        ::AcquireSRWLockExclusive(&lock_);
	}

    void unlock() noexcept {
        ::ReleaseSRWLockExclusive(&lock_);
	}

    [[nodiscard]] bool try_lock() noexcept {
        return ::TryAcquireSRWLockExclusive(&lock_);
	}
private:
    SRWLOCK lock_ = SRWLOCK_INIT;
};

using FastMutex = SRWMutex;

static constexpr uint32_t kUnlocked = 0;
static constexpr uint32_t kLocked = 1;
static constexpr uint32_t kSleeper = 2;

class FutexMutexConditionVariable final {
public:
    FutexMutexConditionVariable() = default;

	void wait(std::unique_lock<FastMutex>& lock) {
        auto old_state = state_.load(std::memory_order_relaxed);
        lock.unlock();
        FutexWait(state_, old_state);
        lock.lock();
	}

    template <typename Predicate>
    void wait(std::unique_lock<FastMutex>& lock, Predicate&& predicate) {
        while (!predicate()) {
            wait(lock);
        }
    }

    template <typename Rep, typename Period>
    std::cv_status wait_for(std::unique_lock<FastMutex>& lock, const std::chrono::duration<Rep, Period>& rel_time) {
        auto old_state = state_.load(std::memory_order_relaxed);
        lock.unlock();
        auto ret = FutexWait(state_, old_state, rel_time) == -1
            ? std::cv_status::timeout : std::cv_status::no_timeout;
        lock.lock();
        return ret;
    }

    void notify_one() noexcept {
        state_.fetch_add(kLocked, std::memory_order_relaxed);
        FutexWakeSingle(state_);
    }

    void notify_all() noexcept {
        state_.fetch_add(kLocked, std::memory_order_relaxed);
        FutexWakeSingle(state_);
    }
private:
    template <typename Rep, typename Period>
    int FutexWait(std::atomic<uint32_t>& to_wait_on, uint32_t expected, std::chrono::duration<Rep, Period> const& duration) {
        using namespace std::chrono;
        timespec ts;
        ts.tv_sec = duration_cast<seconds>(duration).count();
        ts.tv_nsec = duration_cast<nanoseconds>(duration).count() % 1000000000;
        return FutexWait(to_wait_on, expected, &ts);
    }

    int FutexWait(std::atomic<uint32_t>& to_wait_on, uint32_t expected, const struct timespec* to) {
        if (to == nullptr) {
            FutexWait(to_wait_on, expected);
            return 0;
        }

        if (to->tv_nsec >= 1000000000) {
            errno = EINVAL;
            return -1;
        }

        if (to->tv_sec >= 2147) {
            ::WaitOnAddress(&to_wait_on, &expected, sizeof(expected), 2147000000);
            return 0; /* time-out out of range, claim spurious wake-up */
        }

        const DWORD ms = (to->tv_sec * 1000000) + ((to->tv_nsec + 999) / 1000);

        if (!::WaitOnAddress(&to_wait_on, &expected, sizeof(expected), ms)) {
            errno = ETIMEDOUT;
            return -1;
        }
        return 0;
    }

    void FutexWait(std::atomic<uint32_t>& to_wait_on, uint32_t expected) {
        ::WaitOnAddress(&to_wait_on, &expected, sizeof(expected), INFINITE);
    }

    template <typename T>
    void FutexWakeSingle(std::atomic<T>& to_wake) {
        ::WakeByAddressSingle(&to_wake);
    }

    template <typename T>
    void FutexWakeAll(std::atomic<T>& to_wake) {
        ::WakeByAddressAll(&to_wake);
    }

    std::atomic<uint32_t> state_{ kUnlocked };
};

using FastConditionVariable = FutexMutexConditionVariable;
#else
using FastMutex = std::mutex;
using FastConditionVariable = std::condition_variable;
#endif

using Task = std::function<void()>;

template <typename Type>
class TaskQueue {
public:
    TaskQueue()
        : done_(false) {
    }

    template <typename U>
    bool TryEnqueue(U&& task) {
        {
            const std::unique_lock<FastMutex> lock{ mutex_, std::try_to_lock };
            if (!lock) {
                return false;
            }
            queue_.emplace_back(std::forward<U>(task));
        }
        notify_.notify_one();
        return true;
    }

    template <typename U>
    void Enqueue(U&& task) {
        std::unique_lock guard{ mutex_ };
        queue_.emplace_back(std::forward<U>(task));
        notify_.notify_one();
    }

    bool TryDequeue(Type& task) {
        const std::unique_lock lock{ mutex_, std::try_to_lock };

        if (!lock || queue_.empty()) {
            return false;
        }

        task = std::move(queue_.front());
        queue_.pop_front();
        return true;
    }

    bool Dequeue(Type& task) {
        std::unique_lock guard{ mutex_ };

        while (queue_.empty() && !done_) {
            notify_.wait(guard);
        }

        if (queue_.empty()) {
            return false;
        }

        task = std::move(queue_.front());
        queue_.pop_front();
        return true;
    }

    bool Dequeue(Type& task, std::chrono::milliseconds wait_time) {
        std::unique_lock guard{ mutex_ };

        // Note: cv.wait_for() does not deal with spurious wakeups
        while (queue_.empty() && !done_) {
            if (std::cv_status::timeout == notify_.wait_for(guard, wait_time)) {
                return false;
            }
        }

        if (queue_.empty() || done_) {
            return false;
        }

        task = std::move(queue_.front());
        queue_.pop_front();
        return true;
    }

    void Done() {
        std::unique_lock guard{ mutex_ };
        done_ = true;
        notify_.notify_all();
    }

private:
    std::atomic<bool> done_;
    mutable FastMutex mutex_;
    FastConditionVariable notify_;
    std::deque<Type> queue_;
};

template
<
	typename TaskType,
    template <typename> class Queue = TaskQueue
>
class TaskScheduler {
public:
    explicit TaskScheduler(size_t max_thread)
        : is_stopped_(false)
        , index_(0)
        , max_thread_(max_thread) {
        task_queues_.reserve(max_thread);
        threads_.reserve(max_thread);
        for (size_t i = 0; i < max_thread; ++i) {
            task_queues_.push_back(std::unique_ptr<TaskQueue<TaskType>>(new TaskQueue<TaskType>()));
        }
        for (size_t i = 0; i < max_thread; ++i) {
            AddThread(i);
        }
    }

    ~TaskScheduler() {
        Destory();
    }

    void SubmitJob(TaskType&& task) {
        const auto i = index_++;

        for (size_t n = 0; n != threads_.size(); ++n) {
            const auto index = (i + n) % task_queues_.size();
            if (task_queues_[index]->TryEnqueue(task)) {
                return;
            }
        }
        task_queues_[i % task_queues_.size()]->Enqueue(task);
    }

    void Destory() {
        is_stopped_ = true;

        for (size_t i = 0; i < task_queues_.size(); ++i) {
            task_queues_[i]->Done();

            if (threads_[i].joinable()) {
                threads_[i].join();
            }
        }

        task_queues_.clear();
        threads_.clear();
    }

private:
    void AddThread(size_t i) {
        threads_.emplace_back([i, this]() mutable {
            for (;;) {
                TaskType task;

                for (size_t n = 0; n != max_thread_; ++n) {
                    if (is_stopped_) {
                        return;
                    }

                    const auto index = (i + n) % max_thread_;
                    if (task_queues_[index]->TryDequeue(task)) {
                        break;
                    }
                }

                if (!task) {
                    if (task_queues_[i]->Dequeue(task)) {
                        task();
                    }
                    else {
                        std::this_thread::yield();
                    }
                }
                else {
                    task();
                }
            }
        });
    }

    std::atomic<bool> is_stopped_;
    size_t index_;
    size_t max_thread_;
    std::vector<std::thread> threads_;
    std::vector<std::unique_ptr<Queue<TaskType>>> task_queues_;
};

class ThreadPool {
public:
    static ThreadPool& Get() {
        static ThreadPool thread_pool;
        return thread_pool;
    }

    template <class F, class... Args>
    std::future<typename std::result_of<F(Args ...)>::type> Spawn(F&& f, Args&& ... args);

private:
    explicit ThreadPool(size_t max_thread = std::thread::hardware_concurrency() * 2 + 1)
        : scheduler_(max_thread) {
    }

    TaskScheduler<Task> scheduler_;
};

template <class F, class ... Args>
std::future<typename std::result_of<F(Args ...)>::type> ThreadPool::Spawn(F&& f, Args&& ... args) {
    typedef typename std::result_of<F(Args ...)>::type ReturnType;

    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
		std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    scheduler_.SubmitJob([task]() {
        (*task)();
    });
    return task->get_future();
}

template <typename IndexType, typename Function>
std::vector<std::future<void>> ParallelFor(IndexType first, IndexType last, IndexType step, Function &&fun) {
    std::vector<std::future<void>> futs;
    for (auto i = first; i < last; i += step) {
        futs.push_back(ThreadPool::Get().Spawn([i, fun]() {
            fun(i);
        }));
    }
    return futs;
}

template <typename IndexType, typename Function>
void ParallelFor(IndexType count, Function &&fun) {
    auto tasks = ParallelFor(IndexType(0), count, IndexType(1), [fun](IndexType i) {
        fun(i);
    });

	for (auto& task : tasks) {
		task.get();
	}
}

}
