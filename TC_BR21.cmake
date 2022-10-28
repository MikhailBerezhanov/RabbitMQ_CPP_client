
###
# ~/src$ cd build_br21
# ~/src/build$ cmake -DCMAKE_TOOLCHAIN_FILE=../TC-BR21.cmake ..
#...
# CMAKE_TOOLCHAIN_FILE has to be specified only on the initial CMake run; 
# after that, the results are reused from the CMake cache. 
#
# On the second run the toolchain file for the build tree has already been 
# recorded in a persistent location and can't be changed.  Therefore the 
# -DCMAKE_TOOLCHAIN_FILE= command-line option on that run is ignored regardless 
# of its value.  The warning comes from a generic mechanism meant to report 
# unused -D options on the command line.
###


# the name of the target operating system
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR ARM)

set(BUILDROOT_DIR /home/mik/programs/buildroot_socsys/buildroot/output)

# which compilers to use for C and C++
set(CMAKE_C_COMPILER   ${BUILDROOT_DIR}/host/usr/bin/arm-buildroot-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER ${BUILDROOT_DIR}/host/usr/bin/arm-buildroot-linux-gnueabihf-g++)

# where is the target environment located
set(CMAKE_FIND_ROOT_PATH  
    ${BUILDROOT_DIR}/staging
)

# adjust the default behavior of the FIND_XXX() commands:
# search programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# search headers and libraries in the target environment
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
