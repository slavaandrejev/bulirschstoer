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
#include <limits>

#include <boost/test/unit_test_suite.hpp>
#include <boost/test/unit_test_log.hpp>
#include <boost/test/test_tools.hpp>

#include <boost/math/constants/constants.hpp>
#include <boost/multiprecision/mpfr.hpp>

#include <Eigen/Dense>

#include <bulirschstoer.h>

namespace bc = boost::math::constants;

auto constexpr pi = bc::pi<double>();

auto sqr(auto x) {
    return x * x;
}
auto cube(auto x) {
    return x * x * x;
}

template <size_t N>
using Ys = Eigen::Array<double, N, 1>;

namespace std {
    template <typename Derived>
    inline
    auto abs(const Eigen::ArrayBase<Derived> &x) {
        return x.abs();
    }
}

auto eval(auto &&x) {
    if constexpr (std::numeric_limits<std::decay_t<decltype(x)>>::is_specialized &&
                  !std::numeric_limits<std::decay_t<decltype(x)>>::is_integer)
    {
        return x;
    } else {
        return x.eval();
    }
}

template <int N, typename Derivs>
auto integrate(
    std::conditional_t<1 == N, double, Ys<N>&> y
  , double                                     x0
  , double                                     xend
  , double                                     h_init
  , double                                     eps
  , Derivs                                   &&derivs)
{
    using Y = std::conditional_t<1 == N, double, Ys<N>>;

    auto bulirsch_stoer = BulirschStoer<N, 12>{};

    auto dy   = Y{};
    auto x    = x0;
    auto step = h_init;
    while (std::abs(x - xend) > std::numeric_limits<double>::epsilon()) {
        auto dist_remaining = xend - x;
        auto current_step   = (std::abs(step) < std::abs(dist_remaining)) ? step : dist_remaining;
        derivs(x, y, dy);
        auto safe = []() {
            if constexpr (1 == N) {
                return 1e-30;
            } else {
                return Y::Constant(1e-30);
            }
        }();
        auto yscal = eval(std::abs(y) + std::abs(dy * current_step) + safe);
        auto [new_step, success] = bulirsch_stoer(y, dy, x, current_step, eps, yscal, derivs);
        if (success) {
            x += current_step;
        }
        step = new_step;
    }

    return y;
}

BOOST_AUTO_TEST_SUITE(BulirschStoerTests)

// -------------------------------------------------------------------
// 1.  y' = y,  y(0) = 1   =>  y(t) = e^t
//     Simple sanity check: scalar exponential growth.
// -------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(ExponentialGrowth)
{
    auto rhs = [](double, double y, double &dy) { dy = y; };

    auto y0 = 1.0;
    auto y = integrate<1>(y0, 0.0, 3.0, 0.5, 1e-12, rhs);

    BOOST_CHECK_CLOSE(y, std::exp(3.0), 1e-8);   // tolerance in %
}

// -------------------------------------------------------------------
// 2.  y' = -2y,  y(0) = 3   =>  y(t) = 3 e^{-2t}
//     Exponential decay – checks sign handling.
// -------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(ExponentialDecay)
{
    auto rhs = [](double, double y, double &dy) { dy = -2.0 * y; };

    auto y0 = 3.0;
    auto y = integrate<1>(y0, 0.0, 4.0, 0.5, 1e-12, rhs);

    BOOST_CHECK_CLOSE(y, 3.0 * std::exp(-8.0), 1e-6);
}

// -------------------------------------------------------------------
// 3.  Harmonic oscillator:  y'' + y = 0
//       y1' = y2,  y2' = -y1
//       y(0) = (1, 0)  =>  y1 = cos t,  y2 = -sin t
//     Integrated over 10 full periods (20π) to stress-test phase error.
// -------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(HarmonicOscillator)
{
    auto rhs = [](double, const auto &y, auto &dy) {
        dy(0) =  y(1);
        dy(1) = -y(0);
    };

    Ys<2> y0;  y0 << 1.0, 0.0;
    const double T = 10.0 * 2.0 * pi;
    auto y = integrate<2>(y0, 0.0, T, 0.5, 1e-12, rhs);

    // After 10 full periods should return to (1, 0).
    BOOST_CHECK_CLOSE(y(0), 1.0, 1e-6);
    BOOST_CHECK_SMALL(y(1),      1e-8);
}

// -------------------------------------------------------------------
// 4.  Logistic equation:  y' = y(1 - y),  y(0) = 0.5
//       y(t) = 1 / (1 + e^{-t})
//     Nonlinear but smooth; tests the extrapolation on a simple
//     scalar nonlinear ODE.
// -------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(LogisticEquation)
{
    auto rhs = [](double, double y, double &dy) {
        dy = y * (1.0 - y);
    };

    auto y0 = 0.5;
    auto y = integrate<1>(y0, 0.0, 10.0, 0.5, 1e-12, rhs);

    double exact = 1.0 / (1.0 + std::exp(-10.0));
    BOOST_CHECK_CLOSE(y, exact, 1e-8);
}

