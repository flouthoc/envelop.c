/* Author: http://github.com/flouthoc (flouthoc@gmail.com)
   Authors: <Contributor List>
*/
 
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>

/*Reading conventions

  MACROS - all the tokens which are in full capital letters.
  ENUMS - all the tokens which start with capital letter and then switch to small.
  STRUCTS - all small letters most of the time they will contain underscore
  VARS - all small you'll notice them :)
 */


//Server will run on 127.0.0.1:SERVER_PORT
#define SERVER_PORT 3000

/*HTTP versions*/
#define VERSION_11 "1.1"
#define VERSION_10 "1.0"
#define MAXEVENTS 64

/*Header Status*/


/*Just a buffer to save some content for later*/
char global_buffer[1024];

enum Method{ /*only as much as method server needs to process*/
	GET, POST
};

/* You know this right ? HTTP status codes */
enum Http_status{
	HTTP_STATUS_OK        = 200,
	HTTP_STATUS_CREATED    = 201,
	HTTP_STATUS_ACCEPTED    = 202,
	HTTP_STATUS_BAD_REQUEST = 400,
	HTTP_STATUS_NOT_FOUND   = 404,
};




struct http_request{ /*header only as much as we need*/

	char version[4];
	enum Method method;
	char *uri;
};


struct http_response{ /*this is response body which we will use through out the code*/

	char *header;
	size_t length;
	int is_content; //this will be 1 if there is any message body in this response and 0 if not , this is just to add extra rn when replying
};


//will write to the open sockett
void print_rn(char * str)
{
	while ( *str ) {
		if ( *str == '\n' ) {
			fputs("\n", stdout);
		} else if ( *str == '\r' ) {
			fputs("\r", stdout);
		} else {
			fputc(*str, stdout);
		}
		str++;
	}
	fputs("\n", stdout);
}



static inline void extend_header(struct http_response *handle,const char *extension, size_t length)
{
	struct http_response *h = handle;
	h->length += length;
	h->header = realloc(h->header, h->length);
	strcat(h->header, extension);
}

static void reply(struct http_response *handle, int connectionfd){

	int written, n;
	char *buf;
	if(handle->is_content == 0) extend_header(handle, "\r\n", 2);
	n = handle->length;
	buf = handle->header;

	print_rn(buf);

	while(n > 0){
		if( (written = write(connectionfd, buf, (handle->length+1))) > 0){
			buf += written;
			n -= written;
		}else{
			perror("When replying to file descriptor");
		}
	}

	free(handle->header);
	free(handle);

}

/*TODO*/
/* to be done a minimal template parser like jinja */
static char* readTemplate(char *filename){

	if(filename != NULL){

		

		
	}

}


static inline void prepare_status_line(struct http_response **response_handle, char *version, enum Http_status status){

	struct http_response *r = malloc(sizeof(struct http_response));
	/*Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF*/
	/*I wont use Reason-Phrase cause its not important for me now*/
	r->header = malloc(16*sizeof(char)); //hard-coding length for HTTP/X.X(8) SP(1) STATUS-CODE(3) SP(1) CRLF(2)
	r->length = sprintf(r->header, "HTTP/%s %d \r\n", version, status);
	r->is_content = 0;
	*response_handle = r;


}

void setHeader(struct http_response *handle, char *header, char*value){

	size_t len;
	char local_buffer[1024];
	strcpy(local_buffer, value);
	len = sprintf(global_buffer, "%s: %s\r\n", header, local_buffer);
	extend_header(handle, global_buffer, len);
}


void replywithContent(struct http_response *response_handle, int connectionfd, const char *content){

	size_t content_length = strlen(content);
	sprintf(global_buffer, "%d", (int)content_length);
	setHeader(response_handle, "Content-Length", global_buffer);
	setHeader(response_handle, "Content-Type", "text/html");
	extend_header(response_handle, "\r\n", 2);
	response_handle->is_content = 1;
	extend_header(response_handle, content, content_length);
	reply(response_handle, connectionfd);

}




