#pragma once
#include <vector>
#include <string>
#include <cmath>
#include <random>
#include <thread>
#include <mutex>
#include <numeric>
#include <stdexcept>
#ifdef _OPENMP
#include <omp.h>
#endif

struct MCResult {
    double price;
    double std_error;
    double ci_lower;   // 95% confidence interval
    double ci_upper;
    long   n_paths;
    double elapsed_ms;
};

// ─── Serial Monte Carlo ───────────────────────────────────────────────────────
// Each thread gets its own RNG seeded differently to avoid correlation.

double mc_payoff(double S, double K, double r, double sigma, double T,
                 const std::string& type, std::mt19937_64& rng) {
    std::normal_distribution<double> norm(0.0, 1.0);
    double Z = norm(rng);
    double ST = S * std::exp((r - 0.5 * sigma * sigma) * T + sigma * std::sqrt(T) * Z);
    if (type == "call") return std::max(ST - K, 0.0);
    if (type == "put")  return std::max(K - ST, 0.0);
    throw std::invalid_argument("type must be 'call' or 'put'");
}

MCResult mc_serial(double S, double K, double r, double sigma, double T,
                   const std::string& type, long n_paths, unsigned seed = 42) {
    std::mt19937_64 rng(seed);
    std::vector<double> payoffs(n_paths);
    for (long i = 0; i < n_paths; ++i)
        payoffs[i] = mc_payoff(S, K, r, sigma, T, type, rng);

    double discount = std::exp(-r * T);
    double mean = std::accumulate(payoffs.begin(), payoffs.end(), 0.0) / n_paths;
    double sq_sum = 0.0;
    for (double p : payoffs) sq_sum += (p - mean) * (p - mean);
    double variance = sq_sum / (n_paths - 1);
    double se = std::sqrt(variance / n_paths);

    MCResult res;
    res.price    = discount * mean;
    res.std_error = discount * se;
    res.ci_lower = res.price - 1.96 * res.std_error;
    res.ci_upper = res.price + 1.96 * res.std_error;
    res.n_paths  = n_paths;
    return res;
}

// ─── std::thread parallel Monte Carlo ────────────────────────────────────────

MCResult mc_threaded(double S, double K, double r, double sigma, double T,
                     const std::string& type, long n_paths,
                     unsigned n_threads = std::thread::hardware_concurrency()) {
    if (n_threads == 0) n_threads = 4;
    long paths_per_thread = n_paths / n_threads;

    std::vector<double> thread_sums(n_threads, 0.0);
    std::vector<double> thread_sq_sums(n_threads, 0.0);

    auto worker = [&](int tid) {
        std::mt19937_64 rng(tid * 1000 + 42); // distinct seed per thread
        long local_paths = (tid == (int)n_threads - 1)
                           ? n_paths - paths_per_thread * (n_threads - 1)
                           : paths_per_thread;
        double sum = 0.0, sq_sum = 0.0;
        for (long i = 0; i < local_paths; ++i) {
            double p = mc_payoff(S, K, r, sigma, T, type, rng);
            sum    += p;
            sq_sum += p * p;
        }
        thread_sums[tid]    = sum;
        thread_sq_sums[tid] = sq_sum;
    };

    std::vector<std::thread> threads;
    for (unsigned t = 0; t < n_threads; ++t)
        threads.emplace_back(worker, t);
    for (auto& th : threads) th.join();

    double total_sum    = std::accumulate(thread_sums.begin(), thread_sums.end(), 0.0);
    double total_sq_sum = std::accumulate(thread_sq_sums.begin(), thread_sq_sums.end(), 0.0);

    double mean     = total_sum / n_paths;
    double variance = (total_sq_sum / n_paths) - mean * mean;
    double se       = std::sqrt(variance / n_paths);
    double discount = std::exp(-r * T);

    MCResult res;
    res.price     = discount * mean;
    res.std_error = discount * se;
    res.ci_lower  = res.price - 1.96 * res.std_error;
    res.ci_upper  = res.price + 1.96 * res.std_error;
    res.n_paths   = n_paths;
    return res;
}

// ─── OpenMP parallel Monte Carlo ─────────────────────────────────────────────
// Compiled with -fopenmp. Falls back gracefully if OpenMP is unavailable.

MCResult mc_openmp(double S, double K, double r, double sigma, double T,
                   const std::string& type, long n_paths) {
    double sum = 0.0, sq_sum = 0.0;

#ifdef _OPENMP
    #pragma omp parallel reduction(+:sum, sq_sum)
    {
        std::mt19937_64 rng(omp_get_thread_num() * 1000 + 99);
        #pragma omp for schedule(static)
        for (long i = 0; i < n_paths; ++i) {
            double p = mc_payoff(S, K, r, sigma, T, type, rng);
            sum    += p;
            sq_sum += p * p;
        }
    }
#else
    // fallback: serial
    std::mt19937_64 rng(99);
    for (long i = 0; i < n_paths; ++i) {
        double p = mc_payoff(S, K, r, sigma, T, type, rng);
        sum    += p;
        sq_sum += p * p;
    }
#endif

    double mean     = sum / n_paths;
    double variance = (sq_sum / n_paths) - mean * mean;
    double se       = std::sqrt(variance / n_paths);
    double discount = std::exp(-r * T);

    MCResult res;
    res.price     = discount * mean;
    res.std_error = discount * se;
    res.ci_lower  = res.price - 1.96 * res.std_error;
    res.ci_upper  = res.price + 1.96 * res.std_error;
    res.n_paths   = n_paths;
    return res;
}
