#include <iostream>

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
	logger.init(MSG_DEBUG);

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

	AMQP::QueueCallback callback = [&](const std::string &name, int msgcount, int consumercount){
		// 			(exchange, rounting_key, body, flags)
		channel.publish("", "hello", "Hello World!");
		logger.msg(MSG_DEBUG, "[x] Sent '%s' to 'hello' queue\n", "Hello World!");
		
		// Gentle closing
		channel.close().onFinalize([&](){
			myHandler.quit();
			connection.close();
		});
		
	};

	channel.onReady([&]()
	{
		logger.msg(MSG_DEBUG, "Channel is ready\n");
		channel.declareQueue("hello").onSuccess(callback);
	});

	// use the channel object to call the AMQP method you like

	// channel.declareExchange("hello-exchange", AMQP::fanout);
	// Use default exhange
	
	// channel.bindQueue("hello-exchange", "hello", "hello-routing-key");

	myHandler.loop(&connection);
	
	return 0;
}