#include <iostream>
#include <string_view>

#include <amqpcpp.h>
#include <amqpcpp/linux_tcp.h>

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

	std::vector<std::string> severities;

	for(int i = 1; i < argc; ++i){
		severities.push_back(argv[i]);
	}

	if(severities.empty()){
		std::cerr << "Usage: " << argv[0] << " [info] [warning] [error]" << std::endl;
		return 0;
	}

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


	auto reveive_callback = [](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered)
	{
		std::string_view body(message.body(), message.bodySize());
		logger.msg(MSG_DEBUG, " [x] Received %s:%s\n", message.routingkey(), body.data());
	};

	// Create direct exchange (routes message to binded queue 
	// with exactly matching routing_key)
	// ** to see excnahes use:
	//
	// sudo rabbitmqctl list_exchanges
	// rabbitmqctl list_bindings
	//
	channel.declareExchange("direct_logs", AMQP::direct)
		.onSuccess([&]()
		{
			// Use randomly named-by-server queue. Make it exclusive (the queue only exists 
			// for this connection, and is automatically removed when connection is gone)
			channel.declareQueue("", AMQP::exclusive)
				.onSuccess([&](const std::string &name, uint32_t messagecount, uint32_t consumercoun)
				{
					// name contains a random queue name. 
					// For example it may look like amq.gen-JzTY20BRgKO-HjmUJj0wLg
					logger.msg(MSG_DEBUG, "Generated queue name: %s\n", name);

					for(const auto &severity : severities){
						// A binding is a relationship between an exchange and a queue. 
						// This can be simply read as: the queue is interested in messages 
						// from this exchange.
						channel.bindQueue("direct_logs", name, severity);
					}
					
					channel.consume(name, AMQP::noack).onReceived(reveive_callback);
				}
			);
		}
	);
	

	logger.msg(MSG_DEBUG, " [*] Waiting for messages. To exit press CTRL-C\n");
    myHandler.loop(&connection);
	
	return 0;
}