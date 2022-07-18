/*
 * @Author       : mark
 * @Date         : 2020-06-15
 * @copyleft Apache 2.0
 */ 

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>
class ThreadPool {
public:
    explicit ThreadPool(size_t threadCount = 8): pool_(std::make_shared<Pool>()) {
            assert(threadCount > 0);

            // create threadCount threads
            for(size_t i = 0; i < threadCount; i++) {
                std::thread([pool = pool_] {
                    std::unique_lock<std::mutex> locker(pool->mtx);
                    while(true) {
                        if(!pool->tasks.empty()) {
                            // get a tasks from queue
                            auto task = std::move(pool->tasks.front());
                            // remove the task
                            pool->tasks.pop();
                            locker.unlock();
                            task(); // task code
                            locker.lock();
                        } 
                        else if(pool->isClosed) break; // delete thread
                        else pool->cond.wait(locker); // block thread
                    }
                }).detach(); // thread detach
            }
    }

    ThreadPool() = default;

    ThreadPool(ThreadPool&&) = default;
    
    ~ThreadPool() {
        if(static_cast<bool>(pool_)) {
            {
                std::lock_guard<std::mutex> locker(pool_->mtx);
                pool_->isClosed = true;
            }
            pool_->cond.notify_all(); // weak all thread
        }
    }

    template<class F>
    void AddTask(F&& task) {
        {
            std::lock_guard<std::mutex> locker(pool_->mtx);
            pool_->tasks.emplace(std::forward<F>(task)); // add task to queue
        }
        pool_->cond.notify_one(); // weak a thread
    }

private:
    // define pool struct
    struct Pool {
        std::mutex mtx;
        std::condition_variable cond;
        bool isClosed;
        std::queue<std::function<void()>> tasks; // queue(store tasks)
    };
    std::shared_ptr<Pool> pool_;
};


#endif //THREADPOOL_H