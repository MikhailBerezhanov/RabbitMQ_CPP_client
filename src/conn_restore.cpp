#include <iostream>
#include <string_view>
#include <thread>
#include <chrono>
#include <memory>
#include <csignal>
#include <atomic>

extern "C"{
#include <unistd.h>
}

#include <amqpcpp.h>
#include <amqpcpp/linux_tcp.h>

#include "logger.hpp"
#include "my_handler.hpp"

using namespace std;

/*
	Auto reconnection
*/
static std::atomic<int> sig_received{0};
static MyTcpHandler myHandler;

static inline void signal_handler_init(std::initializer_list<int> signals)
{
	sig_received.store(0);

	auto signal_handler = [](int sig_num){ 
		sig_received.store(sig_num); 
		myHandler.quit();
	};

	struct sigaction act;
	memset(&act, 0, sizeof(act));
	act.sa_handler = signal_handler;
	sigemptyset(&act.sa_mask);

	for(const auto sig : signals){
		sigaddset(&act.sa_mask, sig);
		sigaction(sig, &act, nullptr);
	}
}


int main(int argc, char* argv[])
{
	logger.init(MSG_DEBUG);

	signal_handler_init({SIGINT, SIGQUIT, SIGTERM});

	// address of the server
	std::string addr = argc > 1 ? argv[1] : "amqp://guest:guest@localhost/";

	// init the SSL library (this works for openssl 1.1, 
	// for openssl 1.0 use SSL_library_init())
	// OPENSSL_init_ssl(0, NULL);

	AMQP::Address address(addr);

	for(;;){

		logger.msg(MSG_DEBUG, "Connecting to '%s'\n", addr);

		// create a AMQP connection object
		auto connection = std::make_shared<AMQP::TcpConnection>(&myHandler, address);

		// and create a channel
		AMQP::TcpChannel channel(connection.get());

		channel.onError([](const char* message)
		{
		    logger.msg(MSG_DEBUG, "Channel error: %s\n", message);
		});

		// Callback after queue declaration
		AMQP::QueueCallback qcb = [&](const std::string &name, int msgcount, int consumercount){
			
			// noack	- 	if set, consumed messages do not have to be acked, this happens automatically
			// Server will see that the message was acked and can delete it from the queue.
			channel.consume("hello", AMQP::noack)
				.onReceived([](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered)
				{
					std::string_view body(message.body(), message.bodySize());
				    logger.msg(MSG_DEBUG, " [x] Received '%s' (%lu bytes)\n", body.data(), message.bodySize());
				}
			);
		};

		channel.onReady([&]()
		{
			logger.msg(MSG_DEBUG, "Channel is ready\n");

			// Use default exhange ("", direct)
			channel.declareQueue("hello").onSuccess(qcb);
		});

		logger.msg(MSG_DEBUG, " [*] Waiting for messages. To exit press CTRL-C\n");
		myHandler.loop(connection.get());

		// Loop is quited here

		if(myHandler.connection_was_lost()){
			logger.msg(MSG_DEBUG, "Connection was lost\n");

			// Gentle current connection closing 
			connection.get()->close();
			connection.reset();
			logger.msg(MSG_DEBUG, "Connection was closed. Reconnecting\n");
		}
		else{
			// channel.close().onFinalize([&](){
			// 	myHandler.quit();
			// 	connection.close();
			// });
		}

		if(sig_received.load()){
			return 0;
		}

		std::this_thread::sleep_for(std::chrono::seconds(5));
	}
	
	return 0;
}