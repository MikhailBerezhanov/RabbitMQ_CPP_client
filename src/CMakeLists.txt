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

	# Tutorial 4: Routing
	emit_log_direct
	receive_logs_direct

	# Tutorial 5: Topics  
	emit_log_topic
	receive_logs_topic

	# Tutorial 6: Remote Procedure call (RPC)
	rpc_client
	rpc_server

	# Tutorial 7: Publisher Confirms 

    # Demos with secure connection (TLS) support
	reconnection
	publish
)

foreach(item ${BINS})
	add_executable(${item} "${item}.cpp")
	target_link_libraries(${item} logger myhandler amqpcpp pthread dl ssl)    
endforeach(item)