#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

int main()
{
	int sockfd;

	/* create the socket */
	printf("Assign a socket...");
	sockfd = socket( AF_INET, SOCK_STREAM, 0 );
	if( sockfd==-1 )
	{
		perror("failed");
		exit(1);
	}
	puts("done");

	/* close the socket */
	close(sockfd);
	puts("Socket closed, done");
	return(0);
}
