#include <iostream>
#include <string>
#include <amqpcpp.h>
#include <amqpcpp/linux_tcp.h>

#include "utils.hpp"
#include "my_handler.hpp"

using namespace std;

// The main idea behind Work Queues (aka: Task Queues) is to avoid doing 
// a resource-intensive task immediately and having to wait for it to complete. 
// Instead we schedule the task to be done later. We encapsulate a task as a 
// message and send it to the queue. A worker process running in the background 
// will pop the tasks and eventually execute the job. When you run many workers 
// the tasks will be shared between them.


int main(int argc, char* argv[])
{
	// 'Heavy' message to be processed by workers 
	std::string payload = argc > 1 ? utils::join(&argv[1], &argv[argc], " ") : "NewTask message";

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

	AMQP::QueueCallback callback = [&](const std::string &name, int msgcount, int consumercount){

		AMQP::Envelope env(payload.c_str(), payload.size());

		// Our queue os durable (survives server reboots), so
		// mark our message persistant to let queue store and save it.
		// Delivery mode (non-persistent (1) or persistent (2))
		env.setDeliveryMode(2);

		// 			(exchange, rounting_key, body, flags)
		channel.publish("", "task_queue", env);
		std::cout << " [x] Sent '" << env.body() << "' to 'task_queue'" << std::endl;
		
		myHandler.quit();
		channel.close();
	};

	// use the channel object to call the AMQP method you like
	// Use default exhange
	// Make queue durable (messages will be saved on server's disk)
	channel.declareQueue("task_queue", AMQP::durable).onSuccess(callback);

	myHandler.loop(&connection);

	return 0;
}