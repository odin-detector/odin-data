set(TEST_CONFIGS
  fp_log4cxx.xml
  fr_log4cxx.xml
)

foreach(test_config ${TEST_CONFIGS})
	add_custom_command(
		OUTPUT "${CMAKE_CONFIG_OUTPUT_DIRECTORY}/${test_config}"
		COMMAND ${CMAKE_COMMAND} -E copy
			"${CMAKE_CURRENT_SOURCE_DIR}/${test_config}"
			"${CMAKE_CONFIG_OUTPUT_DIRECTORY}/${test_config}"
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${test_config}"
	)
	list(APPEND TEST_CONFIGS_DEST "${CMAKE_CONFIG_OUTPUT_DIRECTORY}/${test_config}")
endforeach(test_config)

add_custom_target(CopyTestConfigs ALL DEPENDS ${TEST_CONFIGS_DEST} )

add_subdirectory(client_msgs)
