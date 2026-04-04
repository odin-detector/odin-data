# Control of code coverage is via two switches
# - CODE_COVERAGE_ALL == 1 : this switches coverage on for all compilations
# - CODE_COVERAGE_TARGET == 1 : this switches coverage on only if add_code_coverage has been used

function(setup_coverage)
  # Check only one of the code coverage switches has been set
  if(DEFINED CODE_COVERAGE_ALL AND DEFINED CODE_COVERAGE_TARGET)
    message(FATAL_ERROR "Both CODE_COVERAGE_ALL and CODE_COVERAGE_TARGET have been defined")
  endif()

  if(DEFINED CODE_COVERAGE_ALL AND CODE_COVERAGE_ALL)
    add_compile_options(-O0 -g3 --coverage)
    # The -Wl,--dynamic-list-data is required if dlopen is to be used in the coverage
    # See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=83879
    add_link_options(-O0 -g3 --coverage -Wl,--dynamic-list-data)
    message(STATUS "CODE_COVERAGE_ALL: Global code coverage switch on")
  endif()
endfunction()

function(add_code_coverage _target)
  # Both CODE_COVERAGE being defined and set to 1 is required
  if(DEFINED CODE_COVERAGE_TARGET AND CODE_COVERAGE_TARGET)
    target_compile_options(${_target} PRIVATE -O0 -g3 --coverage)
    # The -Wl,--dynamic-list-data is required if dlopen is to be used in the coverage
    # See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=83879
    target_link_options(${_target} PRIVATE -O0 -g3 --coverage -Wl,--dynamic-list-data)
    message(STATUS "CODE_COVERAGE_TARGET: Added code coverage for target ${_target}")
  endif(DEFINED CODE_COVERAGE_TARGET AND CODE_COVERAGE_TARGET)
endfunction()
