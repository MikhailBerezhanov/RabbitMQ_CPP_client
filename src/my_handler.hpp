#pragma once

#include <iostream>
#include <memory>
#include <amqpcpp.h>
#include <amqpcpp/linux_tcp.h>

class MyTcpHandler : public AMQP::TcpHandler
{
public:

	MyTcpHandler();

	// std::unique_ptr with incomplete types. The problem lies in destruction.
	// If you use pimpl with unique_ptr, you need to declare a destructor,
	// but define it (with {}, or with = default;) where impl is complete (in .cpp.
	virtual ~MyTcpHandler();
	// Because otherwise the compiler generates a default one, and it needs
	// a complete declaration of MyTcpHandler::Impl for this.


	void loop(AMQP::TcpConnection *connection);

private:

	// IMPL forward declaration
	struct Impl;

	// Using a smart pointer is a better approach since the pointer takes 
	// control over the life cycle of the PImpl.
	std::unique_ptr<Impl> pimpl;
	// Impl *pimpl = nullptr;

	/**
	 *  Method that is called by the AMQP library when a new connection
	 *  is associated with the handler. This is the first call to your handler
	 *  @param  connection      The connection that is attached to the handler
	 */
	virtual void onAttached(AMQP::TcpConnection *connection) override
	{
	    // @todo
	    //  add your own implementation, for example initialize things
	    //  to handle the connection.
	    std::cout << "onAttached" << std::endl;
	}

	/**
	 *  Method that is called by the AMQP library when the TCP connection 
	 *  has been established. After this method has been called, the library
	 *  still has take care of setting up the optional TLS layer and of
	 *  setting up the AMQP connection on top of the TCP layer., This method 
	 *  is always paired with a later call to onLost().
	 *  @param  connection      The connection that can now be used
	 */
	virtual void onConnected(AMQP::TcpConnection *connection) override
	{
	    // @todo
	    //  add your own implementation (probably not needed)
	    std::cout << "onAttached" << std::endl;
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
	virtual bool onSecured(AMQP::TcpConnection *connection, const SSL *ssl) override
	{
	    // @todo
	    //  add your own implementation, for example by reading out the
	    //  certificate and check if it is indeed yours
	    return true;
	}

	/**
	 *  Method that is called by the AMQP library when the login attempt
	 *  succeeded. After this the connection is ready to use.
	 *  @param  connection      The connection that can now be used
	 */
	virtual void onReady(AMQP::TcpConnection *connection) override
	{
	    // @todo
	    //  add your own implementation, for example by creating a channel
	    //  instance, and start publishing or consuming
	    std::cout << "onReady" << std::endl;
	}

	/**
	 *  Method that is called by the AMQP library when a fatal error occurs
	 *  on the connection, for example because data received from RabbitMQ
	 *  could not be recognized, or the underlying connection is lost. This
	 *  call is normally followed by a call to onLost() (if the error occurred
	 *  after the TCP connection was established) and onDetached().
	 *  @param  connection      The connection on which the error occurred
	 *  @param  message         A human readable error message
	 */
	virtual void onError(AMQP::TcpConnection *connection, const char *message) override
	{
	    // @todo
	    //  add your own implementation, for example by reporting the error
	    //  to the user of your program and logging the error
	    std::cerr << "onError: " << message << std::endl;
	}

	/**
	 *  Method that is called when the AMQP protocol is ended. This is the
	 *  counter-part of a call to connection.close() to graceful shutdown
	 *  the connection. Note that the TCP connection is at this time still 
	 *  active, and you will also receive calls to onLost() and onDetached()
	 *  @param  connection      The connection over which the AMQP protocol ended
	 */
	virtual void onClosed(AMQP::TcpConnection *connection) override 
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
	virtual void onLost(AMQP::TcpConnection *connection) override 
	{
	    // @todo
	    //  add your own implementation (probably not necessary)
	    std::cout << "onLost" << std::endl;
	}

	/**
	 *  Final method that is called. This signals that no further calls to your
	 *  handler will be made about the connection.
	 *  @param  connection      The connection that can be destructed
	 */
	virtual void onDetached(AMQP::TcpConnection *connection) override 
	{
	    // @todo
	    //  add your own implementation, like cleanup resources or exit the application
	    std::cout << "onDetached" << std::endl;
	} 

	virtual void monitor(AMQP::TcpConnection *connection, int fd, int flags) override;
};