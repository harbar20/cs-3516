#include <stdio.h>		/* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>	/* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>		/* for atoi() and exit() */
#include <string.h>		/* for memset() */
#include <unistd.h>		/* for close() */
#include <getopt.h>		/* for getopt() */
#include <time.h>
#include <stdbool.h>
#include <semaphore.h>
#include <pthread.h>
#include <signal.h>

const int MAX_FILE_SIZE = 3000; /* Maximum of 3Kb according to http://qrcode.meetheed.com/question7.php  */
sem_t fileWait;
sem_t numClients;
pthread_t pt;
pthread_t threads[100];

struct arg_struct
{
	int sock;
	struct sockaddr_in address;
};

void getTime(char *outTime);
void adminLog(struct sockaddr_in clntAddr, char *text);

void sig_handler(int signum)
{
	FILE *f = fopen("log.txt", "a");
	fprintf(f, "Server timed out. Signum: %d\n", signum);
	fclose(f);
	exit(2);
}

void DieWithError(struct sockaddr_in address, char *errorMessage)
{
	printf("%s", errorMessage);
	adminLog(address, errorMessage);
	exit(1);
}; /* Error handling function */
void commandRunError(char *argv[])
{
	fprintf(stderr, "Usage:  %s -p <PORT between 2000-3000> -r <number requests> <number seconds> -m <Max Users> -t <Timeout in Seconds>\n", argv[0]);
	exit(1);
}

