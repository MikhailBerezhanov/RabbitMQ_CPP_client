#include <iostream>

#include "logger.hpp"
#include "my_handler.hpp"

using namespace std;

/*
	Tutorial #7: Publisher confirms

	Publisher confirms are a RabbitMQ extension to the AMQP 0.9.1 protocol, 
	so they are not enabled by default. Publisher confirms are enabled at
	 the channel level 
*/

int main(int argc, char* argv[])
{
	MyTcpHandler handler;

	AMQP::Address addr("amqp://guest:guest@localhost/");
	AMQP::TcpConnection connection(&handler, addr);
	AMQP::TcpChannel channel(&connection);

	channel.onError([&](const char* message)
	{
		 logger.msg(MSG_DEBUG, "Channel error: %s\n", message);
		 handler.quit();
	});


	// To enable confirms, a client sends the confirm.select method. 
	// Depending on whether no-wait was set or not, the broker may 
	// respond with a confirm.select-ok. Once the confirm.select method is
	//  used on a channel, it is said to be in confirm mode. A transactional 
	// channel cannot be put into confirm mode and once a channel is in confirm mode, 
	// it cannot be made transactional.

	// Put channel in a confirm mode (RabbitMQ specific)
	// This method must be called on every channel that you expect to use publisher 
	// confirms. Confirms should be enabled just once, not for every message published.
	channel.confirmSelect()

		.onSuccess([](){
			logger.msg(MSG_DEBUG, "confirmSelect - OK\n");
		})
		// Callback that is called when the broker confirmed message publication
		.onAck([](uint64_t deliveryTag, bool multiple)
		{

		})
		// Callback that is called when the broker denied message publication
		.onNack([](uint64_t deliveryTag, bool multiple, bool requeue){

		});



	handler.loop(&connection);

	return 0;
}