
#include <cstring>
#include <cerrno>

extern "C"{
#include <sys/select.h>
#include <sys/time.h>
}

#include <thread>
#include <atomic>

#include "logger.hpp"
#include "my_handler.hpp"


struct MyTcpHandler::Impl
{
	fd_set readfds;
	fd_set writefds;

	int fd = -1;
	int flags = 0;
	std::atomic<bool> quit{false}; 			// break event loop flag
	std::atomic<bool> connected{false};		// connection is active flag

	// Heartbeats data
	uint16_t heartbeat_period = 30;			// (interval \ 2)
	std::atomic<uint16_t> heartbeats_timer{0};	//
	uint8_t heartbeat_fails = 0;			//
	static constexpr uint8_t heartbeat_max_fails = 2;
};


MyTcpHandler::MyTcpHandler(): AMQP::TcpHandler(), pimpl(new MyTcpHandler::Impl)
{

}

// Definition of desctuctor in place where MyTcpHandlerImpl is a complete type.
// The reason to definition explicitly a destructor is that when compiling, 
// the smart pointer ( std::unique_ptr ) checks if in the definition of 
// the type exists a visible destructor and throws a compilation error 
// if it’s only forward declared.
MyTcpHandler::~MyTcpHandler() = default;


/**
 *  Method that is called by the AMQP-CPP library when it wants to interact
 *  with the main event loop. The AMQP-CPP library is completely non-blocking,
 *  and only make "write()" or "read()" system calls when it knows in advance
 *  that these calls will not block. To register a filedescriptor in the
 *  event loop, it calls this "monitor()" method with a filedescriptor and
 *  flags telling whether the filedescriptor should be checked for readability
 *  or writability.
 *
 *  @param  connection      The connection that wants to interact with the event loop
 *  @param  fd              The filedescriptor that should be checked
 *  @param  flags           Bitwise or of AMQP::readable and/or AMQP::writable
 */
void MyTcpHandler::monitor(AMQP::TcpConnection *connection, int fd, int flags)
{
	// @todo
	//  add your own implementation, for example by adding the file
	//  descriptor to the main application event loop (like the select() or
	//  poll() loop). When the event loop reports that the descriptor becomes
	//  readable and/or writable, it is up to you to inform the AMQP-CPP
	//  library that the filedescriptor is active by calling the
	//  connection->process(fd, flags) method.

	logger.msg(MSG_TRACE, "monitor: fd: %d, flags: %d\n", fd, flags);

	if( !flags ){
		return;
	}

	pimpl->fd = fd;
	pimpl->flags = flags;

	// Adding the file descriptor to the event loop

	if(flags & AMQP::readable){
		FD_SET(pimpl->fd, &pimpl->readfds);
	}

	if(flags & AMQP::writable){
		FD_SET(pimpl->fd, &pimpl->writefds);
	}	
}

uint16_t MyTcpHandler::onNegotiate(AMQP::TcpConnection *connection, uint16_t interval)
{
	// we accept the suggestion from the server, but if the interval is smaller
	// that one minute, we will use a one minute interval instead
	if(interval < 60){
		interval = 60;
	} 

	// @todo
	//  set a timer in your event loop, and make sure that you call
	//  connection->heartbeat() every _interval_ seconds if no other
	//  instruction was sent in that period.

	pimpl->heartbeat_period = interval > 1 ? interval / 2 : interval;

	log_msg(MSG_DEBUG, "Heartbeat interval: %u, period: %u\n", interval, pimpl->heartbeat_period);

	// return the interval that we want to use
	return interval;
}

/**
 *  Method that is called when the server sends a heartbeat to the client
 *  @param  connection      The connection over which the heartbeat was received
 *  @see    ConnectionHandler::onHeartbeat
 */
