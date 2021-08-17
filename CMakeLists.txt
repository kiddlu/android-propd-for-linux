cmake_minimum_required(VERSION 2.8)
project (propd)

add_definitions (
    -Os
)

include_directories (${PROJECT_SOURCE_DIR}/client)
include_directories (${PROJECT_SOURCE_DIR}/cserver)

#static link
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

add_executable(setprop  ${PROJECT_SOURCE_DIR}/client/setprop.c ${PROJECT_SOURCE_DIR}/client/properties.c)
target_link_libraries(setprop)

add_executable(getprop  ${PROJECT_SOURCE_DIR}/client/getprop.c ${PROJECT_SOURCE_DIR}/client/properties.c)
target_link_libraries(getprop)

add_executable(service  ${PROJECT_SOURCE_DIR}/client/service.c ${PROJECT_SOURCE_DIR}/client/properties.c)
target_link_libraries(service)

add_executable(propd  ${PROJECT_SOURCE_DIR}/cserver/propd.c ${PROJECT_SOURCE_DIR}/cserver/main.c)
target_link_libraries(propd)

include(cross.cmake)