// -------------------------------------------------------------------
// 5.  Nonlinear blowup:  y' = y²,  y(0) = 1
//       y(t) = 1/(1 - t),  singular at t = 1.
//     Integrate to t = 0.9 where y = 10.  Small driver step needed
//     near the singularity.
// -------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(NonlinearBlowup)
{
    auto rhs = [](double, double y, double &dy) {
        dy = y * y;
    };

    auto y0 = 1.0;
    auto y = integrate<1>(y0, 0.0, 0.9, 0.1, 1e-10, rhs);

    BOOST_CHECK_CLOSE(y, 10.0, 1e-4);
}

// -------------------------------------------------------------------
// 6.  Damped harmonic oscillator:  y'' + 2ζω₀ y' + ω₀² y = 0
//       ζ = 0.1,  ω₀ = 1,  ω_d = √(1 - ζ²)
//       y(0) = 1, y'(0) = 0
//       y(t) = e^{-ζt} [ cos(ω_d t) + (ζ/ω_d) sin(ω_d t) ]
//       v(t) = -e^{-ζt} (ω₀²/ω_d) sin(ω_d t)
// -------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(DampedOscillator)
{
    constexpr double zeta = 0.1;
    constexpr double w0   = 1.0;
    const     double wd   = std::sqrt(1.0 - zeta * zeta);

    auto rhs = [](double, const auto &y, auto &dy) {
        dy(0) =  y(1);
        dy(1) = -0.2 * y(1) - y(0);
    };

    Ys<2> y0;  y0 << 1.0, 0.0;
    const double T = 15.0;
    auto y = integrate<2>(y0, 0.0, T, 0.5, 1e-12, rhs);

    double exact_y = std::exp(-zeta * T)
                   * (std::cos(wd * T) + (zeta / wd) * std::sin(wd * T));
    double exact_v = -std::exp(-zeta * T)
                   * (w0 * w0 / wd) * std::sin(wd * T);

    BOOST_CHECK_CLOSE(y(0), exact_y, 1e-7);
    BOOST_CHECK_CLOSE(y(1), exact_v, 1e-7);
}

// -------------------------------------------------------------------
// 7.  Kepler problem, eccentricity e = 0.5
//       State = (x, y, vx, vy),   a'' = -r/|r|³
//       IC at perihelion: r = (1-e, 0), v = (0, √((1+e)/(1-e)))
//       Period = 2π  (for semi-major axis a = 1).
//     After one full orbit the state must return to the IC.
// -------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(KeplerOrbitReturn)
{
    constexpr double ecc = 0.5;

    auto rhs = [](double, const auto &y, auto &dy) {
        double r2 = y(0) * y(0) + y(1) * y(1);
        double r3 = r2 * std::sqrt(r2);
        dy(0) =  y(2);
        dy(1) =  y(3);
        dy(2) = -y(0) / r3;
        dy(3) = -y(1) / r3;
    };

    Ys<4> y0;
    y0 << 1.0 - ecc, 0.0, 0.0, std::sqrt((1.0 + ecc) / (1.0 - ecc));

    auto y = integrate<4>(y0, 0.0, 2.0 * pi, 0.2, 1e-12, rhs);

    BOOST_CHECK_CLOSE(y(0), y0(0), 1e-5);
    BOOST_CHECK_SMALL(y(1),        1e-7);
    BOOST_CHECK_SMALL(y(2),        1e-7);
    BOOST_CHECK_CLOSE(y(3), y0(3), 1e-5);
}

// -------------------------------------------------------------------
// 8.  Kepler problem – energy conservation over 5 orbits
//     Higher eccentricity (e = 0.6) stresses the integrator more at
//     perihelion.  E = ½v² − 1/r must be conserved.
// -------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(KeplerEnergyConservation)
{
    constexpr double ecc = 0.6;

    auto rhs = [](double, const auto &y, auto &dy) {
        double r2 = y(0) * y(0) + y(1) * y(1);
        double r3 = r2 * std::sqrt(r2);
        dy(0) =  y(2);
        dy(1) =  y(3);
        dy(2) = -y(0) / r3;
        dy(3) = -y(1) / r3;
    };

    auto energy = [](const Ys<4> &y) {
        double v2 = y(2) * y(2) + y(3) * y(3);
        double r  = std::sqrt(y(0) * y(0) + y(1) * y(1));
        return 0.5 * v2 - 1.0 / r;
    };

    Ys<4> y0;
    y0 << 1.0 - ecc, 0.0, 0.0, std::sqrt((1.0 + ecc) / (1.0 - ecc));

    double E0 = energy(y0);
    auto y = integrate<4>(y0, 0.0, 5.0 * 2.0 * pi, 0.15, 1e-12, rhs);
    double E1 = energy(y);

    BOOST_CHECK_SMALL(E1 - E0, 1e-8);
}