void MyTcpHandler::onHeartbeat(AMQP::TcpConnection *connection)
{
	logger.msg(MSG_DEBUG, "heartbeat received from server\n");
	connection->heartbeat();

	// Server send us heartbeat frame, so 
	// no need to request heartbeat at current period 
	this->reset_heartbeats();
}

void MyTcpHandler::reset_heartbeats()
{
	pimpl->heartbeats_timer.store(0);
	pimpl->heartbeat_fails = 0;
}

void MyTcpHandler::process_heartbeats(AMQP::TcpConnection *connection)
{
	uint16_t heartbeats_incremented = pimpl->heartbeats_timer.load() + 1;

	if(heartbeats_incremented < pimpl->heartbeat_period){
		pimpl->heartbeats_timer.store(heartbeats_incremented);
		return;
	}

	// Heartbeats timer period elapsed

	if(connection->heartbeat()){
		logger.msg(MSG_DEBUG, "heartbeat sent to server\n");
		this->reset_heartbeats();
	}
	else{
		++pimpl->heartbeat_fails;
		logger.msg(MSG_DEBUG, "heartbeat to server failed (%d)\n", pimpl->heartbeat_fails);

		if(pimpl->heartbeat_fails >= Impl::heartbeat_max_fails){
			// Connection lost 
			this->onLost(connection);
		}

		// Reset heartbeats timer only 
		pimpl->heartbeats_timer.store(0);
	}

}

// Event loop reports that the descriptor becomes readable and/or writable 
// and informs the AMQP-CPP library that the filedescriptor is active 
// by calling the connection->process(fd, flags) method.
void MyTcpHandler::loop(AMQP::TcpConnection *connection)
{
	struct timeval timeout;
	int max_fd = 1;

	constexpr int tmout_sec = 1;	// 1 s
	constexpr int tmout_ms = 0;

	pimpl->quit.store(false);
	pimpl->connected.store(false);
	this->reset_heartbeats();

	for(;;){

		FD_ZERO(&pimpl->readfds);
		FD_ZERO(&pimpl->writefds);
		FD_SET(pimpl->fd, &pimpl->readfds);
		
		timeout.tv_sec = tmout_sec;
		timeout.tv_usec = tmout_ms;

		if(pimpl->fd > -1){
			max_fd = pimpl->fd + 1;
		}

		int res = select(max_fd, &pimpl->readfds, &pimpl->writefds, nullptr, &timeout);
		
		if(res < 0){

			// Signals breaks select() call, we will handle signals manually
			// (with this->quit()) for gentle channel and connection closing
			if(errno == EINTR){
				continue;	
			}

			if( !this->connection_was_lost() ){
				logger.msg(MSG_ERROR, "%s%s\n", excp_method("select failed(" + std::to_string(res) + "): "), strerror(errno));
			}

			return;
		}
		else{ 	
			// Timeout or Filedescriptor is ready for I\O

			// Process I\O operations
			if(FD_ISSET(pimpl->fd, &pimpl->readfds)){
				// logger.msg(MSG_VERBOSE, "connection->process readable (fd: %d, flags: %d)\n", pimpl->fd, pimpl->flags);
				connection->process(pimpl->fd, pimpl->flags);
			}
			
			if(FD_ISSET(pimpl->fd, &pimpl->writefds)){
				// logger.msg(MSG_VERBOSE, "connection->process writable (fd: %d, flags: %d)\n", pimpl->fd, pimpl->flags);
				connection->process(pimpl->fd, pimpl->flags);

				// Any traffic (e.g. protocol operations, published messages, 
				// acknowledgements) counts for a valid heartbeat.
				pimpl->heartbeats_timer.store(0);
			}

			// Check if loop break signal was catched
			if(pimpl->quit.load()){
				return;
			}

			// Hearbeats timer implementation - when select timeouts,increment timer counter. 
			if( !FD_ISSET(pimpl->fd, &pimpl->readfds) && !FD_ISSET(pimpl->fd, &pimpl->writefds) ){
				this->process_heartbeats(connection);
			}
			
		}
	}
}

