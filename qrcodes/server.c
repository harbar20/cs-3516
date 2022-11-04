#include <stdio.h>		/* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>	/* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>		/* for atoi() and exit() */
#include <string.h>		/* for memset() */
#include <unistd.h>		/* for close() */
#include <getopt.h>		/* for getopt() */
#include <time.h>
#include <stdbool.h>

// #define MAXPENDING 5             /* Maximum outstanding connection requests */
void DieWithError(char *errorMessage)
{
	printf("%s", errorMessage);
	exit(1);
}; /* Error handling function */
void commandRunError(char *argv[])
{
	fprintf(stderr, "Usage:  %s -p <PORT between 2000-3000> -r <number requests> <number seconds> -m <Max Users> -t <Timeout in Seconds>\n", argv[0]);
	exit(1);
}
void HandleTCPClient(int clntSocket, struct sockaddr_in clntAddr)
{
	/* Receive the same string back from the server */
	char buffer[32];
	//char finalBuffer[32];
	int bytesRcvd;
	int totalBytesRcvd = 0; /* Count of total bytes received     */
	int bufferLen;
	uint16_t stringSize;
	int stringLen;
	int intBytes = 0;
	while (intBytes < sizeof(uint16_t)){
	
		if((recv(clntSocket, &stringSize, sizeof(uint16_t)-1, 0)) <=0)
		{
			DieWithError("Failed to receive string size. recv() failed or connection closed prematurely\n");
		}
		intBytes++;
	}
	printf("Received strsize: %d\n", stringSize);
	stringLen = htons(stringSize);
	printf("Received strlen: %d\n", stringLen);
	printf("Received: "); /* Setup to print the echoed string */
	while (totalBytesRcvd < stringLen)
	{
		/* Receive up to the buffer size (minus 1 to leave space for
		   a null terminator) bytes from the sender */
		bufferLen = strlen(buffer) - 1;
		if ((bytesRcvd = recv(clntSocket, buffer, bufferLen, 0)) <= 0)
		{
			printf("%d\n", bytesRcvd);
			DieWithError("recv() failed or connection closed prematurely");
			exit(1);
		}

		totalBytesRcvd += bytesRcvd; /* Keep tally of total bytes */
		buffer[bytesRcvd] = '\0';	 /* Terminate the string! */
		//finalBuffer[bytesRcvd] = buffer[0];
		printf("%s", buffer);		 /* Print the echo buffer */
	}
	//finalBuffer[totalBytesRcvd] = '\0';
	printf("\n");

	printf("buffer: %s\n", buffer);
	printf("stringlen: %d\n", stringLen);
	//printf("finalbuffer: %s\n", finalBuffer);

	//send(clntSocket, buffer, strlen(buffer), 0);
	/* Send the string to the server */
	if (send(clntSocket, buffer, stringLen, 0) != stringLen) {
		DieWithError("send() sent a different number of bytes than expected");
		exit(1);
	} else {
		printf("Sent: %s\n", buffer);
	}

	// TODO: QR Code decoding here
};				/* TCP client handling function */
char *getTime() /* Gets the current time for logging. */
{
	time_t t;
	time(&t);

	char *finalTime;
	// snprintf(finalTime, )

	return ctime(&t);
}

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
	const int MAX_SIZE = 3000; /* Maximum of 3Kb according to http://qrcode.meetheed.com/question7.php  */

	FILE *fPtr;
	fPtr = fopen("log.txt", "w");
	printf("%s", getTime());
	fputs("Server intializing...\n", fPtr);

	unsigned int clntLen; /* Length of client address data structure */

	// /* TODO: test input options more strictly. */
	// if (argc > 10) /* Test for correct number of arguments */
	// {
	// 	commandRunError(argv);
	// }

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
	setsockopt(servSock,SOL_SOCKET,SO_REUSEADDR, &(int){1}, sizeof(int));
	if (bind(servSock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) < 0)
		DieWithError("bind() failed");

	/* Mark the socket so it will listen for incoming connections */
	if (listen(servSock, MAX_USERS) < 0)
		DieWithError("listen() failed");

	fputs("Server intialized\n", fPtr);
	fclose(fPtr);

	for (;;) /* Run forever */
	{
		/* Set the size of the in-out parameter */
		clntLen = sizeof(echoClntAddr); /* Wait for a client to connect */
		if ((clntSock = accept(servSock, (struct sockaddr *)&echoClntAddr, &clntLen)) < 0)
			DieWithError("accept() failed");

		/* clntSock is connected to a client! */
		printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));
		HandleTCPClient(clntSock, echoClntAddr);
	}
}
	/* NOT REACHED */
