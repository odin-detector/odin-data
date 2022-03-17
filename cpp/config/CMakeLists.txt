file(GLOB LOG_CONFIGS ${CMAKE_CURRENT_SOURCE_DIR}/*log4cxx.xml)
install(PROGRAMS ${LOG_CONFIGS} DESTINATION test_config)

add_subdirectory(client_msgs)