void MyTcpHandler::quit()
{
	pimpl->quit.store(true);
}

bool MyTcpHandler::connection_was_lost() const
{
	return !pimpl->connected.load();
}


// Asynchronous callbacks (are called during event loop)

// Opening methods

/**
 *  Method that is called by the AMQP library when the TCP connection 
 *  has been established. After this method has been called, the library
 *  still has take care of setting up the optional TLS layer and of
 *  setting up the AMQP connection on top of the TCP layer., This method 
 *  is always paired with a later call to onLost().
 *  @param  connection      The connection that can now be used
 */
void MyTcpHandler::onConnected(AMQP::TcpConnection *connection) 
{
	// @todo
	//  add your own implementation (probably not needed)
	logger.msg(MSG_DEBUG, "onConnected\n");
	pimpl->connected.store(true);
}

/**
 *  Method that is called when the secure TLS connection has been established. 
 *  This is only called for amqps:// connections. It allows you to inspect
 *  whether the connection is secure enough for your liking (you can
 *  for example check the server certificate). The AMQP protocol still has
 *  to be started.
 *  @param  connection      The connection that has been secured
 *  @param  ssl             SSL structure from openssl library
 *  @return bool            True if connection can be used
 */
bool MyTcpHandler::onSecured(AMQP::TcpConnection *connection, const SSL *ssl)
{
	// @todo
	//  add your own implementation, for example by reading out the
	//  certificate and check if it is indeed yours
	logger.msg(MSG_DEBUG, "onSecured\n");
	return true;
}

/**
 *  Method that is called by the AMQP library when the login attempt
 *  succeeded. After this the connection is ready to use.
 *  @param  connection      The connection that can now be used
 */
void MyTcpHandler::onReady(AMQP::TcpConnection *connection) 
{
	// @todo
	//  add your own implementation, for example by creating a channel
	//  instance, and start publishing or consuming
	logger.msg(MSG_DEBUG, "onReady\n");
}



// Closing methods

void MyTcpHandler::onError(AMQP::TcpConnection *connection, const char *message) 
{
	// @todo
	//  add your own implementation, for example by reporting the error
	//  to the user of your program and logging the error
	logger.msg(MSG_ERROR, "onError: %s\n", message);
}

/**
 * Soft closing (when connection.close() method called)
 *  Method that is called when the AMQP protocol is ended. This is the
 *  counter-part of a call to connection.close() to graceful shutdown
 *  the connection. Note that the TCP connection is at this time still 
 *  active, and you will also receive calls to onLost() and onDetached()
 *  @param  connection      The connection over which the AMQP protocol ended
 */
void MyTcpHandler::onClosed(AMQP::TcpConnection *connection)  
{
    // @todo
    //  add your own implementation (probably not necessary, but it could
    //  be useful if you want to do some something immediately after the
    //  amqp connection is over, but do not want to wait for the tcp 
    //  connection to shut down
    logger.msg(MSG_DEBUG, "onClosed\n");
}

/**
 *  Method that is called when the TCP connection was closed or lost.
 *  This method is always called if there was also a call to onConnected()
 *  @param  connection      The connection that was closed and that is now unusable
 */
void MyTcpHandler::onLost(AMQP::TcpConnection *connection)  
{
	// @todo
	//  add your own implementation (probably not necessary)
	logger.msg(MSG_DEBUG, "onLost\n");
	pimpl->connected.store(false);

	// We've been connected already, stop running event gently.
	this->quit();
}

/**
 *  Final method that is called. This signals that no further calls to your
 *  handler will be made about the connection.
 *  @param  connection      The connection that can be destructed
 */
void MyTcpHandler::onDetached(AMQP::TcpConnection *connection)  
{
	// @todo
	//  add your own implementation, like cleanup resources or exit the application
	logger.msg(MSG_TRACE, "onDetached\n");
} 