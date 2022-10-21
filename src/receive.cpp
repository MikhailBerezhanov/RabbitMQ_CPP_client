#include <iostream>

#include <amqpcpp.h>
#include <amqpcpp/linux_tcp.h>

#include "my_handler.hpp"

using namespace std;



int main(int argc, char* argv[])
{
	// address of the server
	AMQP::Address address("amqp://guest:guest@localhost/");

	// create a AMQP connection object
	MyTcpHandler myHandler;
	AMQP::TcpConnection connection(&myHandler, address);

	// and create a channel
	AMQP::TcpChannel channel(&connection);

	channel.onError([](const char* message)
	{
	    cout << "Channel error: " << message << endl;
	});
	channel.onReady([&]()
	{
		cout << "Channel is ready" << endl;
	});

	// use the channel object to call the AMQP method you like

	// channel.declareExchange("hello-exchange", AMQP::fanout);
	// Use default exhange
	channel.declareQueue("hello");
	// channel.bindQueue("hello-exchange", "hello", "hello-routing-key");

	// noack	- 	if set, consumed messages do not have to be acked, this happens automatically
	// Server will see that the message was acked and can delete it from the queue.
	channel.consume("hello", AMQP::noack).onReceived(
			[](const AMQP::Message &message,
				uint64_t deliveryTag,
				bool redelivered)
			{
			    std::cout <<" [x] Received (" << message.bodySize() << " bytes)" << message.body() << std::endl;
			}
	);

	std::cout << " [*] Waiting for messages. To exit press CTRL-C\n";
    myHandler.loop(&connection);
	
	return 0;
}