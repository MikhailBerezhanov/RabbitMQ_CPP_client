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

	channel.onError([](const char* message){
		cout << "Channel error: " << message << endl;
	});

	/**
	 *  Set the Quality of Service (QOS) for this channel
	 *
	 *  When you consume messages, every single message needs to be ack'ed to inform
	 *  the RabbitMQ server that is has been received. The Qos setting specifies the
	 *  number of unacked messages that may exist in the client application. The server
	 *  stops delivering more messages if the number of unack'ed messages has reached
	 *  the prefetchCount
	 *
	 *  @param  prefetchCount       maximum number of messages to prefetch
	 *  @param  global              share counter between all consumers on the same channel
	 *  @return bool                whether the Qos frame is sent.
	 */
	channel.setQos(1);	// setting prefetchCount = 1

	channel.declareQueue("task_queue", AMQP::durable);

	channel.consume("task_queue").onReceived(
		[](const AMQP::Message &message,
			uint64_t deliveryTag,
			bool redelivered)
		{



		    std::cout <<" [x] Received (" << message.bodySize() << " bytes)" << message.body() << std::endl;
		
			// Message fully processed and can be acked to be removed from the queue.
			channel.ack(deliveryTag);
		}
	);



	return 0;
}