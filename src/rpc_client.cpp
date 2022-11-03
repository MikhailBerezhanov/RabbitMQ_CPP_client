#include <iostream>
#include <string>
#include <memory>

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

class FibonacciRpcClient
{
public:

	FibonacciRpcClient()
	{
		_connection_uptr = std::make_unique<AMQP::TcpConnection>(&_myHandler, _address);
		_channel_uptr = std::make_unique<AMQP::TcpChannel>(_connection_uptr.get());

		_channel_uptr->onError([](const char* message)
		{
			 logger.msg(MSG_ERROR, "Channel error: %s\n", message);
		});


		_declare_callback = [&](const std::string &name, int msgcount, int consumercount)
		{
			_callback_queue = name;

			this->request(30);
			
			// start consuming responses (autoack)
			_channel_uptr->consume(name, AMQP::noack).onReceived( 
				[this](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered)
				{
					this->on_response(message, deliveryTag, redelivered);
				}
			);
		};

		_channel_uptr->onReady([&]()
		{
			// generate queue on server 
			_channel_uptr->declareQueue("", AMQP::exclusive).onSuccess(_declare_callback);
		});

		
	}


	int call(int n)
	{
		// Asynchronous design of AMQP-CPP library doesn't allow to use _callback_queue
		// variable before _myHandler.loop() call , because _callback_queue will be 
		// set after declareQueue.onSuccess call. So, this->request() call can't be 
		// performed here with correct env.setReplyTo(_callback_queue) value.
		//
		// this->request(n);
		//
		// For correct usage loop() must be reworked.

		// Process connection and channel operations and wait the result
		_myHandler.loop(_connection_uptr.get());
		return std::stoi(_response);
	}


private:
	MyTcpHandler _myHandler;

	AMQP::Address _address{"amqp://guest:guest@localhost/"};
	std::unique_ptr<AMQP::TcpConnection> _connection_uptr;
	std::unique_ptr<AMQP::TcpChannel> _channel_uptr;
	AMQP::QueueCallback _declare_callback;

	std::string _callback_queue;
	std::string _corr_id;
	std::string _response;


	void request(int n)
	{
		_response.clear();
		_corr_id = "uuid"; // TODO: uuid()

		std::string body = std::to_string(n);
		AMQP::Envelope env(body.c_str(), body.size());

		env.setCorrelationID(_corr_id);
		env.setReplyTo(_callback_queue);

		logger.msg(MSG_DEBUG, " [x] Requesting fib(%d)\n", n);

		_channel_uptr->publish("", "rpc_queue", env);
	}

	void on_response(const AMQP::Message &message, uint64_t deliveryTag, bool redelivered)
	{
		if(message.correlationID() != _corr_id){
			_myHandler.quit();
			return;
		}

		_response = std::string(message.body(), message.bodySize());

		logger.msg(MSG_DEBUG, " [.] Got %s\n",  _response);
		_myHandler.quit();
	}
};



int main(int argc, char* argv[])
{
	logger.init(MSG_DEBUG);

	FibonacciRpcClient rpc_client;

	rpc_client.call(30);
	
	return 0;
}