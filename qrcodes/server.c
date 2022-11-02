#include <stdio.h>		/* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>	/* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>		/* for atoi() and exit() */
#include <string.h>		/* for memset() */
#include <unistd.h>		/* for close() */
#include <getopt.h>		/* for getopt() */

// #define MAXPENDING 5             /* Maximum outstanding connection requests */
void DieWithError(char *errorMessage)
{
	printf("%s", errorMessage);
}; /* Error handling function */
void commandRunError(char *argv[])
{
	fprintf(stderr, "Usage:  %s -p <PORT between 2000-3000> -r <number requests> <number seconds> -m <Max Users> -t <Timeout in Seconds>\n", argv[0]);
	exit(1);
}
void HandleTCPClient(int clntSocket){
	// TODO: QR Code decoding here
}; /* TCP client handling function */

int main(int argc, char *argv[])
{

	setvbuf(stdout, NULL, _IONBF, 0); /* Forces stdout to be unbuffered, allowing printf's to work. */

	int servSock;					 /*Socket descriptor for server */
	int clntSock;					 /* Socket descriptor for client */
	struct sockaddr_in echoServAddr; /* Local address */
	struct sockaddr_in echoClntAddr; /* Client address */
	int option;
	unsigned short PORT = 2012;				 /* Server port */
	unsigned short RATE_NUMBER_REQUESTS = 3; /* Request limit for rate limit */
	unsigned short RATE_NUMBER_SECONDS = 60; /* Time limit for rate limit */
	unsigned short MAX_USERS = 3;
	unsigned short TIME_OUT = 80;

	unsigned int clntLen; /* Length of client address data structure */

	/* TODO: test input options more strictly. */
	if (argc > 10) /* Test for correct number of arguments */
	{
		commandRunError(argv);
	}

	while ((option = getopt(argc, argv, ":p:r:m:t:")) != -1) /* Get the arguments */
	{
		switch (option)
		{
		case 'p':
			if (optarg == NULL)
			{

				commandRunError(argv);
			}
			PORT = atoi(optarg);
			break;
		case 'r':
			if (!optarg)
			{

				commandRunError(argv);
			}
			RATE_NUMBER_REQUESTS = atoi(optarg);
			if (optind < argc && *argv[optind] != '-')
			{
				RATE_NUMBER_SECONDS = atoi(argv[optind]);
				optind++;
			}
			else
			{
				commandRunError(argv);
			}
			break;
		case 'm':
			if (!optarg)
			{
				commandRunError(argv);
			}
			MAX_USERS = atoi(optarg);
			break;
		case 't':
			if (!optarg)
			{
				commandRunError(argv);
			}
			TIME_OUT = atoi(optarg);
			break;
		}
	}

	printf("%d %d %d %d %d\n", PORT, RATE_NUMBER_REQUESTS, RATE_NUMBER_SECONDS, MAX_USERS, TIME_OUT); /* TODO remove this. This is just for debugging. */

	/* Create socket for incoming connections */
	if ((servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		DieWithError("socket() failed");

	/* Construct local address structure */
	memset(&echoServAddr, 0, sizeof(echoServAddr));	  /* Zero out structure */
	echoServAddr.sin_family = AF_INET;				  /* Internet address family */
	echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
	echoServAddr.sin_port = htons(PORT);			  /* Local PORT */

	/* Bind to the local address */
	if (bind(servSock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) < 0)
		DieWithError("bind() failed");

	/* Mark the socket so it will listen for incoming connections */
	if (listen(servSock, MAX_USERS) < 0)
		DieWithError("listen() failed");

	for (;;) /* Run forever */
	{
		/* Set the size of the in-out parameter */
		clntLen = sizeof(echoClntAddr); /* Wait for a client to connect */
		if ((clntSock = accept(servSock, (struct sockaddr *)&echoClntAddr, &clntLen)) < 0)
			DieWithError("accept() failed");

		/* clntSock is connected to a client! */
		printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));
		HandleTCPClient(clntSock);
	}
	/* NOT REACHED */
}
