#include <iostream>

extern "C"{
#include <openssl/ssl.h>
}

#include <amqpcpp.h>
#include <amqpcpp/linux_tcp.h>

#include "utils.hpp"
#include "logger.hpp"
#include "my_handler.hpp"

using namespace std;

/*
	Secure publish demo
*/


int main(int argc, char* argv[])
{
	logger.init(MSG_DEBUG);

	OPENSSL_init_ssl(0, nullptr);

	// 'Heavy' message to be processed by workers 
	std::string payload = argc > 1 ? utils::join(&argv[1], &argv[argc], " ") : "Test message";

	// address of the server
	// AMQP::Address address("amqps://guest:guest@localhost/");
	AMQP::Address address("amqp://guest:guest@localhost/");

	logger.msg(MSG_DEBUG, "Connecting to '%s'\n", std::string(address));

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
		channel.publish("", "hello", payload);
		logger.msg(MSG_DEBUG, "[x] Sent '%s' to 'hello' queue\n", payload);
		
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

	myHandler.loop(&connection);
	
	return 0;
}