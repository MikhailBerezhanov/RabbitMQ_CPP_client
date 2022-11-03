#include <iostream>
#include <string>

#include <amqpcpp.h>
#include <amqpcpp/linux_tcp.h>

#include "logger.hpp"
#include "my_handler.hpp"

using namespace std;

/*
	Tutorial #6: Remote procedure call (RPC)

	In this tutorial we're going to use RabbitMQ to build an RPC system: 
	a client and a scalable RPC server. As we don't have any time-consuming 
	tasks that are worth distributing, we're going to create a dummy RPC service 
	that returns Fibonacci numbers.
*/

// Using tail-recursion with -O2 optimization
int fib(int n, int prev_sum = 0, int curr_sum = 1)
{
	if(n <= 0){
		return prev_sum;
	}

	return fib(n - 1, curr_sum, prev_sum + curr_sum);
}

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
		 logger.msg(MSG_ERROR, "Channel error: %s\n", message);
	});

	// A client sends a request message and a server replies with a response message. 
	// In order to receive a response the client needs to send a 'callback' queue 
	// address with the request. Using 
	// * _reply_to_ and    -  Commonly used to name a callback queue.
	// * _correlation_id_  -  Useful to correlate RPC responses with requests.
	// message properties. 

	AMQP::QueueCallback callback = [&](const std::string &name, int msgcount, int consumercount){

		// Waiting for requests		
		channel.consume("rpc_queue").onReceived(
			[&channel](const AMQP::Message &message,
				uint64_t deliveryTag,
				bool redelivered)
			{
				std::string body(message.body(), message.bodySize());
				// logger.msg(MSG_DEBUG, "[x] Sent '%s' as response\n", res);

				std::string res = std::to_string( fib(std::stoi(body)) );
				AMQP::Envelope response(res.c_str(), res.size());

				// Set correlation_id property 
				response.setCorrelationID(message.correlationID());
				
				// Sending response to callback queue using default exchange "" (direct)
				const std::string &callback_queue = message.replyTo();
				channel.publish("", callback_queue, response);

				// Message fully processed and can be acked to be removed from the queue.
				channel.ack(deliveryTag);

				logger.msg(MSG_DEBUG, "[x] Sent '%s' as response to '%s' callback queue\n", res, callback_queue);
			}
		);

	};

	// Setting prefetchCount = 1 
	// We might want to run more than one server process. In order to spread 
	// the load equally over multiple servers we need to set the prefetch_count setting.
	channel.setQos(1);	

	channel.onReady([&]()
	{
		logger.msg(MSG_DEBUG, "Channel is ready, declaring rpc_queue\n");
		channel.declareQueue("rpc_queue").onSuccess(callback);
	});

	myHandler.loop(&connection);	
	return 0;
}