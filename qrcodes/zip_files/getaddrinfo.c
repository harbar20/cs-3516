#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

int main()
{
	/* server information */
    const char *host = "yahoo.com";
    const char *port = "80";
	int r;
	struct addrinfo hints,*resource;
    
	/* prep the hints structure */
	printf("Configuring host...");
	memset( &hints, 0, sizeof(struct addrinfo) );	/* use memset_s() */
	hints.ai_family = AF_INET;			/* IPv4 */
	hints.ai_socktype = SOCK_STREAM;	/* TCP */
	/* fill the resource structure */
	r = getaddrinfo(host, port, &hints, &resource);
	if( r!=0 )
	{
		perror("Failed");
		exit(1);
	}
	puts("done");

	/* free allocated memory */
	freeaddrinfo(resource);
    return(0);
}
