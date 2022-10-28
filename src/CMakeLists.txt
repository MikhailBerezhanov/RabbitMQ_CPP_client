# Build my_handler as static library to prevent 
# compiling for each tutorial module
add_library(myhandler my_handler.cpp my_handler.hpp)

target_link_libraries(myhandler amqpcpp logger)

# Tutorials' binaries
set(BINS	
    # Tutorial 1: 'Hello world' 
	send
    receive
    # Tutoril 2: Work Queues
    new_task
    worker
    # Tutorial 3: Publish/Subscribe
    emit_log
    receive_logs

    # Auto-reconnection test
    conn_restore

    # Tutorial 4:

    # Tutorial 5:   
)

foreach(item ${BINS})
    add_executable(${item} "${item}.cpp")
    target_link_libraries(${item} logger myhandler amqpcpp pthread dl ssl)    
endforeach(item)