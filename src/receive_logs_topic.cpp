#include <iostream>
#include <string_view>

#include <amqpcpp.h>
#include <amqpcpp/linux_tcp.h>

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

	std::vector<std::string> binding_keys;

	// To receive all the logs run:
	// 
	// 		receive_logs_topic "#"
	//
	// To receive all logs from the facility "kern":
	// 
	// 		receive_logs_topic "kern.*"
	//
	// Or if you want to hear only about "critical" logs:
	// 
	// 		receive_logs_topic "*.critical"
	//
	// You can create multiple bindings:
	//
	// 		receive_logs_topic "kern.*" "*.critical"
	//
	for(int i = 1; i < argc; ++i){
		binding_keys.push_back(argv[i]);
	}

	if(binding_keys.empty()){
		std::cerr << "Usage: " << argv[0] << " [info] [warning] [error]" << std::endl;
		return 0;
	}

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


	auto reveive_callback = [](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered)
	{
		std::string_view body(message.body(), message.bodySize());
		logger.msg(MSG_DEBUG, " [x] Received %s:%s\n", message.routingkey(), body.data());
	};

	// Create direct exchange (routes message to binded queue 
	// with exactly matching routing_key)
	// ** to see excnahes use:
	//
	// sudo rabbitmqctl list_exchanges
	// rabbitmqctl list_bindings
	//
	channel.declareExchange("topic_logs", AMQP::topic)
		.onSuccess([&]()
		{
			// Use randomly named-by-server queue. Make it exclusive (the queue only exists 
			// for this connection, and is automatically removed when connection is gone)
			channel.declareQueue("", AMQP::exclusive)
				.onSuccess([&](const std::string &name, uint32_t messagecount, uint32_t consumercoun)
				{
					// name contains a random queue name. 
					// For example it may look like amq.gen-JzTY20BRgKO-HjmUJj0wLg
					logger.msg(MSG_DEBUG, "Generated queue name: %s\n", name);

					for(const auto &key : binding_keys){
						// A binding is a relationship between an exchange and a queue. 
						// This can be simply read as: the queue is interested in messages 
						// from this exchange.
						channel.bindQueue("topic_logs", name, key);
					}
					
					channel.consume(name, AMQP::noack).onReceived(reveive_callback);
				}
			);
		}
	);
	

	logger.msg(MSG_DEBUG, " [*] Waiting for messages. To exit press CTRL-C\n");
    myHandler.loop(&connection);
	
	return 0;
}