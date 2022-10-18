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

		channel.publish("", "hello", "Hello World!");
		std::cout << " [x] Sent 'Hello World!'" << std::endl;
		channel.close();
	});

	// use the channel object to call the AMQP method you like

	channel.declareExchange("hello-exchange", AMQP::fanout);
	channel.declareQueue("hello");
	channel.bindQueue("hello-exchange", "hello", "hello-routing-key");

    myHandler.loop(&connection);
	
	return 0;
}