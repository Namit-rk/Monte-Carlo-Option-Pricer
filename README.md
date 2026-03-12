# Monte Carlo Option Pricer 

A high-performance European option pricer implementing Monte Carlo simulation with three execution modes — serial, `std::thread`, and OpenMP — benchmarked against analytical Black-Scholes pricing.

---

## What it does

- Prices **European call and put options** via Monte Carlo path simulation (GBM)
- Validates results against **analytical Black-Scholes** with absolute/relative error reporting
- Computes full **Greeks** analytically: Delta, Gamma, Vega, Theta
- Runs a **convergence study** (1K → 5M paths) showing error decay proportional to 1/√N
- Benchmarks **three parallelization strategies** on your hardware
- Verifies **put-call parity** to machine precision

---

## Sample output

```
======================================================================
MONTE CARLO OPTION PRICER
European Options — Black-Scholes Validation + Parallelization Benchmark
======================================================================
Parameters:  S=100  K=100  r=0.05  sigma=0.2  T=1

BLACK-SCHOLES ANALYTICAL  (call)
  Price  : 10.450584
  Delta  : 0.636831
  Gamma  : 0.018762
  Vega   : 0.375240  (per 1% vol move)
  Theta  : -0.017573  (per calendar day)

MONTE CARLO VALIDATION  (call, 1,000,000 paths)
  [Serial]      Price: 10.453732   Abs error: 0.0031 (0.03% vs BS)   Time: 84.74 ms
  [std::thread] Price: 10.467160   Abs error: 0.0166 (0.16% vs BS)   Time: 59.11 ms
  [OpenMP]      Price: 10.428197   Abs error: 0.0224 (0.21% vs BS)   Time: 26.96 ms

CONVERGENCE STUDY  (call)
     Paths    MC Price   Abs Error     Std Error    Time(ms)
      1,000   10.849485   0.398901     0.496464        0.60
     10,000   10.385743   0.064840     0.146701        0.81
    100,000   10.447251   0.003333     0.046635        5.62
  1,000,000   10.467160   0.016576     0.014724       27.64
  5,000,000   10.446470   0.004114     0.006583      292.86

PARALLELIZATION BENCHMARK  (5,000,000 paths)
         Serial        362.23 ms   1.00x
    std::thread        261.12 ms   1.39x
         OpenMP        296.22 ms   1.22x

PUT-CALL PARITY CHECK
  C - P            : 4.87705755
  S - K*exp(-rT)   : 4.87705755
  Difference       : 0.00000000   ✓
```

---

## Project structure

```
.
├── include/
│   ├── black_scholes.h    # Analytical BS pricing + Greeks
│   └── monte_carlo.h      # Serial, std::thread, OpenMP pricers
├── src/
│   └── main.cpp           # Benchmarks, convergence study, validation
├── Makefile
└── README.md
```

---

## Build & run

**Requirements:** g++ with C++17 support, OpenMP

```bash
# Clone
git clone https://github.com/<your-username>/monte-carlo-option-pricer
cd monte-carlo-option-pricer

# Build (OpenMP + std::thread)
make

# Run
./pricer

# Without OpenMP
make no-omp && ./pricer
```

---

## Implementation notes

### GBM path simulation
Stock price at expiry sampled directly from the log-normal distribution:

```
S_T = S * exp((r - σ²/2) * T + σ * √T * Z),   Z ~ N(0,1)
```

This is exact — no discretization error.

### Parallelization

Each thread uses an independently seeded `std::mt19937_64` RNG to avoid correlation between threads. Three strategies are compared:

| Strategy | Description |
|---|---|
| Serial | Single-threaded baseline |
| `std::thread` | Manual thread pool, one RNG per thread |
| OpenMP | `#pragma omp parallel reduction` with per-thread RNG |

### Convergence

Monte Carlo error decays as **1/√N** — confirmed empirically in the convergence study. At 5M paths, absolute pricing error is < 0.005 vs analytical Black-Scholes.

### Put-call parity

Analytical prices satisfy C − P = S − K·e^(−rT) to machine precision (< 1e-10), serving as a correctness check on the BS implementation.

---

## Key results

- MC price converges to within **0.03% of Black-Scholes** at 1M paths
- **std::thread** achieves ~1.4x speedup; **OpenMP** ~1.2x on the test machine (2 hardware threads)
- Put-call parity holds to **8 decimal places** ✓
- Standard error decays proportional to **1/√N** as expected by the CLT

---

## Skills demonstrated

- Quantitative finance: GBM, Black-Scholes, Greeks, put-call parity
- Numerical methods: Monte Carlo convergence, variance estimation, confidence intervals
- C++17: templates, RAII, move semantics, chrono timing
- Parallelism: `std::thread` manual threading vs OpenMP reduction clauses
- Software design: header-only library, clean separation of concerns
