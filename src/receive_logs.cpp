#include <iostream>
#include <string_view>

#include <amqpcpp.h>
#include <amqpcpp/linux_tcp.h>

#include "logger.hpp"
#include "my_handler.hpp"

using namespace std;

/*
	Tutorial #3: Publish/Subscribe
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


	auto reveive_callback = [](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered)
	{
		std::string_view body(message.body(), message.bodySize());
		logger.msg(MSG_DEBUG, " [x] Received '%s' (%lu bytes)\n", body.data(), message.bodySize());
	};

	// Create fanout exchange (routes message to every binded queue)
	// ** to see excnahes use:
	//
	// sudo rabbitmqctl list_exchanges
	// rabbitmqctl list_bindings
	//
	channel.declareExchange("logs", AMQP::fanout)
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
					channel.bindQueue("logs", name, name + "-routing-key");
					channel.consume(name, AMQP::noack).onReceived(reveive_callback);
				}
			);
		}
	);
	

	logger.msg(MSG_DEBUG, " [*] Waiting for messages. To exit press CTRL-C\n");
    myHandler.loop(&connection);
	
	return 0;
}