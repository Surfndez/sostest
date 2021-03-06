cmake_minimum_required (VERSION 3.6)

if( ${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Darwin" )
  set(SOS_TOOLCHAIN_CMAKE_PATH /Applications/StratifyLabs-SDK/Tools/gcc/arm-none-eabi/cmake)
elseif( ${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows" )
	set(SOS_TOOLCHAIN_CMAKE_PATH C:/StratifyLabs-SDK/Tools/gcc/arm-none-eabi/cmake)
else()
  message(FATAL_ERROR "Cross compiling is not yet supportd on: " ${CMAKE_HOST_SYSTEM_NAME})
endif()

include(${SOS_TOOLCHAIN_CMAKE_PATH}/sos-variables.cmake)
include(${SOS_TOOLCHAIN_CMAKE_PATH}/sos-sdk.cmake)
set(CMAKE_TOOLCHAIN_FILE ${SOS_TOOLCHAIN_CMAKE_PATH}/sos-app-toolchain.cmake)

get_filename_component(SOS_NAME ${CMAKE_SOURCE_DIR} NAME)
project(${SOS_NAME} CXX C)

# This will set the default RAM used by the application
set(SOS_RAM_SIZE 65536)

#Add sources to the project
sos_sdk_add_subdirectory(SOS_SOURCELIST src)
list(APPEND SOS_SOURCELIST ${CMAKE_SOURCE_DIR}/sl_settings.json)

include(${SOS_TOOLCHAIN_CMAKE_PATH}/sos-app-std.cmake)
