function(add_code_coverage _target)
  # Both CODE_COVERAGE being defined and set to 1 is required
  if(DEFINED CODE_COVERAGE AND CODE_COVERAGE)
    target_compile_options(${_target} PRIVATE -O0 -g3 --coverage)
    target_link_options(${_target} PRIVATE -O0 -g3 --coverage)
    message(STATUS "Added code coverage for target ${_target}")
  endif(DEFINED CODE_COVERAGE AND CODE_COVERAGE)
endfunction()
