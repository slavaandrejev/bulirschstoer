/*
 * Copyright (c) 2026, Slava Andrejev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * --- Optional exception to the license ---
 *
 * As an exception, if, as a result of your compiling your source code, portions
 * of this Software are embedded into a machine-executable object form of such
 * source code, you may redistribute such embedded portions in such object form
 * without including the above copyright and permission notices.
 */
#pragma once

#include <array>
#include <concepts>
#include <limits>
#include <type_traits>
#include <utility>

#include <Eigen/Dense>

#include <range/v3/algorithm/min_element.hpp>
#include <range/v3/view/take.hpp>

// `RealNumber` can be a class that behaves as a real number, in contrast to
// std::floating_point, which is strictly a built-in type.
template <typename T>
concept RealNumber = std::numeric_limits<T>::is_specialized &&
                     !std::numeric_limits<T>::is_integer;

template <int _Nvars, int _MaxOrder, RealNumber T = double>
    requires (0 < _Nvars && 1 < _MaxOrder)
class BulirschStoer {
private:
    using Ys = Eigen::Array<T, _Nvars, 1>;

public:
    static auto constexpr MaxOrder = _MaxOrder;
    static auto constexpr Nvars    = _Nvars;

    using ValType = std::conditional_t<
        1 == Nvars
      , T
      , Ys
      >;
    using ValConstRef = std::conditional_t<
        std::is_floating_point_v<ValType>
      , ValType
      , const ValType &>;
    using Scalar = T;
    using ScalarConstRef = std::conditional_t<
        std::is_floating_point_v<Scalar>
      , Scalar
      , const Scalar &>;

    BulirschStoer() = default;
    BulirschStoer(ScalarConstRef safety_factor)
      : safety_factor(safety_factor)
    {}
    BulirschStoer(ScalarConstRef safety_factor, ScalarConstRef error_margin)
      : safety_factor(safety_factor)
      , error_margin(error_margin)
    {}

