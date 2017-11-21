set(TEST_SCRIPTS
  run_integration_test.py
)

foreach(test_script ${TEST_SCRIPTS})
	add_custom_command(
		OUTPUT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${test_script}"
		COMMAND ${CMAKE_COMMAND} -E copy
			"${CMAKE_CURRENT_SOURCE_DIR}/${test_script}"
			"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${test_script}"
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${test_script}"
	)
	list(APPEND TEST_SCRIPTS_DEST "${test_script}")
endforeach(test_script)

add_custom_target(CopyTestScripts ALL DEPENDS ${TEST_SCRIPTS_DEST} )



		

