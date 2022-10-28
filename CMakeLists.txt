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


# Build Logger module
add_subdirectory(src/logger/cpp_src)

# Build tutorials 
add_subdirectory(src)