    template <typename Derivs>
        requires
            std::invocable<
                Derivs
              , ScalarConstRef
              , ValConstRef
              , ValType &
              >
    std::tuple<Scalar, bool> // new recommended step, success
    operator()(
        ValType       &y      // current y estimates, updated by this function
      , ValConstRef    dy     // y' evaluated at x
      , ScalarConstRef x      // the initial x
      , ScalarConstRef H      // total step
      , ScalarConstRef eps    // tolerance
      , ValConstRef    yscal  // scale factors for y for error estimates
      , Derivs       &&derivs // a function that calculates derivatives
      )
    {
        namespace rs = ranges;
        namespace rv = rs::views;

        using std::abs;
        using std::copysign;
        using std::min;
        using std::pow;
        using std::sqrt;

        auto yerr  = ValType{};
        auto nexty = ValType{};
        auto 𝛿y    = ValType{};

        auto errs          = std::array<Scalar, MaxOrder>{};
        auto step_accepted = false;
        auto k_converged   = 0;

        i = 0; // reset the extrapolation tableau
        for (auto k = 0; MaxOrder > k; ++k) {
            midpoint(y, dy, x, H, ni[k], nexty, std::forward<Derivs>(derivs));
            // Add a row to the extrapolation tableau. Since midpoint returns
            // 𝛿y, we extrapolating not the full state by a 𝛿 to the current
            // state.
            increase_order(nexty, 𝛿y, yerr);
            if (0 != k) {
                auto errmax = Scalar{};
                if constexpr (1 == Nvars) {
                    errmax = abs(yerr / yscal);
                } else {
                    errmax = (yerr / yscal).abs().maxCoeff();
                }
                errmax /= eps;
                errs[k] = errmax;

                // If we exceed our expected order window by too much, abandon
                // the step. It is cheaper to halve H and start over than to
                // compute up to MaxOrder.
                if (k > suggested_order + 1) {
                    break;
                }

                if (1.0 > errmax) {
                    step_accepted = true;
                    k_converged = k;
                    break;
                }
            }
        }

        if (!step_accepted) {
            // Complete failure (Shampine's dumb halving)
            safe_leash = 1.0;
            return {H * 0.5, false};
        }

        // restore the full state from the extrapolated 𝛿
        y += 𝛿y;

        auto H_opt = std::array<Scalar, MaxOrder - 1>{};
        auto W     = std::array<Scalar, MaxOrder - 1>{};

        // Calculate the ideal step size and work for all computed columns
        for (auto k = 1; k_converged >= k; ++k) {
            auto exp = Scalar{1.0} / (2.0 * k + 1.0);
            H_opt[k - 1] = H * safety_factor * pow(error_margin / errs[k], exp);
            W[k - 1] = Ai[k] / abs(H_opt[k - 1]);
        }

        // Find the column 'q' that minimizes the Work (W_k)
        auto min_it = rs::min_element(W | rv::take(k_converged));
        auto q      = min_it - W.begin() + 1;
        auto W_min  = *min_it;

        // Determine the next step size
        auto H_next = H_opt[q - 1];
        // Shampine's 10% heuristic: guess if order q + 1 would be better
        if (q == k_converged && q + 1 < MaxOrder) {
            auto H_guess = H_next * (Scalar(Ai[q + 1]) / Ai[q]) * Scalar{1.1};
            // If the higher order buys us a big enough step to justify the cost, use it
            if (Ai[q + 1] / abs(H_guess) < W_min) {
                H_next = H_guess;
                q = q + 1;
            }
        }

        // Apply Shampine's SAFE leash to prevent yo-yoing
        safe_leash = min(Scalar{100.0}, safe_leash * 3.0);
        auto H_next_mag = min(abs(H_next), abs(H) * safe_leash);
        H_next = copysign(H_next_mag, H);

        suggested_order = q;

        return {H_next, true};
    }

private:
    static
    constexpr auto NiInit() {
        auto ni = std::array<int, MaxOrder>{};
        for (auto i = 0; MaxOrder > i; ++i) {
            ni[i] = 2 * (i + 1);
        }
        return ni;
    }
    static
    constexpr auto AiInit() {
        auto ni = NiInit();
        auto Ai = std::array<int, MaxOrder>{ni[0] + 1};
        for (auto i = 1; MaxOrder > i; ++i) {
            Ai[i] = Ai[i - 1] + ni[i];
        }
        return Ai;
    }
    static
    constexpr std::array<int, MaxOrder> ni = NiInit();
    static
    constexpr std::array<int, MaxOrder> Ai = AiInit();

    static
    auto eval(ValConstRef x) {
        if constexpr (1 == Nvars) {
            return x;
        } else {
            return x.eval();
        }
    }
    static constexpr
    auto sqr(auto x) { return x * x; }

