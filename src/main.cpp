#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_map.h>
#include <tbb/scalable_allocator.h>
#include <tbb/parallel_for.h>

#include <queue>
#include <mutex>
#include <thread>
#include <chrono>
#include <iostream>
#include <vector>
#include <random>

using namespace std::chrono;

// Simulates packet processing workload
void simulate_work(int us_delay = 50) {
    std::this_thread::sleep_for(std::chrono::microseconds(us_delay));
}

// -----------------------------------------------
// Baseline: std::queue + std::mutex
// -----------------------------------------------
void test_std_mutex_queue(size_t num_threads, size_t num_items) {
    std::queue<int> q;
    std::mutex mtx;
    std::atomic<size_t> processed = 0;

    auto start = high_resolution_clock::now();

    auto producer = [&]() {
        for (size_t i = 0; i < num_items; ++i) {
            std::lock_guard<std::mutex> lock(mtx);
            q.push(i);
        }
    };

    auto consumer = [&]() {
        while (processed < num_items * num_threads) {
            std::lock_guard<std::mutex> lock(mtx);
            if (!q.empty()) {
                q.pop();
                ++processed;
            }
        }
    };

    std::vector<std::thread> threads;
    for (size_t i = 0; i < num_threads; ++i)
        threads.emplace_back(producer);
    threads.emplace_back(consumer);

    for (auto &t : threads)
        t.join();

    auto end = high_resolution_clock::now();
    auto elapsed_ms = duration_cast<milliseconds>(end - start).count();

    double items_per_sec = (processed.load() * 1000.0) / elapsed_ms;

    std::cout << "[std::mutex] Processed " << processed.load()
              << " items in " << elapsed_ms << " ms, "
              << "Throughput: " << items_per_sec << " items/sec" << std::endl;
}

// -----------------------------------------------
// Optimized: TBB concurrent_queue // will only be better in high contention scenarios otherwise overhead may dominate
// -----------------------------------------------
void test_tbb_concurrent_queue(size_t num_threads, size_t num_items) {
    tbb::concurrent_queue<int> cq;
    std::atomic<size_t> processed = 0;

    auto start = high_resolution_clock::now();

    auto producer = [&]() {
        for (size_t i = 0; i < num_items; ++i)
            cq.push(i);
    };

    auto consumer = [&]() {
        int value;
        while (processed < num_items * num_threads) {
            if (cq.try_pop(value)) {
                ++processed;
            } else {
                std::this_thread::yield();
            }
        }
    };

    std::vector<std::thread> threads;
    for (size_t i = 0; i < num_threads; ++i)
        threads.emplace_back(producer);
    threads.emplace_back(consumer);

    for (auto &t : threads)
        t.join();

    auto end = high_resolution_clock::now();
    auto elapsed_ms = duration_cast<milliseconds>(end - start).count();
    double items_per_sec = (processed.load() * 1000.0) / elapsed_ms;

    std::cout << "[TBB::concurrent_queue] Processed " << processed.load()
              << " items in " << elapsed_ms << " ms, "
              << "Throughput: " << items_per_sec << " items/sec" << std::endl;
}

// -----------------------------------------------
// Bonus: Concurrent unordered map example
// -----------------------------------------------
void test_tbb_concurrent_map(size_t num_threads, size_t num_items) {
    using Alloc = tbb::scalable_allocator<std::pair<const int, int>>;
    tbb::concurrent_unordered_map<int, int, std::hash<int>, std::equal_to<int>, Alloc> cmap;

    auto start = high_resolution_clock::now();

    tbb::parallel_for(size_t(0), num_threads, [&](size_t t) {
        for (size_t i = 0; i < num_items; ++i) {
            cmap[i + t * num_items] = int(i);
        }
    });

    auto end = high_resolution_clock::now();
    auto elapsed_ms = duration_cast<milliseconds>(end - start).count();
    double items_per_sec = (cmap.size() * 1000.0) / elapsed_ms;

    std::cout << "[TBB::concurrent_unordered_map] Processed "
              << cmap.size() << " items in "
              << elapsed_ms << " ms, "
              << "Throughput: " << items_per_sec << " items/sec" << std::endl;
}

// -----------------------------------------------
// Main entry point
// -----------------------------------------------
int main() {
    const size_t threads = std::thread::hardware_concurrency();
    const size_t items_per_thread = 100000;

    std::cout << "=== TBB Performance Demo ===\n";
    std::cout << "Threads: " << threads << "\n";
    std::cout << "Items per thread: " << items_per_thread << "\n\n";

    test_std_mutex_queue(threads, items_per_thread);
    //test_tbb_concurrent_queue(threads, items_per_thread); //uncomment if you want to see queue comparison but will likely be slower in low contention
    test_tbb_concurrent_map(threads, items_per_thread);

    std::cout << "\nComparison complete. Observe the latency difference above.\n";
    return 0;
}
