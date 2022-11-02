cmake_minimum_required(VERSION 3.4)

project(RabbitMQ-tutorial)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Options section 
add_compile_options(
	-O2
	-Wall 
)

# Check target system
if(CMAKE_SYSTEM_PROCESSOR MATCHES ARM)
   message(STATUS "Target system processor is ARM")
endif()

# If you don't have installed library enable it (static version is built by default)
option(BUILD_AMQPCPP "Build AMQP-CPP library or use already installed (shared) version" OFF)


# Build Logger module
add_subdirectory(src/logger/cpp_src)

if(BUILD_AMQPCPP)
	add_subdirectory(3rd_party/AMQP-CPP-4.3.15)
endif()

# Build tutorials 
add_subdirectory(src)