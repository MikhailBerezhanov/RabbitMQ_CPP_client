#include <iostream>

#include <amqpcpp.h>
#include <amqpcpp/linux_tcp.h>

#include "utils.hpp"
#include "logger.hpp"
#include "my_handler.hpp"

using namespace std;

/*
	Tutorial #3: Publish/Subscribe

	We're going to build a simple logging system. It will consist of two programs 
	-- the first will emit log messages and
	-- the second will receive and print them.

	In our logging system every running copy of the receiver program will get the messages. 
	That way we'll be able to run one receiver and direct the logs to disk; and at the same 
	time we'll be able to run another receiver and see the logs on the screen.

	Essentially, published log messages are going to be broadcast to all the receivers.
*/

int main(int argc, char* argv[])
{
	logger.init(MSG_DEBUG);

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