    // Add a new row to the extrapolation tableau
    void increase_order(
        ValConstRef y   // a new sample
      , ValType    &ext // returned extrapolation
      , ValType    &err // subdiagonal error estimation
      )
    {
        using std::abs;

        if (0 == i) {
            ext = err = q[0] = y;
        } else {
            ext = y;
            // `d` represents D[i, k - 1] (see the paper). Since it is the same
            // row of the table, we need just one variable, no need to store all
            // of them. D[i, 0] = f[i].
            auto d = y;
            // Memorize Q[i - 1, k = 0] for k = 1.
            auto prevqkm1 = q[0];
            // Q[i, 0] = f[i].
            q[0] = y;
            auto curNi2 = Scalar(sqr(ni[i]));
            for (auto k = 1; i >= k; ++k) {
                // (h[i - k] / h[i])^2 * Q[i - 1, k - 1]
                auto h_ratio_q = eval(curNi2 / sqr(ni[i - k]) * prevqkm1);
                // (D[i, k - 1] - Q[i - 1, k - 1]) / ((h[i - k] / h[i])^2 * Q[i - 1, k - 1] - D[i, k - 1])
                // The implementation below checks if rational extrapolation is
                // degenerate and switches to polynomial extrapolation if it is.
                // It works because Q represents a refinement of the
                // extrapolation when we add an extra sample. Subsequent
                // iterations will still refine the previous values regardless
                // of if they were produced by rational or polynomial
                // algorithms.
                auto Δ     = eval(d - prevqkm1);  // D[i, k - 1] - Q[i - 1, k - 1]
                auto denom = eval(h_ratio_q - d); // (h[i - k] / h[i])^2 * Q[i - 1, k - 1] - D[i, k - 1]
                auto new_q = ValType{};
                const Scalar ratio_limit(1.0e5);
                if constexpr (1 == Nvars) {
                    // note that `>=` also handles the case when both `Δ` and `denom` are zero
                    const bool reject_rational = abs(Δ) >= (ratio_limit * abs(denom));
                    if (!reject_rational) {
                        auto r = Δ / denom; // (D[i, k - 1] - Q[i - 1, k - 1]) / ((h[i - k] / h[i])^2 * Q[i - 1, k - 1] - D[i, k - 1])
                        ext += new_q = d * r;
                        d = h_ratio_q * r;
                    } else {
                        // fallback to polynomial extrapolation
                        ext += new_q = sqr(ni[i - k]) / (curNi2 - sqr(ni[i - k])) * Δ;
                        d = curNi2 / (curNi2 - sqr(ni[i - k])) * Δ;
                    }
                } else {
                    auto reject_mask = (Δ.abs() >= (ratio_limit * denom.abs())).eval();

                    // Polynomial fallback. We compute this for ALL elements,
                    // even those that don't need it. (Scalar math is cheap
                    // compared to pipeline stalls).
                    auto nik2           = sqr(ni[i - k]);
                    auto ni2_minus_nik2 = curNi2 - sqr(ni[i - k]);
                    auto new_q_poly     = (nik2 / ni2_minus_nik2 * Δ).eval();
                    auto d_poly         = (curNi2 / ni2_minus_nik2) * Δ;

                    // Rational extrapolation. To prevent division by zero,
                    // replace small values with 1.0.
                    auto denom_safe = (!reject_mask).select(denom, Scalar(1.0)).eval();
                    auto r          = (Δ / denom_safe).eval();
                    auto new_q_rat  = (d * r).eval();
                    auto d_rat      = h_ratio_q * r;

                    // Select the correct result per-element. If mask is true,
                    // pick polynomial. Else pick rational.
                    new_q = reject_mask.select(new_q_poly, new_q_rat);

                    // Update `d` for the next iteration
                    d = reject_mask.select(d_poly, d_rat);

                    // Accumulate result
                    ext += new_q;
                }
                if (i != k) {
                    prevqkm1 = q[k];
                }
                q[k] = new_q;
            }
            err = q[i]; // subdiagonal error estimate T[i, i] - T[i, i - 1]
        }
        ++i;
    }

    // Multistep midpoint integration. Note that it returns a 𝛿y you should add
    // to y to get the new state.
    template <typename Derivs>
        requires
            std::invocable<
                Derivs
              , ScalarConstRef
              , ValConstRef
              , ValType &
              >
    void midpoint(
        ValConstRef    y     // y evaluated at xs
      , ValConstRef    dy    // y' evaluated at xs
      , ScalarConstRef xs    // the initial x
      , ScalarConstRef H     // total step
      , int            nstep // the number of substeps
      , ValType       &yout  // the result
      , Derivs       &&derivs
      )
    {
        using std::swap;

        const auto h  = H / nstep;
        const auto h2 = 2 * h;

        auto dym = []() {
            if constexpr (1 == Nvars) {
                return Scalar{0.0};
            } else {
                return ValType{ValType::Zero()};
            }
        }();
        auto dyn = eval(h * dy);

        auto x = Scalar{xs + h};
        derivs(x, eval(y + dyn), yout); // use `yout` for temporary storage of derivatives

        for (auto n = 1; nstep > n; ++n) {
            dym += h2 * yout;
            swap(dym, dyn);
            x += h;
            derivs(x, eval(y + dyn), yout);
        }
        yout = (dym + dyn + h * yout) / 2;
    }

    int i = 0;                       // Current extrapolation step
    std::array<ValType, MaxOrder> q; // The last row of the Q[i, k] table.

    Scalar safe_leash      = 100.0;
    int    suggested_order = 3; // Initial guess for optimal order index

    Scalar safety_factor = 0.7;
    Scalar error_margin  = 0.65;
};
