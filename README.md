# Bulirsch-Stoer ODE Integrator

A modern, high-precision header-only C++ implementation of the Bulirsch-Stoer
algorithm for solving ordinary differential equations with adaptive step and
order control.

The core engine uses rational extrapolation with polynomial fallback when the
rational extrapolation becomes divergent. The step and order control follows the
"Control of step size and order in extrapolation codes" paper by L. F. Shampine
(Journal of Computational and Applied Mathematics 18 (1987) 3-16).

This implementation performs on par with the state-of-the-art GNU Scientific Library
(GSL) 8th-order Runge-Kutta (RK8) solver. In orbital mechanics tests, it matches
the precision while requiring fewer right-hand-side function evaluations. In the
same test, the Boost implementation of Bulirsch-Stoer hits the wall at two
orders of magnitude higher error. Below is a benchmark result of calculating a
three-body unstable periodic orbit. The benchmark measured the accumulated error
after one full period.
```text
Bulirsch-Stoer error = 7.327680181580088e-12, derivative evaluations = 43801
GSL Error = 5.7638839141124744e-12, derivative evaluations = 49596
Boost Bulirsch-Stoer error = 2.884019502824533e-10, derivative evaluations = 31961
```

## Basic usage

```cpp
#include <bulirschstoer.h>

// ...

// Logistic equation:  y' = y(1 - y)
auto rhs = [](double, double y, double &dy) {
    dy = y * (1.0 - y);
};

auto bulirsch_stoer = BulirschStoer<1, 12>{};

const auto target_time = 10.0;

auto t  = 0.0;
auto y  = 0.5;
auto dy = 0.0;
auto 𝛿t = 0.5;
while (std::abs(target_time - t) > 1e-14 * target_time) {
    auto remaining_time = target_time - t;
    auto current_step   = (abs(𝛿t) < abs(remaining_time)) ? 𝛿t : remaining_time;
    rhs(0, y, dy);
    auto yscal = std::abs(y) + std::abs(dy * current_step);
    auto [new_step, success] = bulirsch_stoer(y, dy, t, current_step, 1e-12, yscal, rhs);
    if (success) {
        t += current_step;
    }
    𝛿t = new_step;
}
```

## Multiprecision Integration

Because the integrator is templated, you can trivially drop in
`Boost.Multiprecision` types to solve equations to arbitrary precision. The
adaptive order control will increase the order automatically, allowing you to
integrate with relatively big strides and not spend the entire age of the
Universe on integrating with infinitesimally small steps. For example, the
following calculation of the Euler number with 88 digits after the decimal point
requires just 20 integration steps:
```cpp
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
printf("%s\n", y.str(88, std::ios_base::fixed).c_str());
```
The output:
```text
2.7182818284590452353602874713526624977572470936999595749669676277240766303535475945713822
```

## Build & Installation

Because this is a header-only implementation, no compilation is required. To
install:
```bash
meson setup build
meson install -C build
```
To consume the library in your own Meson project, simply add:
```meson
bs_dep = dependency('bulirschstoer')
```
