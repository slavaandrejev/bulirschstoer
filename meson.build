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

if get_option('tests') or get_option('benchmarks') or get_option('three-body-animation')
    fmt_dep   = dependency('fmt')
    if get_option('benchmarks') or get_option('tests')
        boost_dep = dependency('boost')
        mpfr_dep  = dependency('mpfr')
    endif
    if get_option('benchmarks')
        gsl_dep   = dependency('gsl')
    endif
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