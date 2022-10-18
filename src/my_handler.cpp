
#include <cstring>
#include <cerrno>

extern "C"{
#include <sys/select.h>
#include <sys/time.h>
}

#include <thread>

#include "my_handler.hpp"


struct MyTcpHandler::Impl
{

	// The reason to declare explicitly a destructor is that when compiling, 
	// the smart pointer ( std::unique_ptr ) checks if in the definition of 
	// the type exists a visible destructor and throws a compilation error 
	// if it’s only forward declared.


	fd_set readfds;
	int fd = -1;
	int flags = 0;
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
	

	// if( !flags ){
	// 	return;
	// }

	if(flags & AMQP::readable){
		pimpl->fd = fd;
		pimpl->flags = flags;

		FD_SET(pimpl->fd, &pimpl->readfds);
	}
}

// TODO: spawn independent thread for the loop
void MyTcpHandler::loop(AMQP::TcpConnection *connection)
{
	struct timeval timeout;
	int max_fd = 1;

	for(;;){

		FD_ZERO(&pimpl->readfds);
		FD_SET(pimpl->fd, &pimpl->readfds);

		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		if(pimpl->fd > -1){
			max_fd = pimpl->fd + 1;
		}

		// std::cout << "max_fd: " << max_fd << std::endl;

		int res = select(max_fd, &pimpl->readfds, nullptr, nullptr, &timeout);
		
		if(res < 0 /*&& errno == EINTR*/){
			std::cerr << "MyTcpHanler::loop: select() failed: " << strerror(errno) << std::endl;
			return;
		}
		else if( !FD_ISSET(pimpl->fd, &pimpl->readfds) ){
			// Timeout
			continue;
		}

		// Filedescriptor is ready for I\O
		connection->process(pimpl->fd, pimpl->flags);
	}
}