#include <iostream>

#include <sys/select.h>
#include <amqpcpp.h>
#include <amqpcpp/linux_tcp.h>

using namespace std;


fd_set rfds;

class MyTcpHandler : public AMQP::TcpHandler
{
private:
    /**
    * Method that is called when the connection succeeded
    * @param socket Pointer to the socket
    */
    virtual void onConnected(AMQP::TcpConnection* connection)
    {
        std::cout << "connected" << std::endl;
    }

    /**
     *  When the connection ends up in an error state this method is called.
     *  This happens when data comes in that does not match the AMQP protocol
     *
     *  After this method is called, the connection no longer is in a valid
     *  state and can be used. In normal circumstances this method is not called.
     *
     *  @param  connection      The connection that entered the error state
     *  @param  message         Error message
     */
    virtual void onError(AMQP::TcpConnection* connection, const char* message)
    {
        // report error
        std::cout << "AMQP TCPConnection error: " << message << std::endl;
    }

    /**
     *  Method that is called when the connection was closed.
     *  @param  connection      The connection that was closed and that is now unusable
     */
    virtual void onClosed(AMQP::TcpConnection* connection)
    {
        std::cout << "closed" << std::endl;
    }


     // *
     // *  Method that is called by the AMQP-CPP library when it wants to interact
     // *  with the main event loop. The AMQP-CPP library is completely non-blocking,
     // *  and only make "write()" or "read()" system calls when it knows in advance
     // *  that these calls will not block. To register a filedescriptor in the
     // *  event loop, it calls this "monitor()" method with a filedescriptor and
     // *  flags telling whether the filedescriptor should be checked for readability
     // *  or writability.
     // *
     // *  @param  connection      The connection that wants to interact with the event loop
     // *  @param  fd              The filedescriptor that should be checked
     // *  @param  flags           Bitwise or of AMQP::readable and/or AMQP::writable
     
    virtual void monitor(AMQP::TcpConnection* connection, int fd, int flags)
    {
    	// @todo
        //  add your own implementation, for example by adding the file
        //  descriptor to the main application event loop (like the select() or
        //  poll() loop). When the event loop reports that the descriptor becomes
        //  readable and/or writable, it is up to you to inform the AMQP-CPP
        //  library that the filedescriptor is active by calling the
        //  connection->process(fd, flags) method.

    	// cout << "monitor::" << endl;

		// we did not yet have this watcher - but that is ok if no filedescriptor was registered
		if (flags == 0){
			return;
		} 

		cout << "monitor::Fd " << fd << " Flags " << flags << endl;

		if(flags & AMQP::readable)
		{
			// cout << "monitor::Fd is readable" << endl;

			FD_SET(fd, &rfds);
			m_fd = fd;
			m_flags = flags;

			// cout << "monitor::FD_SET" << endl;
		}
    }

public:
    int m_fd = -1;
    int m_flags = 0;
};


int main(int argc, char* argv[])
{
    // address of the server
    AMQP::Address address("amqp://guest:guest@localhost/");

    // create a AMQP connection object
    MyTcpHandler myHandler;
    AMQP::TcpConnection connection(&myHandler, address);

    // and create a channel
    AMQP::TcpChannel channel(&connection);

    channel.onError([](const char* message)
    {
        cout << "channel error: " << message << endl;
    });
    channel.onReady([&]()
    {
        cout << "channel ready " << endl;

        channel.publish("", "hello", "Hello World!");
        std::cout << " [x] Sent 'Hello World!'" << std::endl;
    });

    // use the channel object to call the AMQP method you like

    channel.declareExchange("hello-exchange", AMQP::fanout);
    channel.declareQueue("hello");
    channel.bindQueue("hello-exchange", "hello", "hello-routing-key");


	// channel.consume("hello", AMQP::noack).onReceived(
	// 		[](const AMQP::Message &message,
	// 			uint64_t deliveryTag,
	// 			bool redelivered)
	// 		{
	// 		    std::cout <<" [x] Received " << message.body() << std::endl;
	// 		}
	// );

	int maxfd = 1;
	struct timeval timeout;

	while(true)
	{
		FD_ZERO(&rfds);
		FD_SET(myHandler.m_fd, &rfds);

		timeout.tv_sec = 2;
		timeout.tv_usec = 0;

		// cout << myHandler.m_fd << endl;
		if(myHandler.m_fd != -1){
    		maxfd = myHandler.m_fd + 1;
		}

		int result = select(maxfd, &rfds, NULL, NULL, &timeout);

		if((result == -1) && errno == EINTR)
		{
			cout << "Error in socket" << endl;
		}
        else if(result > 0)
        {
			if(myHandler.m_flags & AMQP::readable){
				 // cout << "Got something" << endl;
			}

			if(FD_ISSET(myHandler.m_fd, &rfds)){

				// cout << "FD_ISSET" << endl;

				connection.process(myHandler.m_fd, myHandler.m_flags);
			}
		}
    }
	
	return 0;
}