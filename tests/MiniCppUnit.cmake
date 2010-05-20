enable_language(CXX)

set(MINI_CPP_UNIT_FILES TestsRunner.cxx MiniCppUnit.cxx MiniCppUnit.hxx)

macro(ADD_MINI_CPP_UNIT_TEST test)
  add_executable(${test} ${test}.cxx ${MINI_CPP_UNIT_FILES} ${ARGN})
  add_test(${test} ${test})
endmacro(ADD_MINI_CPP_UNIT_TEST)
