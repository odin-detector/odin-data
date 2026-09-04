set(INTEGRATION_CONFIG_FILES
  dummyUDP.json
  dummyUDP-fp.json
  dummyUDP-fr.json
  testUDP.json
)

function(configure_integration_test_file source output prefix)
  set(CMAKE_INSTALL_PREFIX "${prefix}")
  configure_file("${source}" "${output}")
endfunction()

# Generate one set for tests run from the build tree and one for installation.
set(INSTALL_CONFIG_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/install")
foreach(config_file IN LISTS INTEGRATION_CONFIG_FILES)
  configure_integration_test_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/${config_file}"
    "${CMAKE_CONFIG_OUTPUT_DIRECTORY}/${config_file}"
    "${CMAKE_BINARY_DIR}"
  )
  configure_integration_test_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/${config_file}"
    "${INSTALL_CONFIG_OUTPUT_DIRECTORY}/${config_file}"
    "${CMAKE_INSTALL_PREFIX}"
  )
  list(APPEND INSTALL_CONFIG_FILES "${INSTALL_CONFIG_OUTPUT_DIRECTORY}/${config_file}")
endforeach()

install(FILES ${INSTALL_CONFIG_FILES} DESTINATION test_config)
