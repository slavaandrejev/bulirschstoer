#  Copyright (c) 2026, Slava Andrejev
#
#  Permission is hereby granted, free of charge, to any person obtaining a copy
#  of this software and associated documentation files (the "Software"), to deal
#  in the Software without restriction, including without limitation the rights
#  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#  copies of the Software, and to permit persons to whom the Software is
#  furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included in
#  all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
#  SOFTWARE.
#
#  --- Optional exception to the license ---
#
#  As an exception, if, as a result of your compiling your source code, portions
#  of this Software are embedded into a machine-executable object form of such
#  source code, you may redistribute such embedded portions in such object form
#  without including the above copyright and permission notices.

project(
    'bulirschstoer'
  , ['c', 'cpp']
  , version : '1.0'
  , meson_version: '>= 1.2'
  , default_options: {
        'cpp_std': 'gnu++23'
      , 'c_std':   'gnu18'
      }
  )

project_inc = include_directories('include')

if get_option('buildtype') == ('debug')
    add_project_arguments('-DDEBUG', language: ['c', 'cpp'])
endif
if get_option('buildtype') == 'release'
    add_project_arguments('-DNDEBUG', language: ['c', 'cpp'])
endif

eigen_dep = dependency('Eigen3', method: 'cmake', modules: 'Eigen3::Eigen')
range_dep = dependency('range-v3', method: 'cmake')

if get_option('three-body-animation') or get_option('benchmarks') or get_option('three-body-animation')
    boost_dep = dependency('boost')
    fmt_dep   = dependency('fmt')
    gsl_dep   = dependency('gsl')
    mpfr_dep  = dependency('mpfr')
endif

if get_option('three-body-animation')
    subdir('three-body-animation')
    subdir('refinegoggles')
endif
if get_option('benchmarks')
    subdir('benchmarks')
endif
if get_option('tests')
    subdir('tests')
endif

install_headers('include' / 'bulirschstoer.h', subdir: 'bulirschstoer')

pkg = import('pkgconfig')

pkg.generate(
    name: 'bulirschstoer'
  , description: 'Rational Bulirsch-Stoer ODE integrator'
  , version: meson.project_version()
  , subdirs: 'bulirschstoer'
  )