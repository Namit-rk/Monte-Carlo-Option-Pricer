#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <chrono>

std::mutex print_lock;

void greet(int tid) {
    // ── work happens OUTSIDE the lock — asll threads run this simultaneously
    int sum = 0;
    for (int i = 0; i < 1000000; i++)
        sum++;

    // ── lock only for printing — as short as possible
    std::cout << "Thread " << tid << " counted to " << sum << "\n";
}

int main() {
    // ── Parallel ──────────────────────────────────────────────────────────
    auto t0 = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> threads;
    int n_threads = std::thread::hardware_concurrency();
    for (int t = 0; t < n_threads; ++t)
        threads.emplace_back(greet, t);
    // for (auto& th : threads)
    //     th.join();

    double ms_parallel = std::chrono::duration<double, std::milli>(
        std::chrono::high_resolution_clock::now() - t0).count();

    // ── Serial ────────────────────────────────────────────────────────────
    auto t1 = std::chrono::high_resolution_clock::now();

    for (int t = 0; t < n_threads; t++) {
        int sum = 0;
        for (int i = 0; i < 12000000; i++)
            sum++;
        std::cout << "Thread " << t << " counted to " << sum << "\n";
    }

    double ms_serial = std::chrono::duration<double, std::milli>(
        std::chrono::high_resolution_clock::now() - t1).count();

    // ── Results ───────────────────────────────────────────────────────────
    std::cout << "\nParallel : " << ms_parallel << " ms\n";
    std::cout << "Serial   : " << ms_serial   << " ms\n";
    std::cout << "Speedup  : " << ms_serial / ms_parallel << "x\n";

    return 0;
}


// ─── WHAT TO OBSERVE ─────────────────────────────────────────────────────────
// Run it several times. Notice the order of "Hello from thread X" changes
// every run — threads don't execute in a guaranteed order.
// This is the fundamental nature of parallelism.