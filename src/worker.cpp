#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <string>
#include <string_view>

#include <amqpcpp.h>
#include <amqpcpp/linux_tcp.h>

#include "logger.hpp"
#include "my_handler.hpp"

/*
	Tutorial #2: Task queue
*/

using namespace std;
using namespace std::chrono_literals;

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

	channel.onError([](const char* message){
		logger.msg(MSG_DEBUG, "Channel error: %s\n", message);
	});


	// Fair dispatch.
	//
	// RabbitMQ just dispatches a message when the message enters the queue. 
	// It doesn't look at the number of unacknowledged messages for a consumer. 
	// It just blindly dispatches every n-th message to the n-th consumer.
	// 
	// In order to defeat that we can use the Channel#basic_qos channel method with 
	// the prefetch_count=1 setting.
	// This uses the basic.qos protocol method to tell RabbitMQ not to give more 
	// than one message to a worker at a time. Or, in other words, don't dispatch 
	// a new message to a worker until it has processed and acknowledged the previous one. 
	// Instead, it will dispatch it to the next worker that is not still busy.

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


	// We don't want to lose any tasks. If a worker dies, 
	// we'd like the task to be delivered to another worker.
	// In order to make sure a message is never lost, RabbitMQ supports 
	// message acknowledgments. An ack(nowledgement) is sent back by the 
	// consumer to tell RabbitMQ that a particular message had been received, 
	// processed and that RabbitMQ is free to delete it.
	//
	// So we don't use automatic ack when message is read. 
	// We use manual ack when message is proccessed.
	//
	// If a consumer dies (its channel is closed, connection is closed, 
	// or TCP connection is lost) without sending an ack, RabbitMQ will
	// understand that a message wasn't processed fully and will re-queue 
	// it. If there are other consumers online at the same time, it will 
	// then quickly redeliver it to another consumer. That way you can be 
	// sure that no message is lost, even if the workers occasionally die.

	channel.consume("task_queue").onReceived(
		[&channel](const AMQP::Message &message,
			uint64_t deliveryTag,
			bool redelivered)
		{
			const std::string_view body(message.body(), message.bodySize());

			logger.msg(MSG_DEBUG, " [x] Received '%s' (%lu bytes)\n", body.data(), message.bodySize());

			/* auto cnt = std::count(message.body(), &message.body()[message.bodySize()], '.'); */
			size_t cnt = std::count(body.begin(), body.end(), '.');

			// Imitation of data processing
			std::this_thread::sleep_for(cnt * 1s);

			// Message fully processed and can be acked to be removed from the queue.
			logger.msg(MSG_DEBUG, " [x] Done\n");
			channel.ack(deliveryTag);
		}
	);

	// for debug unacked messages can be seen with
	// sudo rabbitmqctl list_queues name messages_ready messages_unacknowledged

	return 0;
}