#include <iostream>

#include <amqpcpp.h>
#include <amqpcpp/linux_tcp.h>

#include "utils.hpp"
#include "logger.hpp"
#include "my_handler.hpp"

using namespace std;

/*
	Tutorial #4: Routing

	In this tutorial we're going to add a feature to log system - 
	we're going to make it possible to subscribe only to a subset 
	of the messages. For example, we will be able to direct only 
	critical error messages to the log file (to save disk space), 
	while still being able to print all of the log messages on the console.
*/

int main(int argc, char* argv[])
{
	logger.init(MSG_DEBUG);

	// _severity_ is used as binding_key (routing_key)
	std::string severity = argc > 1 ? argv[1] : "info";
	std::string payload = argc > 2 ? utils::join(&argv[2], &argv[argc], " ") : "message";

	// address of the server
	AMQP::Address address("amqp://guest:guest@localhost/");

	// create a AMQP connection object
	MyTcpHandler myHandler;
	AMQP::TcpConnection connection(&myHandler, address);

	// and create a channel
	AMQP::TcpChannel channel(&connection);

	channel.onError([](const char* message)
	{
		 logger.msg(MSG_DEBUG, "Channel error: %s\n", message);
	});

	// use the channel object to call the AMQP method you like

	// Create direct exchange (routes message to binded queue 
	// with exactly matching routing_key)
	// ** to see excnahes use:
	//
	// sudo rabbitmqctl list_exchanges
	//
	channel.declareExchange("direct_logs", AMQP::direct)
		.onSuccess([&]()
		{
			// 	publish(exchange, rounting_key, message, flags)
			channel.publish("direct_logs", severity, payload);
			logger.msg(MSG_DEBUG, "[x] Sent %s:%s to direct_logs exchange\n", severity, payload);
		
			// Gentle closing
			channel.close().onFinalize([&](){
				myHandler.quit();
				connection.close();
			});
		}
	);

	myHandler.loop(&connection);
	
	return 0;
}