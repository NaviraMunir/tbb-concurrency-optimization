# Optimizing Multithreaded Queues with Intel® TBB

*A practical guide to lock-free, scalable data structures and allocators in modern C++*

---

## Overview

This repository demonstrates how to drastically improve performance in multithreaded systems using **Intel® oneTBB (Threading Building Blocks)** — a C++ library providing **high-performance concurrent containers, task scheduling**, and **a scalable memory allocator**.

In our real-world use case, a high-throughput packet processing service suffered performance degradation during traffic bursts because multiple threads contended on shared queues. After integrating **TBB’s lock-free structures and scalable allocator**, we reduced peak packet processing latency from **1000 ms to just 50 ms**, achieving **up to 20× faster performance** under load.

---

## Why TBB?

Traditional concurrency often relies on `std::mutex` or `std::lock_guard` to protect shared data.
However, as thread counts grow, **global locks become bottlenecks** — introducing contention, context switching, and cache invalidation.

**Intel oneTBB** addresses this through:

1. **Fine-grained sharding and internal concurrency**
   Containers such as `tbb::concurrent_queue` and `tbb::concurrent_unordered_map` use internal shards guarded by lightweight locks or atomics, enabling multiple threads to operate simultaneously.

2. **Scalable memory allocation**
   `tbb::scalable_allocator` avoids the global heap lock of `malloc/new` by maintaining per-thread memory pools and a *remote free list* for safe cross-thread deallocations.

3. **Task-based parallelism**
   TBB’s scheduler automatically distributes tasks across cores — improving CPU utilization and reducing idle time.

---

## 🧱 Components Used

| Component                       | Description            | Benefit                                             |
| ------------------------------- | ---------------------- | --------------------------------------------------- |
| `tbb::concurrent_queue`         | Thread-safe FIFO queue | Lock-free push/pop operations                       |
| `tbb::concurrent_unordered_map` | Thread-safe hash map   | Internally sharded, minimizes contention            |
| `tbb::scalable_allocator`       | Memory allocator       | Removes global heap lock, improves allocation speed |
| `tbb::atomic`                   | Atomic counter type    | Lightweight and faster under contention             |

---

## ⚙️ Setup

### 1️⃣ Install TBB

**Ubuntu / Debian**

```bash
sudo apt install libtbb-dev
```

**Fedora / CentOS**

```bash
sudo dnf install tbb-devel
```

**CMake Example**

```cmake
find_package(TBB REQUIRED)
target_link_libraries(my_app PRIVATE TBB::tbb)
```

---

## 🧠 Example 1: Replacing a Locked Queue

### 🧱 Before (mutex-protected queue)

```cpp
#include <queue>
#include <mutex>

std::queue<int> q;
std::mutex q_mtx;

void producer() {
    for (int i = 0; i < 100000; ++i) {
        std::lock_guard<std::mutex> lock(q_mtx);
        q.push(i);
    }
}

void consumer() {
    while (true) {
        std::lock_guard<std::mutex> lock(q_mtx);
        if (!q.empty()) q.pop();
    }
}
```

🔴 *Under load, threads constantly wait for each other, leading to latency spikes.*

---

### ⚡ After (lock-free queue)

```cpp
#include <tbb/concurrent_queue.h>

tbb::concurrent_queue<int> cq;

void producer() {
    for (int i = 0; i < 100000; ++i)
        cq.push(i);
}

void consumer() {
    int value;
    while (cq.try_pop(value)) {
        // process(value);
    }
}
```

✅ Lock-free and wait-free operations
✅ Scales linearly with CPU cores
✅ Drastically reduced thread contention

---

## 🧠 Example 2: Using a Scalable Allocator

### Before

```cpp
std::vector<std::string> logs;
```

### After

```cpp
#include <tbb/scalable_allocator.h>
#include <vector>
#include <string>

std::vector<std::string, tbb::scalable_allocator<std::string>> logs;
```

✅ Each thread gets its own memory pool
✅ Eliminates global heap locks
✅ Lower fragmentation and better cache locality

---

## ⚖️ `malloc` vs `tbb::scalable_allocator`

| Aspect        | malloc / calloc          | TBB scalable_allocator          |
| ------------- | ------------------------ | ------------------------------- |
| Locking       | Global heap lock         | Per-thread memory pools         |
| Fragmentation | High under concurrency   | Reduced via pooled blocks       |
| Scalability   | Degrades beyond ~4 cores | Scales to 64+ cores             |
| Overhead      | OS-level synchronization | Lightweight atomic coordination |

---

## 📊 Real-World Performance Results

| Metric                            | Before (mutex-based) | After (TBB concurrent + scalable_allocator) |
| :-------------------------------- | :------------------- | :------------------------------------------ |
| **Avg. packet processing time**   | ~250 ms              | ~25–35 ms                                   |
| **Peak latency under contention** | ~1000 ms+            | ≤ 50 ms                                     |
| **CPU utilization**               | 60–70 %              | 90–95 %                                     |
| **Heap contention**               | High                 | Negligible                                  |
| **Overall improvement**           | –                    | **10–20× faster**                           |

🔍 This improvement was observed in a multi-threaded packet-handling service, where multiple threads enqueue and process real-time data concurrently.

---

## 🧩 Conclusion

Intel® oneTBB provides a **modern, high-performance C++ foundation** for parallelism — ideal for systems dealing with high data throughput, packet processing, logging, or analytics.

By replacing global locks and traditional heap allocations with **lock-free concurrent containers** and **scalable memory pools**, you can achieve:

* **Near-linear scalability**
* **Drastically reduced contention**
* **Consistent latency even under peak load**

TBB is primarily a **C++ library**, but its concepts (task graphs, work stealing, fine-grained concurrency) have inspired equivalents in other ecosystems — e.g., Java’s `ForkJoinPool` and .NET’s TPL.

---

## 📚 Learn More

* 🔗 [Intel® oneTBB Benefits]([https://www.intel.com/content/www/us/en/developer/tools/oneapi/onetbb.html](https://www.intel.com/content/www/us/en/docs/onetbb/developer-guide-api-reference/2022-1/onetbb-benefits.html#BENEFITS))
* 🧠 [Scalable Memory Allocator](https://www.researchgate.net/publication/334350254_Scalable_Memory_Allocation)
* 💻 [oneTBB GitHub Repository](https://github.com/oneapi-src/oneTBB)

---

## Run Given Example Code to see performance difference

```bash
git clone https://github.com/NaviraMunir/tbb-concurrency-optimization.git
cd tbb-performance-demo
mkdir build && cd build
cmake ..
make
./tbb_performance_demo
```
