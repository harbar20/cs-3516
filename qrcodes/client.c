#include <stdio.h>		/* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>	/* for sockaddr_in and inet_addr() */
#include <stdlib.h>		/* for atoi() and exit() */
#include <string.h>		/* for memset() */
#include <unistd.h>		/* for close() */

#define RCVBUFSIZE 32 /* Size of receive buffer */

void DieWithError(char *errorMessage)
{ /* Error handling function */
	printf("%s\n", errorMessage);
	exit(1);
}
int main(int argc, char *argv[])
{
	int sock;					   /* Socket descriptor */
	struct sockaddr_in servAddr;   /* Echo server address */
	unsigned short servPort;	   /* Echo server port */
	char *servIP;				   /* Server IP address (dotted quad) */
	char *echoString;			   /* String to send to echo server */
	char echoBuffer[RCVBUFSIZE];   /* Buffer for echo string */
	unsigned int echoStringLen;	   /* Length of string to echo */
	int bytesRcvd, totalBytesRcvd; /* Bytes read in single recv() and total bytes read */
	long fileSize;
	char *fileName;

	if ((argc < 3) || (argc > 4)) /* Test for correct number of arguments */
	{
		fprintf(stderr, "Usage: %s <Server IP> <Echo Word> [<Echo Port>]\n",
				argv[0]);
		exit(1);
	}
	servIP = argv[1];	/* First arg: server IP address (dotted quad) */
	fileName = argv[2]; /* Second arg: name of file to send */

	if (argc == 4)
		servPort = atoi(argv[3]); /* Use given port, if any */
	else
		servPort = 7; /* 7 is the well-known port for the echo service */

	/* Create a reliable, stream socket using TCP */
	if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		DieWithError("socket() failed");
		exit(1);
	}
	/* Construct the server address structure */
	memset(&servAddr, 0, sizeof(servAddr));		  /* Zero out structure */
	servAddr.sin_family = AF_INET;				  /* Internet address family */
	servAddr.sin_addr.s_addr = inet_addr(servIP); /* Server IP address */
	servAddr.sin_port = htons(servPort);		  /* Server port */
	/* Establish the connection to the echo server */
	if (connect(sock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
		DieWithError("connect() failed");

	FILE *fptr = fopen(fileName, "rb");
	// Getting size of contents in file
	fseek(fptr, 0L, SEEK_END);
	fileSize = ftell(fptr);
	rewind(fptr);
	// Getting contents in file
	unsigned char *fileContents = malloc(sizeof(unsigned char) * fileSize);
	fread(fileContents, 1, ++fileSize, fptr);

	// Sending fileSize
	if (send(sock, &fileSize, sizeof(fileSize), 0) != sizeof(fileSize))
		DieWithError("send() the size sent a different number of bytes than expected");
	// Sending file contents
	if (send(sock, fileContents, fileSize, 0) != fileSize)
		DieWithError("send() the file sent a different number of bytes than expected");

	free(fileContents);

	// Receiving return code
	int returnCode;
	if ((recv(sock, &returnCode, sizeof(int), 0)) <= 0)
		DieWithError("Failed to receive return code. recv() failed or connection closed prematurely");

	printf("Return code: %d\n", returnCode);
	// Receiving size of string
	int returnSize;
	if ((recv(sock, &returnSize, sizeof(int), 0)) <= 0)
		DieWithError("Failed to receive return size. recv() failed or connection closed prematurely");

	printf("Return size: %d\n", returnSize);

	totalBytesRcvd = 0;	  /* Count of total bytes received     */
	char *buffer;
	printf("Received: "); /* Setup to print the echoed string */
	while (totalBytesRcvd < returnSize)
	{
		/* Receive up to the buffer size (minus 1 to leave space for
		   a null terminator) bytes from the sender */
		if ((bytesRcvd = recv(sock, buffer, returnSize - totalBytesRcvd, 0)) <= 0)
		{
			DieWithError("recv() failed or connection closed prematurely");
			exit(1);
		}
		totalBytesRcvd += bytesRcvd;  /* Keep tally of total bytes */
		buffer[bytesRcvd] = '\0'; /* Terminate the string! */
		printf("%s", buffer);	  /* Print the echo buffer */
	}
	/* send string to be displayed*/

	// TODO keep echo server stuff in

	// echoStringLen = strlen(echoString); /* Determine input length */
	// /* Send number of string characters to server */
	// if (send(sock, &echoStringLen, sizeof(echoStringLen), 0) != sizeof(echoStringLen))
	// {
	// 	DieWithError("send() sent a different number of bytes than expected");
	// 	exit(1);
	// }
	// /* Send the string to the server */
	// if (send(sock, echoString, echoStringLen, 0) != echoStringLen)
	// {
	// 	DieWithError("send() sent a different number of bytes than expected");
	// 	exit(1);
	// }
	// /* Receive the same string back from the server */
	// totalBytesRcvd = 0;	  /* Count of total bytes received     */
	// printf("Received: "); /* Setup to print the echoed string */
	// while (totalBytesRcvd < echoStringLen)
	// {
	// 	/* Receive up to the buffer size (minus 1 to leave space for
	// 	   a null terminator) bytes from the sender */
	// 	if ((bytesRcvd = recv(sock, echoBuffer, RCVBUFSIZE - 1, 0)) <= 0)
	// 	{
	// 		DieWithError("recv() failed or connection closed prematurely");
	// 		exit(1);
	// 	}
	// 	totalBytesRcvd += bytesRcvd;  /* Keep tally of total bytes */
	// 	echoBuffer[bytesRcvd] = '\0'; /* Terminate the string! */
	// 	printf("%s", echoBuffer);	  /* Print the echo buffer */
	// }
	// /* send string to be displayed*/

	printf("\n"); /* Print a final linefeed */
	close(sock);
	exit(0);
}
