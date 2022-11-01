#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

int main()
{
	struct addrinfo hints,*server;
	int r,sockfd,clientfd;
	struct sockaddr client_address;
	socklen_t client_len;

	/* configure the host */
	printf("Configuring host...");
	memset( &hints, 0, sizeof(struct addrinfo) );	/* use memset_s() */
	hints.ai_family = AF_INET;			/* IPv4 connection */
	hints.ai_socktype = SOCK_STREAM;	/* TCP, streaming */
	/* connection with localhost (zero) on port 8080 */
	r = getaddrinfo( 0, "8080", &hints, &server );
	if( r!= 0 )
	{
		perror("failed");
		exit(1);
	}
	puts("done");

	/* create the socket */
	printf("Assign a socket...");
	sockfd = socket(
			server->ai_family,		/* domain, TCP here */
			server->ai_socktype,	/* type, stream */
			server->ai_protocol		/* protocol, IP */
			);
	if( sockfd==-1 )
	{
		perror("failed");
		exit(1);
	}
	puts("done");

	/* bind - name the socket */
	printf("Binding socket...");
	r = bind(sockfd,
			server->ai_addr,
			server->ai_addrlen
			);
	if( r==-1 )
	{
		perror("failed");
		exit(1);
	}
	puts("done");

	/* listen for incoming connections */
	printf("Listening...");
	r = listen(sockfd,1);
	if( r==-1 )
	{
		perror("failed");
		exit(1);
	}
	puts("done");

	/* accept a new connection */
	printf("Accepting new connection ");
	client_len = sizeof(client_address);
	clientfd = accept(sockfd,
			&client_address,
			&client_len
			);
	if( clientfd==-1 )
	{
		perror("failed");
		exit(1);
	}
	printf("on file descriptor %d\n",clientfd);

	/* close the client socket */
	close(clientfd);

	/* free allocated memory */
	freeaddrinfo(server);
	/* close the socket */
	close(sockfd);
	puts("Socket closed, done");
	return(0);
}