void *HandleTCPClient(void *args)
{
	struct arg_struct *arguments = args;
	int clntSocket = arguments->sock;
	struct sockaddr_in clntAddr = arguments->address;

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
	int returnCode = 0;

	// Receiving size of string
	if ((recv(clntSocket, &clientFileSize, sizeof(long), 0)) <= 0)
	{
		DieWithError(clntAddr, "Failed to receive string size. recv() failed or connection closed prematurely");
	}
	adminLog(clntAddr, "Received size of file from client.");
        sem_wait(&fileWait);

	while (totalBytesRcvd < clientFileSize)
	{
		if ((bytesRcvd = recv(clntSocket, buffer, clientFileSize - totalBytesRcvd, 0)) <= 0)
			DieWithError(clntAddr, "Failed to receive image. recv() failed or connection closed prematurely");

		totalBytesRcvd += bytesRcvd; /* Keep tally of total bytes */
		buffer[bytesRcvd] = '\0';	 /* Terminate the string! */
	}
	adminLog(clntAddr, "Received file contents from client.");

	// Saving sent bytes to a file to be read later
	fptr = fopen("server.png", "wb");
	fwrite(buffer, 1, sizeof(buffer), fptr);
	fclose(fptr);
	adminLog(clntAddr, "Saved file contents from client.");

	// Decoding the qrcode
	int javaCode = system("java -cp javase.jar:core.jar com.google.zxing.client.j2se.CommandLineRunner server.png > server.txt");
	if (javaCode != 0)
	{
		char *errorMessage = "Decoding the QR Code failed.";
		int errorLen = strlen(errorMessage);
		int returnCode = 1;
		send(clntSocket, &returnCode, sizeof(returnCode), 0);
		send(clntSocket, &errorLen, sizeof(errorLen), 0);
		send(clntSocket, errorMessage, errorLen, 0);
		adminLog(clntAddr, errorMessage);
	}
	adminLog(clntAddr, "Decoded file contents from client.");
	// Deleting server.png
	if (remove("server.png") != 0)
		DieWithError(clntAddr, "Deleted server.png unsuccessfully.");
	adminLog(clntAddr, "Deleted saved file contents from client.");

	// Sending the decoded information back
	fptr = fopen("server.txt", "r");
	// Getting size of contents in file
	fseek(fptr, 0L, SEEK_END);
	serverFileSize = ftell(fptr);
	rewind(fptr);
	// Getting contents in file
	fileContents = malloc(sizeof(char) * serverFileSize);
	fread(fileContents, 1, serverFileSize, fptr);
	adminLog(clntAddr, "Read results of decoded file contents from client.");
	// Deleting server.txt
	if (remove("server.txt") != 0)
		DieWithError(clntAddr, "Deleted server.txt unsuccessfully.");
	adminLog(clntAddr, "Deleted results of decoded file contents from client.");


	// Sending return code
	if (send(clntSocket, &returnCode, sizeof(returnCode), 0) != sizeof(returnCode))
		DieWithError(clntAddr, "send() for the return code sent a different number of bytes than expected.");
	adminLog(clntAddr, "Sent return code to client.");
	printf("Return code: %d\n", returnCode);
	// Sending serverFileSize
	if (send(clntSocket, &serverFileSize, sizeof(serverFileSize), 0) != sizeof(serverFileSize))
		DieWithError(clntAddr, "send() for the size sent a different number of bytes than expected");
	adminLog(clntAddr, "Sent decoded contents size to client.");
	printf("Size: %d\n", serverFileSize);
	// Sending file content
	if (send(clntSocket, fileContents, serverFileSize, 0) != serverFileSize)
		DieWithError(clntAddr, "send() for the file sent a different number of bytes than expected");
	adminLog(clntAddr, "Sent decoded contents to client.");
	printf("Contents: %s", fileContents);

	sem_post(&numClients);
	free(fileContents);
	fclose(fptr);
	sem_post(&fileWait);
	pthread_exit(NULL);
	return NULL;
};							/* TCP client handling function */
void getTime(char *outTime) /* Gets the current time for logging. */
{
	time_t timeNull = time(NULL);
	struct tm *t = localtime(&timeNull);
	snprintf(outTime, 20, "%4d-%2d-%2d %2d:%2d:%2d", t->tm_year + 1900, t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
	// printf("%s\n", outTime);
}

void adminLog(struct sockaddr_in clntAddr, char *text)
{
	char finalTime[100];
	FILE *f;
	f = fopen("log.txt", "a");
	getTime(finalTime);
	char *ip = inet_ntoa(clntAddr.sin_addr);
	int clients;
	sem_getvalue(&numClients, &clients);
	fprintf(f, "%s %s %s %d\n", finalTime, ip, text, clients);
	fclose(f);
}

int main(int argc, char *argv[])
{

	setvbuf(stdout, NULL, _IONBF, 0); /* Forces stdout to be unbuffered, allowing printf's to work. */

	signal(SIGALRM, sig_handler);

	int servSock;					 /*Socket descriptor for server */
	int clntSock;					 /* Socket descriptor for client */
	struct sockaddr_in echoServAddr; /* Local address */
	struct sockaddr_in clntAddr;	 /* Client address */
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
		DieWithError(clntAddr, "socket() failed");
	adminLog(clntAddr, "Socket created.");

	/* Construct local address structure */
	memset(&echoServAddr, 0, sizeof(echoServAddr));	  /* Zero out structure */
	echoServAddr.sin_family = AF_INET;				  /* Internet address family */
	echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
	echoServAddr.sin_port = htons(PORT);			  /* Local PORT */
	/* Bind to the local address */
	setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
	if (bind(servSock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) < 0)
		DieWithError(clntAddr, "bind() failed");
	adminLog(clntAddr, "Bind successful.");

	/* Mark the socket so it will listen for incoming connections */
	if (listen(servSock, MAX_USERS) < 0)
		DieWithError(clntAddr, "listen() failed");
	adminLog(clntAddr, "Listening...");

	sem_init(&fileWait, 0, 1);
	sem_init(&numClients, 0, MAX_USERS);
	printf("Max users: %d\n", MAX_USERS);
	int i = 0;
	for (;;) /* Run forever */
	{
		/* Set the size of the in-out parameter */
		clntLen = sizeof(clntAddr); /* Wait for a client to connect */

		alarm(TIME_OUT);
		if ((clntSock = accept(servSock, (struct sockaddr *)&clntAddr, &clntLen)) < 0)
			DieWithError(clntAddr, "accept() failed");
		signal(SIGALRM, sig_handler);
		adminLog(clntAddr, "Client socket accepted.");

		/* clntSock is connected to a client! */
		printf("Handling client %s\n", inet_ntoa(clntAddr.sin_addr));
		adminLog(clntAddr, "Client handling...");
		struct arg_struct args;
		args.sock = clntSock;
		args.address = clntAddr;
		int clients;
		sem_getvalue(&numClients, &clients);
		printf("Num clients: %d\n", 3-clients);
		if (sem_trywait(&numClients)==-1) /* MAX_USERS was exceeded */
		{
			char *errorMessage = "The server is busy.";
			int errorLen = strlen(errorMessage);
			int returnCode = 1;
			send(clntSock, &returnCode, sizeof(returnCode), 0);
			send(clntSock, &errorLen, sizeof(errorLen), 0);
			send(clntSock, errorMessage, errorLen, 0);
			adminLog(clntAddr, "Client tried to connect, but MAX_USERS was exceeded.");
			sem_post(&numClients);
		}
		else
		{
			pthread_create(&pt, NULL, &HandleTCPClient, &args);
		}
	}
}
/* NOT REACHED */
