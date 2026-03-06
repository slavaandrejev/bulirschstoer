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
#include <tuple>

#include <boost/multiprecision/mpfr.hpp>
#include <boost/multiprecision/eigen.hpp>

#include <Eigen/Dense>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <bulirschstoer.h>

using namespace boost::multiprecision;

using precise_float = number<mpfr_float_backend<100, allocate_stack>>;

using Ys = Eigen::Array<precise_float, 12, 1>;

struct stid {
    static auto constexpr r1  = 0;
    static auto constexpr r2  = 2;
    static auto constexpr r3  = 4;

    static auto constexpr dr1  = 6;
    static auto constexpr dr2  = 8;
    static auto constexpr dr3  = 10;

    static auto constexpr x1  = 0;
    static auto constexpr y1  = 1;
    static auto constexpr x2  = 2;
    static auto constexpr y2  = 3;
    static auto constexpr x3  = 4;
    static auto constexpr y3  = 5;

    static auto constexpr dx1 = 6;
    static auto constexpr dy1 = 7;
    static auto constexpr dx2 = 8;
    static auto constexpr dy2 = 9;
    static auto constexpr dx3 = 10;
    static auto constexpr dy3 = 11;
};

template <typename Backend, expression_template_option ET>
struct fmt::formatter<number<Backend, ET>> : fmt::ostream_formatter {};

auto cube(auto x) {
    return x * x * x;
}

void derivs(const Ys &y, Ys &dy) {
    auto r1 = y.segment<2>(stid::r1);
    auto r2 = y.segment<2>(stid::r2);
    auto r3 = y.segment<2>(stid::r3);

    auto r2mr1 = (r2 - r1) / cube((r2 - r1).matrix().norm());
    auto r3mr1 = (r3 - r1) / cube((r3 - r1).matrix().norm());
    auto r3mr2 = (r3 - r2) / cube((r3 - r2).matrix().norm());

    dy.segment<2>(stid::r1) = y.segment<2>(stid::dr1);
    dy.segment<2>(stid::r2) = y.segment<2>(stid::dr2);
    dy.segment<2>(stid::r3) = y.segment<2>(stid::dr3);

    dy.segment<2>(stid::dr1) =  r2mr1 + r3mr1;
    dy.segment<2>(stid::dr2) =  r3mr2 - r2mr1;
    dy.segment<2>(stid::dr3) = -r3mr1 - r3mr2;
}

Eigen::Matrix<precise_float, 3, 1>
Δ(const Eigen::Matrix<precise_float, 3, 1> &x) {
    auto bulirsch_stoer = BulirschStoer<12, 30, precise_float>{0.94};

    auto t  = precise_float{0.0};
    auto y0 = Ys{};
    auto dy = Ys{};

    y0(stid::x1) = -1;
    y0(stid::x2) = 1;
    y0(stid::x3) = 0;
    y0(stid::y1) = y0(stid::y2) = y0(stid::y3) = 0;

    y0(stid::dx1) = x(0);
    y0(stid::dy1) = x(1);
    y0(stid::dx2) = y0(stid::dx1);
    y0(stid::dy2) = y0(stid::dy1);
    y0(stid::dx3) = -2 * y0(stid::dx1);
    y0(stid::dy3) = -2 * y0(stid::dy1);

    auto y = Ys{y0};

    auto 𝛿t = precise_float{0.01};
    while (abs(x(2) - t) > precise_float{1e-99}) {
        auto remaining_time = x(2) - t;
        auto current_step   = (abs(𝛿t) < abs(remaining_time)) ? 𝛿t : remaining_time;
        derivs(y, dy);
        auto yscal = (abs(y) + abs(dy * current_step)).eval();
        auto [new_step, success] = bulirsch_stoer(y, dy, t, current_step, 1e-90, yscal, [&](auto, const auto &y, auto &dy) {
            derivs(y, dy);
        });
        if (success) {
            t += current_step;
        }
        𝛿t = new_step;
    }

    return {
        y(stid::x1)  - y0(stid::x1)
      , y(stid::y1)  - y0(stid::y1)
      , y(stid::dx1) - y0(stid::dx1)
      };
}

auto J(const Eigen::Matrix<precise_float, 3, 1> &x,
       const Eigen::Matrix<precise_float, 3, 1> &center)
{
    auto 𝛿 = precise_float{1e-50};
    auto res = Eigen::Matrix<precise_float, 3, 3>{};

    res.col(0) = (Δ(x + Eigen::Matrix<precise_float, 3, 1>{𝛿, 0, 0}) - center) / 𝛿;
    res.col(1) = (Δ(x + Eigen::Matrix<precise_float, 3, 1>{0, 𝛿, 0}) - center) / 𝛿;
    res.col(2) = (Δ(x + Eigen::Matrix<precise_float, 3, 1>{0, 0, 𝛿}) - center) / 𝛿;

    return res;
}

int main (int argc, char *argv[]) {
    // The following initial conditions correspond to a periodic solution of the
    // three-body problem known as "goggles". The parameters were taken from
    // "Three Classes of Newtonian Three-Body Planar Periodic Orbits" by Milovan
    // Šuvakov and V. Dmitrašinović (Phys. Rev. Lett. 110, 114301).
    //
    // Since the paper gave only 6 digits, these parameters were refined using
    // the same Bulirsch-Stoer integrator with multiprecision numbers.
    //
    // This orbit was chosen because it is highly unstable and accumulates
    // errors very quickly, which makes it a perfect test for the quality of an
    // ODE solver.

    // The refined parameters as the result of running this program are:
    // dx1: 0.08330007185049746989952757537419570164158552456772599092773952197771478216838449
    // dy1: 0.12788925551976670647803067663151709988133809414715842404997985790915880608904909
    //   T: 10.46484952592071984539928042851131704896726187045825559329773067489333993596154994

    auto dx1 = precise_float{0.08330};
    auto dy1 = precise_float{0.12789};
    auto T   = precise_float{10.4668};

    auto uk = Eigen::Matrix<precise_float, 3, 1>{dx1, dy1, T};
    for (auto iter = 0; iter < 15; ++iter) {
        auto residual = Δ(uk);
        auto error    = residual.norm();

        fmt::print("Iter {}: Error = {:>25}\n", iter, error.str(20, std::ios_base::scientific));

        if (error < precise_float{"1e-85"}) {
            fmt::print("Converged!\n");
            break;
        }

        auto jacob = J(uk, residual);
        uk = uk - jacob.inverse() * residual;

        fmt::print("   dx1: {}\n   dy1: {}\n     T: {}\n",
                   uk(0).str(80, std::ios_base::fixed),
                   uk(1).str(80, std::ios_base::fixed),
                   uk(2).str(80, std::ios_base::fixed));
    }
}
