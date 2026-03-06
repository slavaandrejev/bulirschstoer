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
#include <memory>

#include <boost/numeric/odeint/stepper/bulirsch_stoer.hpp>

#include <Eigen/Dense>

#include <fmt/format.h>

#include <gsl/gsl_errno.h>
#include <gsl/gsl_odeiv2.h>

#include <range/v3/algorithm/copy.hpp>

#include <bulirschstoer.h>

using Ys = Eigen::Array<double, 12, 1>;

namespace rs = ranges;

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

auto cube(auto x) {
    return x * x * x;
}

template <typename Derived1, typename Derived2>
void derivs(const Eigen::ArrayBase<Derived1> &y, Eigen::ArrayBase<Derived2> &dy) {
    auto r1 = y.template segment<2>(stid::r1);
    auto r2 = y.template segment<2>(stid::r2);
    auto r3 = y.template segment<2>(stid::r3);

    auto r2mr1 = (r2 - r1) / cube((r2 - r1).matrix().norm());
    auto r3mr1 = (r3 - r1) / cube((r3 - r1).matrix().norm());
    auto r3mr2 = (r3 - r2) / cube((r3 - r2).matrix().norm());

    dy.template segment<2>(stid::r1) = y.template segment<2>(stid::dr1);
    dy.template segment<2>(stid::r2) = y.template segment<2>(stid::dr2);
    dy.template segment<2>(stid::r3) = y.template segment<2>(stid::dr3);

    dy.template segment<2>(stid::dr1) =  r2mr1 + r3mr1;
    dy.template segment<2>(stid::dr2) =  r3mr2 - r2mr1;
    dy.template segment<2>(stid::dr3) = -r3mr1 - r3mr2;
}

int gsl_derivs_wrapper(double t, const double y[], double dydt[], void *params) {
    int *evals = static_cast<int *>(params);
    (*evals)++;

    Eigen::Map<const Ys> y_map(y);
    Eigen::Map<Ys> dy_map(dydt);

    derivs(y_map, dy_map);

    return GSL_SUCCESS;
}

namespace boost { namespace numeric { namespace odeint {
    template <>
    struct vector_space_norm_inf<Ys>
    {
        typedef double result_type;
        result_type operator()(const Ys &m) const {
            return m.abs().maxCoeff();
        }
    };
} } }

template <>
struct boost::numeric::odeint::algebra_dispatcher<Ys> {
    typedef boost::numeric::odeint::vector_space_algebra algebra_type;
};

int main (int argc, char *argv[]) {
    auto bulirsch_stoer = BulirschStoer<12, 12>{};

    auto constexpr dx1 = 0.0833000718504974698995276;
    auto constexpr dy1 = 0.1278892555197667064780307;
    auto constexpr T   = 10.464849525920719845399;

    auto y0 = Ys{};

    using std::abs;

    y0(stid::x1) = -1;
    y0(stid::x2) = 1;
    y0(stid::x3) = 0;
    y0(stid::y1) = y0(stid::y2) = y0(stid::y3) = 0;

    y0(stid::dx1) = dx1;
    y0(stid::dy1) = dy1;
    y0(stid::dx2) = y0(stid::dx1);
    y0(stid::dy2) = y0(stid::dy1);
    y0(stid::dx3) = -2 * y0(stid::dx1);
    y0(stid::dy3) = -2 * y0(stid::dy1);

    {
        auto t       = 0.0;
        auto y       = Ys{y0};
        auto dy      = Ys{};
        auto 𝛿t      = 0.01;
        auto n_evals = 0;
        while (abs(T - t) > 1e-14 * T) {
            auto remaining_time = T - t;
            auto current_step   = (abs(𝛿t) < abs(remaining_time)) ? 𝛿t : remaining_time;
            derivs(y, dy); ++n_evals;
            auto yscal = (abs(y) + 1e-14).eval();
            auto [new_step, success] = bulirsch_stoer(y, dy, t, current_step, 1e-14 / 1.3, yscal, [&](auto t, const auto &y, auto &dy) {
                derivs(y, dy); ++n_evals;
            });
            if (success) {
                t += current_step;
            }
            𝛿t = new_step;
        }

        fmt::print("Bulirsch-Stoer error = {}, derivative evaluations = {}\n", (y - y0).matrix().norm(), n_evals);
    }

    {
        auto t       = 0.0;
        auto y       = std::array<double, 12>{};
        auto 𝛿t      = 0.01;
        auto n_evals = 0;

        auto sys = gsl_odeiv2_system{gsl_derivs_wrapper, nullptr, 12, &n_evals};

        auto step = std::unique_ptr<gsl_odeiv2_step, decltype(&gsl_odeiv2_step_free)>{
            gsl_odeiv2_step_alloc(gsl_odeiv2_step_rk8pd, 12)
          , &gsl_odeiv2_step_free};
        auto con = std::unique_ptr<gsl_odeiv2_control, decltype(&gsl_odeiv2_control_free)>{
            gsl_odeiv2_control_y_new(1e-14, 1e-14)
          , &gsl_odeiv2_control_free};
        auto evolve = std::unique_ptr<gsl_odeiv2_evolve, decltype(&gsl_odeiv2_evolve_free)>{
            gsl_odeiv2_evolve_alloc(12)
          , &gsl_odeiv2_evolve_free};

        rs::copy(y0, &y[0]);

        while (abs(T - t) > 1e-14 * T) {
            auto status = gsl_odeiv2_evolve_apply(evolve.get(), con.get(), step.get(), &sys, &t, T, &𝛿t, &y[0]);
            if (GSL_SUCCESS != status) {
                fmt::print("GSL failed with status: {}\n", status);
                break;
            }
        }

        auto y_mapped = Eigen::Map<Ys>(&y[0]);
        fmt::print("GSL Error = {}, derivative evaluations = {}\n", (y_mapped - y0).matrix().norm(), n_evals);
    }

    {
        auto bs = boost::numeric::odeint::bulirsch_stoer<Ys>(1e-14, 1e-14);

        auto t       = 0.0;
        auto y       = Ys{y0};
        auto dy      = Ys{};
        auto yout    = Ys{};
        auto 𝛿t      = 0.01;
        auto n_evals = 0;

        while (abs(T - t) > 1e-14 * T) {
            auto remaining_time = T - t;
            𝛿t = (abs(𝛿t) < abs(remaining_time)) ? 𝛿t : remaining_time;
            derivs(y, dy); ++n_evals;
            auto result = bs.try_step([&](auto &&y, auto &&dy, auto) {
                derivs(y, dy); ++n_evals;
            }, y, dy, t, yout, 𝛿t);
            if (boost::numeric::odeint::success == result) {
                y = yout;
            }
        }

        fmt::print("Boost Bulirsch-Stoer error = {}, derivative evaluations = {}\n", (y - y0).matrix().norm(), n_evals);
    }
}
