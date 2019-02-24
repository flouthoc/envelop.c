# envelop.c
Thread-less, event-loop based toy http-server from scratch. This project focuses on practical implementation of EPOLL for learning purpose.

<img src="https://github.com/flouthoc/envelop.c/blob/master/work.jpg" alt="drawing" width="350"/>

## Build

```bash
gcc -o envelop envelop.c
```

##### Starts server on default port: 3000
```bash
./envelop
```
#### Hit
```http://127.0.0.1:3000/hello```

##### Add your own Routes
See the Routes function.

## What is event-loop ?
Its an infinite loop which looks for any available events and performs required action on them.

##### Psuedo Code for event loop.

```
for (1){

	event = getReadyEvents();
    if(event == "task1"){
    	perform task1;
    }else if(event == "task2"){
    	perform task2;
    }
}
```
###### Theory
is a programming construct that waits for and dispatches events or messages in a program. It works by making a request to some internal or external "event provider" (that generally blocks the request until an event has arrived), and then it calls the relevant event handler ("dispatches the event"). The event-loop may be used in conjunction with a reactor, if the event provider follows the file interface, which can be selected or 'polled' (the Unix system call, not actual polling). The event loop almost always operates asynchronously with the message originator. 

## Epoll
- https://medium.com/@copyconstruct/the-method-to-epolls-madness-d9d2d6378642
- http://man7.org/linux/man-pages/man7/epoll.7.html