/* You can start reading from here, Its a basic router welcome back little web deveoper */
void router(struct http_request *request_handle, int connectionfd, struct http_response **response_handle){

	if(!strcmp(request_handle->uri, "/hello")){

		prepare_status_line(response_handle, request_handle->version, HTTP_STATUS_OK);
		replywithContent(*response_handle, connectionfd, "Hello");

	}else if(!strcmp(request_handle->uri, "/hey")){

		prepare_status_line(response_handle, request_handle->version, HTTP_STATUS_OK);
		replywithContent(*response_handle, connectionfd, "Hey hi this is a test");


	}else{
		prepare_status_line(response_handle, request_handle->version, HTTP_STATUS_NOT_FOUND);
		replywithContent(*response_handle, connectionfd, "Not Found");
	}

}




//this will parse http request but in a limited manner as we are making this for learning.
static int parse_http_request(struct http_request **req, char *request_buffer) /*parsing only as much as we need to make this server work*/
{

	int length;
	char *uri;
	char *token_handler;
	struct http_request *p = (struct http_request *)(malloc(sizeof(struct http_request)));
	token_handler = strtok(request_buffer, " ");

	if(!strcmp(token_handler, "GET")) p->method = GET;
	else if(!strcmp(token_handler, "POST")) p->method = POST;
	else return 0;

	token_handler = strtok(NULL, " ");
	length = strlen(token_handler);
	uri = malloc(sizeof(char) * (length + 1));
	strcpy(uri, token_handler);
	p->uri = uri;

	token_handler = strtok(NULL, " ");
	if(!strncmp(token_handler, "HTTP/1.1\r\n", 10)) strcpy(p->version, VERSION_11);
	else if(!strncmp(token_handler, "HTTP/1.0\r\n", 10)) strcpy(p->version, VERSION_10);
	else return 0;


	*req = p;
	return 1;

}

void respond(struct http_request *request_handle, int connectionfd)
{

	struct http_response *response_handle;
	if(request_handle->method == GET) router(request_handle, connectionfd, &response_handle);
	else goto error;
	return;

error:
	prepare_status_line(&response_handle, request_handle->version, HTTP_STATUS_BAD_REQUEST);
	reply(response_handle, connectionfd);
}


