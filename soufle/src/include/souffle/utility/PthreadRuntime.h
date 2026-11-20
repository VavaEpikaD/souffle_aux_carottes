/*
 * POSIX-thread execution support for generated relation scans.
 * Licensed under the Universal Permissive License v1.0 (see licenses/SOUFFLE-UPL.txt).
 */
#pragma once

#include <pthread.h>
#include <unistd.h>
#include <algorithm>
#include <atomic>
#include <exception>
#include <functional>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <vector>

namespace souffle::pthread_runtime {

inline thread_local std::size_t workerLane = 0;
inline std::size_t threadCount = 1;
inline bool enabled = true;

inline std::size_t hardwareThreads() {
    long count = sysconf(_SC_NPROCESSORS_ONLN);
    return count > 0 ? static_cast<std::size_t>(count) : 1;
}

inline void configure(std::size_t count, bool parallel = true) {
    threadCount = count == 0 ? hardwareThreads() : count;
    enabled = parallel;
}

// One persistent worker team. Only the controlling thread submits work. A new
// job is published under the mutex, and completion is a barrier before return.
class Pool {
    struct Worker { Pool* pool; std::size_t lane; };
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t ready = PTHREAD_COND_INITIALIZER;
    pthread_cond_t complete = PTHREAD_COND_INITIALIZER;
    std::vector<pthread_t> threads;
    std::vector<Worker> workers;
    std::function<void()> job;
    std::size_t generation = 0, remaining = 0;
    bool stopping = false;

    static void* entry(void* pointer) {
        auto& worker = *static_cast<Worker*>(pointer);
        workerLane = worker.lane;
        worker.pool->work();
        return nullptr;
    }

    void work() {
        std::size_t observed = 0;
        pthread_mutex_lock(&mutex);
        for (;;) {
            while (!stopping && generation == observed)
                pthread_cond_wait(&ready, &mutex);
            if (stopping) break;
            observed = generation;
            pthread_mutex_unlock(&mutex);
            job(); // The submitted wrapper catches all callback exceptions.
            pthread_mutex_lock(&mutex);
            if (--remaining == 0) pthread_cond_signal(&complete);
        }
        pthread_mutex_unlock(&mutex);
    }

    void stop() noexcept {
        pthread_mutex_lock(&mutex);
        stopping = true;
        pthread_cond_broadcast(&ready);
        pthread_mutex_unlock(&mutex);
        for (auto thread : threads) {
            if (pthread_join(thread, nullptr) != 0) std::terminate();
        }
        pthread_cond_destroy(&complete);
        pthread_cond_destroy(&ready);
        pthread_mutex_destroy(&mutex);
    }

public:
    explicit Pool(std::size_t count) : workers(count) {
        threads.reserve(count);
        for (std::size_t i = 0; i < count; ++i) {
            workers[i] = {this, i};
            pthread_t thread;
            int error = pthread_create(&thread, nullptr, entry, &workers[i]);
            if (error) {
                stop();
                throw std::system_error(error, std::generic_category(), "pthread_create");
            }
            threads.push_back(thread);
        }
    }
    Pool(const Pool&) = delete;
    Pool& operator=(const Pool&) = delete;
    ~Pool() { stop(); }

    std::size_t size() const { return threads.size(); }

    void run(std::function<void()> task) {
        pthread_mutex_lock(&mutex);
        job = std::move(task);
        remaining = threads.size();
        ++generation;
        pthread_cond_broadcast(&ready);
        while (remaining != 0) pthread_cond_wait(&complete, &mutex);
        job = {};
        pthread_mutex_unlock(&mutex);
    }
};

inline Pool& workerPool() {
    static thread_local std::unique_ptr<Pool> pool;
    if (!pool || pool->size() != threadCount) pool = std::make_unique<Pool>(threadCount);
    return *pool;
}

template <class Parts, class Function>
void parallelFor(const Parts& parts, Function&& function) {
    if (!enabled || threadCount == 1 || parts.size() <= 1) {
        for (const auto& part : parts) function(part);
        return;
    }
    // Retain workers across rules and fixpoint iterations.
    auto& pool = workerPool();
    std::atomic<std::size_t> next{0};
    std::atomic<bool> failed{false};
    std::exception_ptr exception;
    pthread_mutex_t exceptionMutex = PTHREAD_MUTEX_INITIALIZER;
    pool.run([&] {
        try {
            while (!failed.load(std::memory_order_relaxed)) {
                auto index = next.fetch_add(1, std::memory_order_relaxed);
                if (index >= parts.size()) break;
                function(parts[index]);
            }
        } catch (...) {
            pthread_mutex_lock(&exceptionMutex);
            if (!exception) exception = std::current_exception();
            pthread_mutex_unlock(&exceptionMutex);
            failed.store(true, std::memory_order_relaxed);
        }
    });
    pthread_mutex_destroy(&exceptionMutex);
    if (exception) std::rethrow_exception(exception);
}

} // namespace souffle::pthread_runtime
