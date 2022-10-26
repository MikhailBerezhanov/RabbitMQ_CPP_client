#include <iostream>

#include <amqpcpp.h>
#include <amqpcpp/linux_tcp.h>

#include "utils.hpp"
#include "logger.hpp"
#include "my_handler.hpp"

using namespace std;

/*
	Tutorial #3: Publish/Subscribe
*/

int main(int argc, char* argv[])
{
	logger.init(MSG_TRACE);

	std::string payload = argc > 1 ? utils::join(&argv[1], &argv[argc], " ") : "payload";

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

	// Create fanout exchange (routes message to every binded queue)
	// ** to see excnahes use:
	//
	// sudo rabbitmqctl list_exchanges
	//
	channel.declareExchange("logs", AMQP::fanout)
		.onSuccess([&]()
		{
			// 	publish(exchange, rounting_key, message, flags)
			channel.publish("logs", "", payload);
			logger.msg(MSG_DEBUG, "[x] Sent '%s' to logs exchange\n", payload);
		
			myHandler.quit();
			channel.close();
		}
	);

	myHandler.loop(&connection);
	
	return 0;
}