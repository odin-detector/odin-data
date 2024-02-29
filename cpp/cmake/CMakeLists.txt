file(GLOB CMAKE_FILES *.cmake *.cmake.in)

install(FILES ${CMAKE_FILES} DESTINATION cmake)
