#pragma once

#include <string>
#include <sstream>
#include <memory>
#include <future>
#include <atomic>
#include <queue>
#include <mutex>
#include <functional>
#include <vector>
#include <condition_variable>

namespace mcts {

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
            const std::unique_lock<std::mutex> lock{ mutex_, std::try_to_lock };
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
        std::unique_lock<std::mutex> guard{ mutex_ };
        queue_.emplace_back(std::forward<U>(task));
        notify_.notify_one();
    }

    bool TryDequeue(Type& task) {
        const std::unique_lock<std::mutex> lock{ mutex_, std::try_to_lock };

        if (!lock || queue_.empty()) {
            return false;
        }

        task = std::move(queue_.front());
        queue_.pop_front();
        return true;
    }

    bool Dequeue(Type& task) {
        std::unique_lock<std::mutex> guard{ mutex_ };

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
        std::unique_lock<std::mutex> guard{ mutex_ };

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
        std::unique_lock<std::mutex> guard{ mutex_ };
        done_ = true;
        notify_.notify_all();
    }

private:
    std::atomic<bool> done_;
    mutable std::mutex mutex_;
    std::condition_variable notify_;
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
    static const int MAX_THREAD_HT_PAD = 64 * 1024;

    void AddThread(size_t i) {
        threads_.emplace_back([i, this]() mutable {
#ifdef _WIN32
            // https://software.intel.com/en-us/articles/multithreaded-game-programming-and-hyper-threading-technology
            const auto cacheline_pad = _alloca(((i + 1) % max_thread_) * MAX_THREAD_HT_PAD);
#endif
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
    std::future<typename std::result_of<F(Args ...)>::type> RunAsync(F&& f, Args&& ... args);

private:
    explicit ThreadPool(size_t max_thread = std::thread::hardware_concurrency())
        : scheduler_(max_thread) {
    }

    TaskScheduler<Task> scheduler_;
};

template <class F, class ... Args>
std::future<typename std::result_of<F(Args ...)>::type> ThreadPool::RunAsync(F&& f, Args&& ... args) {
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
std::vector<std::future<void>> ParallelFor(IndexType first, IndexType last, IndexType step, Function fun) {
    std::vector<std::future<void>> futs;
    for (auto i = first; i < last; i += step) {
        futs.push_back(ThreadPool::Get().RunAsync([i, fun]() {
            fun(i);
        }));
    }
    return futs;
}

template <typename IndexType, typename Function>
void ParallelFor(IndexType count, Function fun) {
    auto tasks = ParallelFor(IndexType(0), count, IndexType(1), [fun](IndexType i) {
        fun(i);
    });

	for (auto& task : tasks) {
		task.get();
	}
}

}
