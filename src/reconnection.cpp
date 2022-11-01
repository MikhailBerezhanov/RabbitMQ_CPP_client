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
	Auto reconnection demo
*/

static constexpr int recon_sec = 5;
static std::atomic<int> sig_received{0};
static MyTcpHandler myHandler;
static std::unique_ptr<AMQP::TcpConnection> connection_ptr;
static std::unique_ptr<AMQP::TcpChannel> channel_ptr;


static inline void signal_handler_init(std::initializer_list<int> signals)
{
	sig_received.store(0);

	auto signal_handler = [](int sig_num){ 
		sig_received.store(sig_num); 

		// Check if opened channel exists
		if(channel_ptr){

			// Gentle closing
			channel_ptr->close().onFinalize([&](){

				myHandler.quit();

				if(connection_ptr){
					connection_ptr->close();
					connection_ptr.reset();
				}
			});
		}
		else{
			// stop event loop
			myHandler.quit();
		}
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

	// Address of the server
	std::string addr = argc > 1 ? argv[1] : "amqp://guest:guest@localhost/";

	// For secure connections (TLS) use 'amqps://' address.
	// And init the SSL library:
	//
	// (for openssl 1.1: OPENSSL_init_ssl(0, nullptr); ) 
	// (for openssl 1.0 use SSL_library_init(); ) 

	AMQP::Address address(addr);

	for(;;){
		logger.msg(MSG_DEBUG, "Connecting to '%s'\n", addr);

		// Create a AMQP connection_ptr object
		connection_ptr = std::unique_ptr<AMQP::TcpConnection>(new AMQP::TcpConnection(&myHandler, address));

		// And create a channel
		// AMQP::TcpChannel channel(connection_ptr.get());
		channel_ptr = std::unique_ptr<AMQP::TcpChannel>(new AMQP::TcpChannel(connection_ptr.get()));

		// Setup asynchronous callbacks
		channel_ptr->onError([](const char* message)
		{
		    logger.msg(MSG_DEBUG, "Channel error: %s\n", message);
		});

		// Callback after queue declaration
		AMQP::QueueCallback qcb = [&](const std::string &name, int msgcount, int consumercount){
			// noack	- 	if set, consumed messages do not have to be acked, this happens automatically
			// Server will see that the message was acked and can delete it from the queue.
			channel_ptr->consume("hello", AMQP::noack)
				.onReceived([](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered)
				{
					std::string_view body(message.body(), message.bodySize());
				    logger.msg(MSG_DEBUG, " [x] Received '%s' (%lu bytes)\n", body.data(), message.bodySize());
				}
			);

			logger.msg(MSG_DEBUG, "Waiting for messages\n");
		};

		channel_ptr->onReady([&]()
		{
			logger.msg(MSG_DEBUG, "Channel is ready\n");

			// Use default exhange ("", direct)
			channel_ptr->declareQueue("hello").onSuccess(qcb);
		});

		// Main event loop (blocking)
		logger.msg(MSG_DEBUG, " [*] Starting event loop. To exit press CTRL-C\n");
		myHandler.loop(connection_ptr.get());

		// Loop is quited here for connection lost reason or signal received

		if(myHandler.connection_was_lost()){
			logger.msg(MSG_DEBUG, "Connection was lost\n");

			// Current connection  closing 
			connection_ptr->close();
			connection_ptr.reset();
			channel_ptr.reset();
			logger.msg(MSG_DEBUG, "Connection was closed. Reconnecting in %d seconds..\n", recon_sec);
		}

		// Prepare for reconnection try

		if(sig_received.load()){
			return 0;
		}

		sleep(recon_sec);	// signals may break sleep() 

		if(sig_received.load()){
			return 0;
		}
	}
	
	return 0;
}