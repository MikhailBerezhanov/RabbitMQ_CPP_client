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
	channel.onReady([]()
	{
		logger.msg(MSG_DEBUG, "Channel is ready\n");
	});

	// use the channel object to call the AMQP method you like

	// channel.declareExchange("hello-exchange", AMQP::fanout);
	// Use default exhange
	channel.declareQueue("hello");
	// channel.bindQueue("hello-exchange", "hello", "hello-routing-key");

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