int main(){

	int n,i;
	int listenfd;
	int listenfd_flags;
	int epoll_fd;
	char msg_buffer[1024];
	char temp_buffer[1024];
	int connectionfd;
	int client_length;
	
	/* Go scroll to top to read about struct http_request */
	struct http_request *request_handle;

	/*sockaddr_in structs to store client and server's addr info , IP address and all */
	struct sockaddr_in server_address, client_address;

	
	/* epoll_event will store all the event which we are going to monitor */
	/* OSX Users might want to leave as dis code will only work on systems supporting epoll */
	struct epoll_event event;
	struct epoll_event *events;

	char greet[] = "Heelo";


	/*This file discriptor will listen*/
	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	if(listenfd == -1){
		perror("Socket");
		exit(1);
	}

	/*clear up the server_address*/
	memset(&server_address, 0, sizeof(server_address));

	/*giving it some server info */
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr("0.0.0.0");
	/*check server port on the top */
	server_address.sin_port = htons(SERVER_PORT);


	/* we need to bind descriptor to a port and address */
	if(bind(listenfd, (struct sockaddr*)&server_address, sizeof(server_address)) == -1){
		perror("Bind");
		exit(1);
	}


	if((listenfd_flags = fcntl(listenfd, F_GETFL, 0)) == -1){
		perror("While trying to get listenfd flags");
		exit(1);
	}
	
	/*we need this connection to be non-blocking , remember event-loop*/
	listenfd_flags |= O_NONBLOCK;

	if(fcntl(listenfd, F_SETFL, listenfd_flags) == -1){
		perror("While trying to set flags");
		exit(1);
	}


	if(listen(listenfd, 10) == -1){
		perror("Listen");
		exit(1);
	}

	if((epoll_fd = epoll_create1(0)) == -1){
		perror("epoll_create");
		exit(1);
	}

	event.data.fd = listenfd;
	event.events = EPOLLIN;
	if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listenfd, &event) == -1){
		perror("Epoll context");
		exit(1);
	}

	events = calloc(MAXEVENTS, sizeof event);

	/*event-loop*/
	while (1){

		/*wait for event_wait to return no of events which are ready to be processed*/ 
		n = epoll_wait (epoll_fd, events, MAXEVENTS, -1);
		for (i = 0; i < n; i++){

			/* if event state is not the one we need skip this event */
			if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) ||(!(events[i].events & EPOLLIN))){
				/* file descriptor is not yet connected for transport, can be closed without shutdown */
				close(events[i].data.fd);
				continue;
			}
			else if (listenfd == events[i].data.fd){

				//okay a new client , we need to onboard him
				while (1){

					/*oke new connection might wanna accept it*/
					int connectionfd_flags;					
					connectionfd = accept(listenfd, (struct sockaddr*)&client_address, &client_length);
					if (connectionfd == -1){

						if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
						{
							/* We have processed all incoming
							   connections. */
							break;
						}
						else
						{
							perror ("accept");
							break;
						}
					}


					/* We musk make client's socket NON-BLOCKING */
					if((connectionfd_flags = fcntl(connectionfd, F_GETFL, 0)) == -1){
						perror("While trying to get listenfd flags");
						exit(1);
					}

					connectionfd_flags |= O_NONBLOCK;

					if(fcntl(connectionfd, F_SETFL, connectionfd_flags) == -1){
						perror("While trying to set flags");
						exit(1);
					}

					// add this clients discriptor to our event_poll watcher
					event.data.fd = connectionfd;
					event.events = EPOLLIN;
					if(epoll_ctl (epoll_fd, EPOLL_CTL_ADD, connectionfd, &event) == -1){
						perror ("epoll_ctl");
						exit(1);
					}
				}
				continue;
			}
			else
			{
				

				//okay old client maybe he is trying to communicate
				memset(msg_buffer, 0, sizeof(msg_buffer));
				memset(temp_buffer, 0, sizeof(temp_buffer));

				//read everything into a buffer which client is trying to say
				while ((n = read(events[i].data.fd, msg_buffer, 1024))){
					
					if (n == -1){
					//	printf("Might be EAGAIN\n");
						break;
					}
					
				}

				
				//You need to scoll up to read these functions, we are just processing the http request/
				strcpy(temp_buffer, msg_buffer);
				if(!parse_http_request(&request_handle, temp_buffer)){
					printf("Request Unparsablen\n");
					//we have failed to parse client's request might wanna shutdown this connection.
					if (shutdown(events[i].data.fd, SHUT_RDWR) == -1){
						perror("shutdown");
					}

					continue;
				}
				
				//okay response to client
				respond(request_handle, events[i].data.fd);
				//shutdown connection and free everything
				if (shutdown(events[i].data.fd, SHUT_RDWR) == -1){
					perror("shutdown");
				}

				free(request_handle->uri);
				free(request_handle);
				//printf("%s\n", temp_buffer); 

			
			}
		}
	}



	/*

	//This was some rough loop so i'm commenting it
	while(1){
		client_length = sizeof(client_address);
		memset(msg_buffer, 0, sizeof(msg_buffer));
		memset(temp_buffer, 0, sizeof(temp_buffer));
		connectionfd = accept(listenfd, (struct sockaddr*)&client_address, &client_length);
		while( n = recv(connectionfd, msg_buffer, 1024, MSG_DONTWAIT)){

			//printf("%sn", msg_buffer);
			//printf("%zdn", strlen(msg_buffer));
			if(strstr(msg_buffer, "rnrn"))
			break;


			//if( n < 0)
			//    if(errno == EWOULDBLOCK)
			//        break;
		}
		strcpy(temp_buffer, msg_buffer);
		if(!parse_http_request(&request_handle, temp_buffer)){
			printf("Request Unparsablen");
			close(connectionfd);
			continue;
		}
		respond(request_handle, connectionfd);
		close(connectionfd);
		free(request_handle->uri);
		free(request_handle);


	}*/



	exit(0);
}