// -------------------------------------------------------------------
// 9.  Lotka–Volterra predator-prey
//       x' = αx − βxy,   y' = δxy − γy
//     with α = 2/3, β = 4/3, δ = 1, γ = 1.
//     Conserved quantity: H = δx − γ ln(x) + βy − α ln(y).
//     Integrate for t = 30 and check H is preserved.
// -------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(LotkaVolterraInvariant)
{
    constexpr double a = 2.0 / 3.0, b = 4.0 / 3.0;
    constexpr double d = 1.0,        g = 1.0;

    auto rhs = [&](double, const auto &y, auto &dy) {
        dy(0) = a * y(0) - b * y(0) * y(1);
        dy(1) = d * y(0) * y(1) - g * y(1);
    };

    auto invariant = [&](const Ys<2> &y) {
        return d * y(0) - g * std::log(y(0))
             + b * y(1) - a * std::log(y(1));
    };

    Ys<2> y0;  y0 << 1.0, 0.5;
    double H0 = invariant(y0);

    auto y = integrate<2>(y0, 0.0, 30.0, 1, 1e-12, rhs);
    double H1 = invariant(y);

    BOOST_CHECK_SMALL(H1 - H0, 1e-7);
}

// -------------------------------------------------------------------
// 10. Coupled linear system:  y' = A y
//       A = [[-1, 2], [-2, -1]]
//     Eigenvalues −1 ± 2i  =>  decaying oscillation.
//       y(0) = (1, 0)
//       y(t) = e^{-t} (cos 2t, −sin 2t)
// -------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(CoupledLinearSystem)
{
    auto rhs = [](double, const auto &y, auto &dy) {
        dy(0) = -y(0) + 2.0 * y(1);
        dy(1) = -2.0 * y(0) - y(1);
    };

    Ys<2> y0;  y0 << 1.0, 0.0;
    const double T = 8.0;
    auto y = integrate<2>(y0, 0.0, T, 0.5, 1e-12, rhs);

    double exact0 =  std::exp(-T) * std::cos(2.0 * T);
    double exact1 = -std::exp(-T) * std::sin(2.0 * T);

    BOOST_CHECK_CLOSE(y(0), exact0, 1e-5);
    BOOST_CHECK_CLOSE(y(1), exact1, 1e-5);
}

// -------------------------------------------------------------------
// 11. This system optimizes time for a spaceship that flies from Earth orbit to
//     Mars orbit. The particular costates don't correspond to the final
//     solution but chosen for being problematic for the integrator. The ship
//     flies with constant acceleration, but can change the thrust direction.
//     Time is measured in hours, distance in au.
//       𝑟'' = 𝜇 / 𝑟² - 𝑟 𝜑'² = 𝑎 cos(𝜑)
//       2𝑟' 𝜑' + 𝑟 𝜑'' = 𝑎 sin(𝜑)
//       𝜆𝑟' = 𝜆𝑣𝑟 (𝜑'² - 2 𝜇 / 𝑟³) - 𝜆𝑣𝜑 𝑟' 𝜑' / r
//       𝜆𝑣𝑟' = 𝜆𝑣𝜑 𝜑' - 𝜆𝑟
//       𝜆𝑣𝜑' = 𝜆𝑣𝜑 𝑟' / 𝑟 - 2 𝜆𝑣𝑟 𝜑'
// -------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(MarsOptimalFlyby)
{
    static auto constexpr r_id   = 0;
    static auto constexpr 𝜑_id   = 1;
    static auto constexpr s_id   = 2;
    static auto constexpr dr_id  = 3;
    static auto constexpr d𝜑_id  = 4;
    static auto constexpr 𝜆r_id  = 5;
    static auto constexpr 𝜆vr_id = 6;
    static auto constexpr 𝜆v𝜑_id = 7;

    static auto constexpr 𝜇  = 5.137339e-7; // au^3 / hour^2

    static auto constexpr a  = 0.0849572; // au / hour^2

    auto derivs = [&](double, const auto &y, auto &dy) {
        auto cos_𝛼 = -y(𝜆vr_id);
        auto sin_𝛼 = -y(𝜆v𝜑_id);
        auto norm = std::sqrt(sqr(cos_𝛼) + sqr(sin_𝛼));
        sin_𝛼 /= norm;
        cos_𝛼 /= norm;

        dy(r_id)  = y(dr_id);
        dy(𝜑_id)  = y(d𝜑_id);
        dy(s_id)  = std::sqrt(sqr(y(dr_id)) + sqr(y(r_id) * y(d𝜑_id)));
        dy(dr_id) = a * cos_𝛼 - 𝜇 / sqr(y(r_id)) + y(r_id) * sqr(y(d𝜑_id));
        dy(d𝜑_id) = (a * sin_𝛼 - 2 * y(dr_id) * y(d𝜑_id)) / y(r_id);
        dy(𝜆r_id) =
            y(𝜆vr_id) * (sqr(y(d𝜑_id)) - 2 * 𝜇 / cube(y(r_id))) -
            y(𝜆v𝜑_id) * y(dr_id) * y(d𝜑_id) / y(r_id);
        dy(𝜆vr_id) = y(𝜆v𝜑_id) * y(d𝜑_id) - y(𝜆r_id);
        dy(𝜆v𝜑_id) = y(𝜆v𝜑_id) * y(dr_id) / y(r_id) - 2 * y(𝜆vr_id) * y(d𝜑_id);
    };

    auto y0 = Ys<8>{};
    auto h  = 1.0;
    y0 <<
        1.382193287969973
      , 0.007730483188100067
      , 0.3823190818607664
      , 0.25418784262805233
      , 0.007401511113784708
      , -3.8523681987042098
      , -0.21335812593926817
      , -0.1246541490233215
      ;
    auto y = integrate<8>(y0, 0.0, h, h, 1e-12, derivs);
    BOOST_CHECK_CLOSE(y(r_id), 1.60371216875223, 1e-7);
}

