file(GLOB CLIENT_MSG_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.json)

set(CMAKE_CLIENT_MSG_OUTPUT_DIRECTORY ${CMAKE_CONFIG_OUTPUT_DIRECTORY}/client_msgs)

foreach(client_msg_file ${CLIENT_MSG_FILES})
  add_custom_command(
    OUTPUT "${CMAKE_CLIENT_MSG_OUTPUT_DIRECTORY}/${client_msg_file}"
    COMMAND ${CMAKE_COMMAND} -E copy
      "${CMAKE_CURRENT_SOURCE_DIR}/${client_msg_file}"
      "${CMAKE_CLIENT_MSG_OUTPUT_DIRECTORY}/${client_msg_file}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${client_msg_file}"
  )
  list(APPEND CLIENT_MSGS_DEST "${CMAKE_CLIENT_MSG_OUTPUT_DIRECTORY}/${client_msg_file}")
endforeach(client_msg_file)

add_custom_target(CopyClientMsgFiles ALL DEPENDS ${CLIENT_MSGS_DEST})
