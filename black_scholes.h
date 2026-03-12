#pragma once
#include <cmath>
#include <stdexcept>
#include <string>

// Standard normal CDF using erfc
inline double norm_cdf(double x) {
    return 0.5 * std::erfc(-x / std::sqrt(2.0));
}

// Standard normal PDF
inline double norm_pdf(double x) {
    return std::exp(-0.5 * x * x) / std::sqrt(2.0 * M_PI);
}

struct BSResult {
    double price;
    double delta;
    double gamma;
    double vega;
    double theta;
};

// Analytical Black-Scholes for European options
// S: spot, K: strike, r: risk-free rate, sigma: volatility, T: time to expiry
BSResult black_scholes(double S, double K, double r, double sigma, double T,
                       const std::string& type = "call") {
    if (T <= 0) throw std::invalid_argument("T must be positive");
    if (sigma <= 0) throw std::invalid_argument("sigma must be positive");
    if (S <= 0 || K <= 0) throw std::invalid_argument("S and K must be positive");

    double d1 = (std::log(S / K) + (r + 0.5 * sigma * sigma) * T) / (sigma * std::sqrt(T));
    double d2 = d1 - sigma * std::sqrt(T);

    BSResult res;

    if (type == "call") {
        res.price = S * norm_cdf(d1) - K * std::exp(-r * T) * norm_cdf(d2);
        res.delta = norm_cdf(d1);
    } else if (type == "put") {
        res.price = K * std::exp(-r * T) * norm_cdf(-d2) - S * norm_cdf(-d1);
        res.delta = norm_cdf(d1) - 1.0;
    } else {
        throw std::invalid_argument("type must be 'call' or 'put'");
    }

    // Greeks (same for call and put)
    res.gamma = norm_pdf(d1) / (S * sigma * std::sqrt(T));
    res.vega  = S * norm_pdf(d1) * std::sqrt(T) / 100.0; // per 1% move in vol
    res.theta = (-(S * norm_pdf(d1) * sigma) / (2.0 * std::sqrt(T))
                 - r * K * std::exp(-r * T) * (type == "call" ? norm_cdf(d2) : norm_cdf(-d2))) / 365.0;

    return res;
}
