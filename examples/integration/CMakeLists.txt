cmake_minimum_required(VERSION 3.16)
project(kymo_integration LANGUAGES CXX)

get_filename_component(KYMO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
file(GLOB_RECURSE KYMO_CORE_SOURCES CONFIGURE_DEPENDS "${KYMO_ROOT}/src/*.cpp")

# One shared build configuration reaches BOTH the example and library sources.
add_library(example_config INTERFACE)
target_include_directories(example_config INTERFACE
    "${CMAKE_CURRENT_LIST_DIR}" "${KYMO_ROOT}/src")
target_compile_definitions(example_config INTERFACE
    KYMO_USER_CONFIG_HEADER="kymo_user_config.h")
target_compile_features(example_config INTERFACE cxx_std_11)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(example_config INTERFACE
        -Wall -Wextra -Werror -pedantic -fno-exceptions -fno-rtti)
endif()

add_library(kymo_core STATIC ${KYMO_CORE_SOURCES})
target_link_libraries(kymo_core PUBLIC example_config)
add_executable(kymo_integration main_native.cpp kymo_app.cpp
    kymo_bindings.cpp kymo_port_native.cpp)
target_link_libraries(kymo_integration PRIVATE kymo_core)
