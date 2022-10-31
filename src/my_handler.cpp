
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

	// The reason to declare explicitly a destructor is that when compiling, 
	// the smart pointer ( std::unique_ptr ) checks if in the definition of 
	// the type exists a visible destructor and throws a compilation error 
	// if it’s only forward declared.


	fd_set readfds;
	fd_set writefds;

	int fd = -1;
	int flags = 0;
	std::atomic<bool> quit{false}; 	// break event loop
	std::atomic<bool> lost{false};	// connection was lost
};


MyTcpHandler::MyTcpHandler(): AMQP::TcpHandler(), pimpl(new MyTcpHandler::Impl)
{

}

// Definition of desctuctor in place where MyTcpHandlerImpl is a complete type.
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

	logger.msg(MSG_VERBOSE, "monitor: fd: %d, flags: %d\n", fd, flags);

	if( !flags ){
		return;
	}

	pimpl->fd = fd;
	pimpl->flags = flags;

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
	if (interval < 60) interval = 60;

	// @todo
	//  set a timer in your event loop, and make sure that you call
	//  connection->heartbeat() every _interval_ seconds if no other
	//  instruction was sent in that period.

	// return the interval that we want to use
	return interval;
}

void MyTcpHandler::onHeartbeat(AMQP::TcpConnection *connection)
{
	logger.msg(MSG_DEBUG, "heartbeat\n");
	connection->heartbeat();
}

// TODO: spawn independent thread for the loop
//
// TODO: make connection auto-reconnect - use pointer to pointer or shared_ptr
//
void MyTcpHandler::loop(AMQP::TcpConnection *connection)
{
	struct timeval timeout;
	int max_fd = 1;

	constexpr int ms = 100'1000;	// 100 ms

	pimpl->quit.store(false);

	for(;;){

		FD_ZERO(&pimpl->readfds);
		FD_SET(pimpl->fd, &pimpl->readfds);

		FD_ZERO(&pimpl->writefds);
		// FD_SET(pimpl->fd, &pimpl->writefds);

		timeout.tv_sec = 0;
		timeout.tv_usec = ms;

		if(pimpl->fd > -1){
			max_fd = pimpl->fd + 1;
		}

		int res = select(max_fd, &pimpl->readfds, &pimpl->writefds, nullptr, &timeout);
		
		if(res < 0 /*&& errno == EINTR*/){

			if( !this->connection_was_lost() ){
				logger.msg(MSG_ERROR, "%s%s\n", excp_method("select failed(" + std::to_string(res) + "): "), strerror(errno));
			}
			
			return;
		}
		else{ 	// Timeout or Filedescriptor is ready for I\O

			// Which I\O is ready ?

			if(FD_ISSET(pimpl->fd, &pimpl->readfds)){
				// logger.msg(MSG_TRACE, "connection->process readable (fd: %d, flags: %d)\n", pimpl->fd, pimpl->flags);
				connection->process(pimpl->fd, pimpl->flags);
			}
			
			if(FD_ISSET(pimpl->fd, &pimpl->writefds)){
				// logger.msg(MSG_TRACE, "connection->process writable (fd: %d, flags: %d)\n", pimpl->fd, pimpl->flags)
				connection->process(pimpl->fd, pimpl->flags);
			}

			if(pimpl->quit.load()){
				return;
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
	bool res = pimpl->lost.load();

	if(res){
		pimpl->lost.store(false);
	}

	return res;
}


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
	std::cout << "onConnected" << std::endl;
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
	std::cout << "onSecured" << std::endl; 
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
	std::cout << "onReady" << std::endl;
}



// Closing methods

void MyTcpHandler::onError(AMQP::TcpConnection *connection, const char *message) 
{
	// @todo
	//  add your own implementation, for example by reporting the error
	//  to the user of your program and logging the error
	std::cerr << "onError: " << message << std::endl;
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
    std::cout << "onClosed" << std::endl;
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
	std::cout << "onLost" << std::endl;
	pimpl->lost.store(true);

	// We've been connected already, event loop is running by 
	// current time - stop it gently. 
	this->quit();

	// TODO: Try to perform auto-reconnect. 
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
	std::cout << "onDetached" << std::endl;
} 