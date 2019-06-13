#
# Finds the Kafka C client library. This module defines:
#   - KAFKA_INCLUDE_DIR, directory containing headers
#   - KAFKA_LIBRARIES, the Kafka library path
#   - KAFKA_FOUND, whether Kafka has been found
# Define KAFKA_ROOT_DIR  if librdkafka is installed in a non-standard location.

message ("\nLooking for kafka headers and libraries")


if (KAFKA_ROOT_DIR)
    message (STATUS "Kafka Root Dir: ${KAFKA_ROOT_DIR}")
endif ()

# Find header files
if(KAFKA_ROOT_DIR)
    find_path(
        KAFKA_INCLUDE_DIR librdkafka
        PATHS ${KAFKA_ROOT_DIR}/include
        NO_DEFAULT_PATH
    )
else()
    find_path(KAFKA_INCLUDE_DIR librdkafka)
endif()

# Find library
if(KAFKA_ROOT_DIR)
    find_library(
        KAFKA_LIBRARIES NAMES rdkafka
        PATHS ${KAFKA_ROOT_DIR}/lib
        NO_DEFAULT_PATH
    )
else()
    find_library(KAFKA_LIBRARIES NAMES rdkafka)
endif()

if(KAFKA_INCLUDE_DIR AND KAFKA_LIBRARIES)
    message(STATUS "Found Kafka: ${KAFKA_LIBRARIES}")
    set(KAFKA_FOUND TRUE)
else()
    set(KAFKA_FOUND FALSE)
endif()

if(NOT KAFKA_FOUND)
    message(STATUS "Could not find the Kafka library.")
endif()

