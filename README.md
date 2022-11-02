# RabbitMQ tutorials for C++
### Using AMQP-CPP client library

## Pre requirenments

* C++17 compiler
* Cmake

For Debian \ Ubuntu
```bash
sudo apt install cmake
```

## How to build

Prepare directories

```bash
# Get submodules
git submodule update --init --recursive --remote

# Build using Cmake
mkdir build
cd build
```

If you have AMQP-CPP already installed in your system, then configure default build 
(binaries will be linked dynamically with shared library)

```bash
cmake ..
```

Otherwise enable AMQP-CPP library building (static by default):

```bash
cmake .. -DBUILD_AMQPCPP=ON -DAMQP-CPP_LINUX_TCP=ON
```

Finally build executables!
```bash
cmake --build .
```

## Run tutorials

[Tutorial one: "Hello World!"](http://www.rabbitmq.com/tutorial-one-python.html):

    send
    receive


[Tutorial two: Work Queues](http://www.rabbitmq.com/tutorial-two-python.html):

    new_task "A very hard task which takes two seconds.."
    worker


[Tutorial three: Publish/Subscribe](http://www.rabbitmq.com/tutorial-three-python.html):

    receive_logs
    emit_log "info: This is the log message"


[Tutorial four: Routing](http://www.rabbitmq.com/tutorial-four-python.html):

    receive_logs_direct info
    emit_log_direct info "The message"


[Tutorial five: Topics](http://www.rabbitmq.com/tutorial-five-python.html):

    receive_logs_topic "*.rabbit"
    emit_log_topic red.rabbit Hello


[Tutorial six: RPC](http://www.rabbitmq.com/tutorial-six-python.html):

    rpc_server
    rpc_client