set(PYTHON_TOOL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/python")
set(PYTHON_TOOL_MODULE_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/python")

add_custom_target(CopyPythonToolModules ALL
	COMMAND cmake -E copy_directory ${PYTHON_TOOL_SOURCE_DIR} ${PYTHON_TOOL_MODULE_DIR}
)

IF (NOT HDF5_FOUND)
    message(WARNING "Did not find HDF5. Not building file writer application")
ELSE (NOT HDF5_FOUND)
    add_subdirectory(client)
ENDIF (NOT HDF5_FOUND)
