#include <iostream>
#include <iomanip>
#include <chrono>
#include <string>
#include <vector>
#include <cmath>

#include "black_scholes.h"
#include "monte_carlo.h"

// ─── Timing helper ────────────────────────────────────────────────────────────
using Clock = std::chrono::high_resolution_clock;

double elapsed_ms(Clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

// ─── Pretty printing ──────────────────────────────────────────────────────────
void print_separator(char c = '-', int width = 70) {
    std::cout << std::string(width, c) << "\n";
}

void print_bs(const BSResult& bs, const std::string& /*type*/) {
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "  Price  : " << bs.price  << "\n";
    std::cout << "  Delta  : " << bs.delta  << "\n";
    std::cout << "  Gamma  : " << bs.gamma  << "\n";
    std::cout << "  Vega   : " << bs.vega   << "  (per 1% vol move)\n";
    std::cout << "  Theta  : " << bs.theta  << "  (per calendar day)\n";
}

void print_mc(const MCResult& mc, double bs_price, const std::string& label) {
    double abs_err = std::abs(mc.price - bs_price);
    double rel_err = abs_err / bs_price * 100.0;
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "  [" << label << "]\n";
    std::cout << "    Price       : " << mc.price << "\n";
    std::cout << "    Std Error   : " << mc.std_error << "\n";
    std::cout << "    95% CI      : [" << mc.ci_lower << ", " << mc.ci_upper << "]\n";
    std::cout << std::setprecision(4);
    std::cout << "    Abs error   : " << abs_err << "  (" << rel_err << "% vs BS)\n";
    std::cout << std::setprecision(2);
    std::cout << "    Time        : " << mc.elapsed_ms << " ms\n";
    std::cout << "    Paths       : " << mc.n_paths << "\n\n";
}

// ─── Convergence study ────────────────────────────────────────────────────────
void convergence_study(double S, double K, double r, double sigma, double T,
                       const std::string& type, double bs_price) {
    print_separator('=');
    std::cout << "CONVERGENCE STUDY  (" << type << ")\n";
    print_separator('=');
    std::cout << std::setw(12) << "Paths"
              << std::setw(12) << "MC Price"
              << std::setw(12) << "Abs Error"
              << std::setw(14) << "Std Error"
              << std::setw(12) << "Time(ms)" << "\n";
    print_separator();

    for (long n : {1000L, 10000L, 100000L, 500000L, 1000000L, 5000000L}) {
        auto t0 = Clock::now();
        MCResult mc = mc_threaded(S, K, r, sigma, T, type, n);
        mc.elapsed_ms = elapsed_ms(t0);
        double err = std::abs(mc.price - bs_price);
        std::cout << std::fixed << std::setprecision(6)
                  << std::setw(12) << n
                  << std::setw(12) << mc.price
                  << std::setw(12) << err
                  << std::setw(14) << mc.std_error
                  << std::setprecision(2)
                  << std::setw(12) << mc.elapsed_ms << "\n";
    }
    std::cout << "\n";
}

// ─── Parallelization benchmark ────────────────────────────────────────────────
void benchmark(double S, double K, double r, double sigma, double T,
               const std::string& type, long n_paths) {
    print_separator('=');
    std::cout << "PARALLELIZATION BENCHMARK  (" << n_paths << " paths)\n";
    print_separator('=');

    // Serial
    auto t0 = Clock::now();
    MCResult serial = mc_serial(S, K, r, sigma, T, type, n_paths);
    serial.elapsed_ms = elapsed_ms(t0);

    // std::thread
    t0 = Clock::now();
    MCResult threaded = mc_threaded(S, K, r, sigma, T, type, n_paths);
    threaded.elapsed_ms = elapsed_ms(t0);

    // OpenMP
    t0 = Clock::now();
    MCResult omp = mc_openmp(S, K, r, sigma, T, type, n_paths);
    omp.elapsed_ms = elapsed_ms(t0);

    unsigned hw = std::thread::hardware_concurrency();
    std::cout << "Hardware threads available: " << hw << "\n\n";

    std::cout << std::fixed << std::setprecision(2);
    std::cout << std::setw(20) << "Method"
              << std::setw(14) << "Price"
              << std::setw(14) << "Time (ms)"
              << std::setw(12) << "Speedup" << "\n";
    print_separator();

    auto row = [&](const std::string& name, const MCResult& r, double base_ms) {
        std::cout << std::setw(20) << name
                  << std::setw(14) << std::setprecision(6) << r.price
                  << std::setw(14) << std::setprecision(2) << r.elapsed_ms
                  << std::setw(12) << std::setprecision(2) << base_ms / r.elapsed_ms << "x\n";
    };

    row("Serial",       serial,   serial.elapsed_ms);
    row("std::thread",  threaded, serial.elapsed_ms);
    row("OpenMP",       omp,      serial.elapsed_ms);
    std::cout << "\n";
}

// ─── Main ─────────────────────────────────────────────────────────────────────
int main() {
    // Option parameters
    double S     = 100.0;   // spot price
    double K     = 100.0;   // strike (ATM)
    double r     = 0.05;    // risk-free rate
    double sigma = 0.20;    // volatility
    double T     = 1.0;     // 1 year to expiry

    std::cout << "\n";
    print_separator('=');
    std::cout << "MONTE CARLO OPTION PRICER\n";
    std::cout << "European Options — Black-Scholes Validation + Parallelization Benchmark\n";
    print_separator('=');
    std::cout << "Parameters:  S=" << S << "  K=" << K
              << "  r=" << r << "  sigma=" << sigma << "  T=" << T << "\n\n";

    // ── Analytical Black-Scholes ──────────────────────────────────────────────
    for (const std::string type : {"call", "put"}) {
        print_separator('=');
        std::cout << "BLACK-SCHOLES ANALYTICAL  (" << type << ")\n";
        print_separator('=');
        BSResult bs = black_scholes(S, K, r, sigma, T, type);
        print_bs(bs, type);
        std::cout << "\n";

        // ── MC Validation (1M paths each method) ─────────────────────────────
        long n_val = 1'000'000;
        print_separator();
        std::cout << "MONTE CARLO VALIDATION  (" << type << ", " << n_val << " paths)\n";
        print_separator();

        auto t0 = Clock::now();
        MCResult s_res = mc_serial(S, K, r, sigma, T, type, n_val);
        s_res.elapsed_ms = elapsed_ms(t0);
        print_mc(s_res, bs.price, "Serial");

        t0 = Clock::now();
        MCResult t_res = mc_threaded(S, K, r, sigma, T, type, n_val);
        t_res.elapsed_ms = elapsed_ms(t0);
        print_mc(t_res, bs.price, "std::thread");

        t0 = Clock::now();
        MCResult o_res = mc_openmp(S, K, r, sigma, T, type, n_val);
        o_res.elapsed_ms = elapsed_ms(t0);
        print_mc(o_res, bs.price, "OpenMP");

        // ── Convergence study ─────────────────────────────────────────────────
        convergence_study(S, K, r, sigma, T, type, bs.price);
    }

    // ── Parallelization benchmark (5M paths) ─────────────────────────────────
    benchmark(S, K, r, sigma, T, "call", 5'000'000);

    // ── Put-Call Parity check ─────────────────────────────────────────────────
    print_separator('=');
    std::cout << "PUT-CALL PARITY CHECK\n";
    print_separator('=');
    BSResult call_bs = black_scholes(S, K, r, sigma, T, "call");
    BSResult put_bs  = black_scholes(S, K, r, sigma, T, "put");
    double parity_lhs = call_bs.price - put_bs.price;
    double parity_rhs = S - K * std::exp(-r * T);
    std::cout << std::fixed << std::setprecision(8);
    std::cout << "  C - P              : " << parity_lhs << "\n";
    std::cout << "  S - K*exp(-rT)     : " << parity_rhs << "\n";
    std::cout << "  Difference         : " << std::abs(parity_lhs - parity_rhs) << "\n";
    std::cout << "  Parity holds       : "
              << (std::abs(parity_lhs - parity_rhs) < 1e-10 ? "YES ✓" : "NO ✗") << "\n\n";

    return 0;
}
