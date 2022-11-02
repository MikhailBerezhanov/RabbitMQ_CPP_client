#include <iostream>

#include <amqpcpp.h>
#include <amqpcpp/linux_tcp.h>

#include "utils.hpp"
#include "logger.hpp"
#include "my_handler.hpp"

using namespace std;

/*
	Tutorial #5: Topic

	In our logging system we might want to subscribe to not only logs 
	based on severity, but also based on the source which emitted the log. 
	You might know this concept from the syslog unix tool, which routes 
	logs based on both severity (info/warn/crit...) and facility (auth/cron/kern...).

	That would give us a lot of flexibility - we may want to listen to just 
	critical errors coming from 'cron' but also all logs from 'kern'.

	To implement that in our logging system we need to learn about a more complex 
	topic exchange.


	Topic exchange is powerful and can behave like other exchanges.

	When a queue is bound with "#" (hash) binding key - it will receive all the messages, 
	regardless of the routing key - like in fanout exchange.

	When special characters "*" (star) and "#" (hash) aren't used in bindings, the topic 
	exchange will behave just like a direct one.
*/

int main(int argc, char* argv[])
{
	logger.init(MSG_DEBUG);

	// Usage example: emit_log_topic "kern.critical" "A critical kernel error"

	// the routing keys of logs will have two words: "<facility>.<severity>"
	std::string routing_key = argc > 1 ? argv[1] : "anonymous.info";
	std::string payload = argc > 2 ? utils::join(&argv[2], &argv[argc], " ") : "message";

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

	// use the channel object to call the AMQP method you like

	// Create topic exchange 
	// The binding key must also be in the same form. The logic behind the topic exchange is similar to a direct one - a message sent with a particular routing key will be delivered to all the queues that are bound with a matching binding key. However there are two important special cases for binding keys:
	//
	// * (star) can substitute for exactly one word.
	// # (hash) can substitute for zero or more words.

	// Messages sent to a topic exchange can't have an arbitrary routing_key - 
	// it must be a list of words, delimited by dots. The words can be anything, 
	// but usually they specify some features connected to the message.

	channel.declareExchange("topic_logs", AMQP::topic)
		.onSuccess([&]()
		{
			// 	publish(exchange, rounting_key, message, flags)
			channel.publish("topic_logs", routing_key, payload);
			logger.msg(MSG_DEBUG, "[x] Sent %s:%s to topic_logs exchange\n", routing_key, payload);
		
			// Gentle closing
			channel.close().onFinalize([&](){
				myHandler.quit();
				connection.close();
			});
		}
	);

	myHandler.loop(&connection);
	
	return 0;
}