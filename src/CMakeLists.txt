# use my_handler as static library to prevent 
# compiling for each tutorial module
add_library(myhandler my_handler.cpp my_handler.hpp)
target_link_libraries(myhandler amqpcpp)


# tutorials' binaries
set(BINS	
		produce
        consume
)

foreach(item ${BINS})
    add_executable(${item} "${item}.cpp")
    target_link_libraries(${item} myhandler amqpcpp pthread dl)    
endforeach(item)