// -------------------------------------------------------------------
// 12. y' = y,  y(0) = 1   =>  y(t) = e^t
//     Scalar exponential growth with negative integration step.
// -------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(ExponentialGrowthNegStep)
{
    auto rhs = [](double, double y, double &dy) { dy = y; };

    auto y0 = std::exp(3.0);
    auto y  = integrate<1>(y0, 3.0, 0.0, -0.5, 1e-12, rhs);

    BOOST_CHECK_CLOSE(y, 1.0, 1e-8);   // tolerance in %
}

// -------------------------------------------------------------------
// 13. y' = y,  y(0) = 1   =>  y(t) = e^t
//     Scalar exponential growth with multiprecision
// -------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(ExponentialGrowthMP)
{
    using namespace boost::multiprecision;
    using precise_float = number<mpfr_float_backend<100, allocate_stack>>;

    auto rhs = [](auto, auto &&y, auto &&dy) { dy = y; };

    auto bulirsch_stoer = BulirschStoer<1, 30, precise_float>{0.94};

    auto t  = precise_float{0.0};
    auto y  = precise_float{1.0};
    auto dy = precise_float{};
    auto 𝛿t = precise_float{0.01};
    while (abs(1.0 - t) > precise_float{1e-99}) {
        auto remaining_time = 1.0 - t;
        auto current_step   = (abs(𝛿t) < abs(remaining_time)) ? 𝛿t : remaining_time;
        rhs(0, y, dy);
        auto yscal = abs(y) + abs(dy * current_step);
        auto [new_step, success] = bulirsch_stoer(y, dy, t, current_step, 1e-90, yscal, rhs);
        if (success) {
            t += current_step;
        }
        𝛿t = new_step;
    }
    BOOST_CHECK(y.str(88, std::ios_base::fixed) == "2.7182818284590452353602874713526624977572470936999595749669676277240766303535475945713822");
}

// -------------------------------------------------------------------
// 14. y' = 4 / (1 + t^2),  y(0) = 0
//     High precision 𝜋 calculation
// -------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(PiMP)
{
    using namespace boost::multiprecision;
    using precise_float = number<mpfr_float_backend<100, allocate_stack>>;

    auto rhs = [](auto t, auto &&y, auto &&dy) { dy = 4 / (1 + t * t); };

    auto bulirsch_stoer = BulirschStoer<1, 30, precise_float>{0.94};

    auto t  = precise_float{0.0};
    auto y  = precise_float{0.0};
    auto dy = precise_float{};
    auto 𝛿t = precise_float{0.01};
    while (abs(1.0 - t) > precise_float{1e-99}) {
        auto remaining_time = 1.0 - t;
        auto current_step   = (abs(𝛿t) < abs(remaining_time)) ? 𝛿t : remaining_time;
        rhs(t, y, dy);
        auto yscal = abs(y) + abs(dy * current_step);
        auto [new_step, success] = bulirsch_stoer(y, dy, t, current_step, 1e-90, yscal, rhs);
        if (success) {
            t += current_step;
        }
        𝛿t = new_step;
    }
    BOOST_CHECK(y.str(88, std::ios_base::fixed) == "3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348");
}

BOOST_AUTO_TEST_SUITE_END()
