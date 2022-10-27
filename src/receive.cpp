#include <iostream>
#include <string_view>

#include <amqpcpp.h>
#include <amqpcpp/linux_tcp.h>

#include "logger.hpp"
#include "my_handler.hpp"

using namespace std;

/*
	Tutorial #1: Hello world
*/

int main(int argc, char* argv[])
{
	logger.init(MSG_TRACE);

	// address of the server
	std::string addr = argc > 1 ? argv[1] : "amqp://guest:guest@localhost/";

	AMQP::Address address(addr);

	// create a AMQP connection object
	MyTcpHandler myHandler;
	logger.msg(MSG_DEBUG, "Connecting to '%s'\n", addr);
	AMQP::TcpConnection connection(&myHandler, address);

	// and create a channel
	AMQP::TcpChannel channel(&connection);

	channel.onError([](const char* message)
	{
	    logger.msg(MSG_DEBUG, "Channel error: %s\n", message);
	});

	channel.onReady([]()
	{
		logger.msg(MSG_DEBUG, "Channel is ready\n");
	});

	// use the channel object to call the AMQP method you like

	// Use default exhange ("", direct)
	channel.declareQueue("hello");

	// noack	- 	if set, consumed messages do not have to be acked, this happens automatically
	// Server will see that the message was acked and can delete it from the queue.
	channel.consume("hello", AMQP::noack)
		.onReceived([](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered)
		{
			std::string_view body(message.body(), message.bodySize());
		    logger.msg(MSG_DEBUG, " [x] Received '%s' (%lu bytes)\n", body.data(), message.bodySize());
		}
	);

	logger.msg(MSG_DEBUG, " [*] Waiting for messages. To exit press CTRL-C\n");
    myHandler.loop(&connection);
	
	return 0;
}