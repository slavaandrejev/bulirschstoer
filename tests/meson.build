testsuite = executable(
    'testsuite'
  , 'testsuite.cpp'
  , 'bulirschstoer.cpp'
  , include_directories: project_inc
  , dependencies: [
        boost_dep
      , eigen_dep
      , mpfr_dep
      ]
)

test('Tests for Bulirsch and Stoer algorithm', testsuite)
