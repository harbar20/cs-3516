#include <stdio.h>		/* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>	/* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>		/* for atoi() and exit() */
#include <string.h>		/* for memset() */
#include <unistd.h>		/* for close() */
#include <getopt.h>		/* for getopt() */
#include <time.h>
#include <stdbool.h>

const int MAX_FILE_SIZE = 3000; /* Maximum of 3Kb according to http://qrcode.meetheed.com/question7.php  */

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
	// Buffer for receiving the file
	unsigned char buffer[MAX_FILE_SIZE];
	// Track how many bytes of the file have been received witin the loop
	int bytesRcvd;
	// Total bytes received of the file
	int totalBytesRcvd = 0;
	// Size of the file sent by the client
	long clientFileSize;
	// File pointer for general use
	FILE *fptr;
	// Size of the text sent by the server
	int serverFileSize;
	// Text sent by the server
	char *fileContents;
	// Return code sent by the server
	int returnCode;

	// Receiving size of string
	if ((recv(clntSocket, &clientFileSize, sizeof(long), 0)) <= 0)
		DieWithError("Failed to receive string size. recv() failed or connection closed prematurely");

	while (totalBytesRcvd < clientFileSize)
	{
		if ((bytesRcvd = recv(clntSocket, buffer, clientFileSize - totalBytesRcvd, 0)) <= 0)
			DieWithError("Failed to receive image. recv() failed or connection closed prematurely");

		totalBytesRcvd += bytesRcvd; /* Keep tally of total bytes */
		buffer[bytesRcvd] = '\0';	 /* Terminate the string! */
	}

	// Saving sent bytes to a file to be read later
	fptr = fopen("server.png", "wb");
	fwrite(buffer, 1, sizeof(buffer), fptr);
	fclose(fptr);

	// Decoding the qrcode
	system("java -cp javase.jar:core.jar com.google.zxing.client.j2se.CommandLineRunner server.png > server.txt");

	// Deleting server.png
	if (remove("server.png") != 0)
		DieWithError("Deleted server.png unsuccessfully.");

	// Sending the decoded information back
	fptr = fopen("server.txt", "r");
	// Getting size of contents in file
	fseek(fptr, 0L, SEEK_END);
	serverFileSize = ftell(fptr);
	rewind(fptr);
	// Getting contents in file
	fileContents = malloc(sizeof(char) * serverFileSize);
	fread(fileContents, 1, serverFileSize, fptr);
	// Deleting server.txt
	if (remove("server.txt") != 0)
		DieWithError("Deleted server.txt unsuccessfully.");

	// Sending return code
	returnCode = 0;
	if (send(clntSocket, &returnCode, sizeof(returnCode), 0) != sizeof(returnCode))
		DieWithError("send() for the return code sent a different number of bytes than expected.");
	// Sending serverFileSize
	if (send(clntSocket, &serverFileSize, sizeof(serverFileSize), 0) != sizeof(serverFileSize))
		DieWithError("send() for the size sent a different number of bytes than expected");
	// Sending file content
	if (send(clntSocket, fileContents, serverFileSize, 0) != serverFileSize)
		DieWithError("send() for the file sent a different number of bytes than expected");

	free(fileContents);
	fclose(fptr);
};							/* TCP client handling function */
void getTime(char *outTime) /* Gets the current time for logging. */
{
	time_t timeNull = time(NULL);
	struct tm *t = localtime(&timeNull);
	snprintf(outTime, 19, "%d-%d-%d %d:%d:%d", t->tm_year + 1900, t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
	printf("%s\n", outTime);
}

void adminLog(struct sockaddr_in echoClntAddr, char *text)
{
	char finalTime[100];
	FILE *f;
	f = fopen("log.txt", "a");
	getTime(finalTime);
	char *ip = inet_ntoa(echoClntAddr.sin_addr);
	fprintf(f, "%s %s %s", finalTime, ip, text);
	fclose(f);
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

	FILE *fPtr;
	fPtr = fopen("log.txt", "w");
	fputs("Server intializing...\n", fPtr);
	fclose(fPtr);

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

	/* Create socket for incoming connections */
	if ((servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		DieWithError("socket() failed");
	adminLog(echoClntAddr, "Client socket created.");

	/* Construct local address structure */
	memset(&echoServAddr, 0, sizeof(echoServAddr));	  /* Zero out structure */
	echoServAddr.sin_family = AF_INET;				  /* Internet address family */
	echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
	echoServAddr.sin_port = htons(PORT);			  /* Local PORT */

	/* Bind to the local address */
	setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
	if (bind(servSock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) < 0)
		DieWithError("bind() failed");
	adminLog(echoClntAddr, "Client bind successful.");

	/* Mark the socket so it will listen for incoming connections */
	if (listen(servSock, MAX_USERS) < 0)
		DieWithError("listen() failed");
	adminLog(echoClntAddr, "Client listen...");

	for (;;) /* Run forever */
	{
		/* Set the size of the in-out parameter */
		clntLen = sizeof(echoClntAddr); /* Wait for a client to connect */
		if ((clntSock = accept(servSock, (struct sockaddr *)&echoClntAddr, &clntLen)) < 0)
			DieWithError("accept() failed");
		adminLog(echoClntAddr, "Client socket accepted.");

		/* clntSock is connected to a client! */
		printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));
		adminLog(echoClntAddr, "Client handling...");
		HandleTCPClient(clntSock, echoClntAddr);
		adminLog(echoClntAddr, "Client handle successful.");
	}
}
/* NOT REACHED */
