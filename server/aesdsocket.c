#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>


#define PORT "9000"
#define LISTEN_BACKLOG 1
#define BUF_MAX_LENGTH 1024
#define DATA_FILE_PATH "/var/tmp/aesdsocketdata"


int fdDataFile;
int fdNewSocket;
int fdAcceptedSocket;
int addrAcceptedSocket;
struct addrinfo *addrInfoResult;


static void SignalHandler(int signalNumber);
int SignalActionSetup();
int CreateSocketAndListenConnection();
int AcceptConnection();
int ProcessConnection();
int ReceiveWriteReadSend();
int ReadFromFileAndSend();
int CloseConnection();
void CloseEverything();


static void SignalHandler(int signalNumber)
{
	if ((signalNumber == SIGINT) || (signalNumber == SIGTERM))
	{
		syslog(LOG_INFO, "Caught signal, exiting");

		CloseEverything();
		
		exit(0);
	}
}

int SignalActionSetup()
{
	struct sigaction sigAction = {0};
	sigAction.sa_handler = SignalHandler;
	
	if (sigaction(SIGINT, &sigAction, NULL) != 0)
	{
		// Error on sigaction
		syslog(LOG_ERR, "Error %d (%s) on sigaction for SIGINT", errno, strerror(errno));
		return -1;
	}
	if (sigaction(SIGTERM, &sigAction, NULL) != 0)
	{
		// Error on sigaction
		syslog(LOG_ERR, "Error %d (%s) on sigaction for SIGTERM", errno, strerror(errno));
		return -1;
	}
	
	return 0;
}

int CreateSocketAndListenConnection()
{
	fdNewSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (fdNewSocket == -1)
	{
		// Error on creating the socket
		syslog(LOG_ERR, "Error %d (%s) on socket", errno, strerror(errno));
		return -1;
	}
	
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
   	hints.ai_socktype = SOCK_STREAM; 
   	hints.ai_flags = AI_PASSIVE;
   	hints.ai_protocol = 0;
	addrInfoResult = NULL;
	if (getaddrinfo(NULL, PORT, &hints, &addrInfoResult) != 0)
	{
		// Error on getting network address
		syslog(LOG_ERR, "Error %d (%s) on getaddrinfo", errno, strerror(errno));
		return -1;
	}
	
	if (bind(fdNewSocket, addrInfoResult->ai_addr, addrInfoResult->ai_addrlen) != 0)
	{
		// Error on binding
		return -1;
	}
	freeaddrinfo(addrInfoResult);
	addrInfoResult = NULL;
	
	if (listen(fdNewSocket, LISTEN_BACKLOG) != 0)
	{
		// Error on listening
		syslog(LOG_ERR, "Error %d (%s) on listen", errno, strerror(errno));
		return -1;
	}
	
	return 0;
}

int AcceptConnection()
{
	struct sockaddr socketAddr = {0};
	socklen_t socketAddrLength = sizeof(socketAddr);
	fdAcceptedSocket = accept(fdNewSocket, &socketAddr, &socketAddrLength);
	if (fdAcceptedSocket == -1)
	{
		// Error on accepting
		syslog(LOG_ERR, "Error %d (%s) on accept", errno, strerror(errno));
		return -1;
	}	
	
	struct sockaddr_in *socketAddrIn;
	socketAddrIn = (struct sockaddr_in *)&socketAddr;
	addrAcceptedSocket = socketAddrIn->sin_addr.s_addr;
	syslog(LOG_INFO, "Accepted connection from %d", addrAcceptedSocket);
	
	return 0;
}

int ProcessConnection()
{
	int retVal;
	
	retVal = AcceptConnection();
	if (retVal != 0)
	{
		return retVal;
	}

	retVal = ReceiveWriteReadSend();
	if (retVal != 0)
	{
		return retVal;
	}
	
	retVal = CloseConnection();
	if (retVal != 0)
	{
		return retVal;
	}
	
	return 0;
}

int ReceiveWriteReadSend()
{
	// Receive and Write
	int retVal;
	char recvBuf[BUF_MAX_LENGTH];
	int recvBufLength, wrBufLength;
	
	do
	{
		memset(recvBuf, 0, BUF_MAX_LENGTH);
		
		// Receive
		recvBufLength = recv(fdAcceptedSocket, recvBuf, BUF_MAX_LENGTH, 0);
		if (recvBufLength < 0)
		{
			// Error on receiving
			syslog(LOG_ERR, "Error %d (%s) on recv", errno, strerror(errno));
			return -1;
		}
		syslog(LOG_INFO, "Received %d bytes: %s", recvBufLength, recvBuf);
		if (recvBufLength > 0)
		{
			// Write to file
			wrBufLength = write(fdDataFile, recvBuf, recvBufLength);
			if (wrBufLength < 0)
			{
				// Error on writing
				syslog(LOG_ERR, "Error %d (%s) on write", errno, strerror(errno));
				return -1;
			}
			syslog(LOG_INFO, "Written %d bytes", wrBufLength);
			
			// Check if the packet is complete
			if (recvBuf[recvBufLength - 1] == '\n')
			{
				// The packet is complete => Read from file and send the whole content
				syslog(LOG_INFO, "Packet complete. Starting to read from file and send");
				retVal = ReadFromFileAndSend();
				if (retVal != 0)
				{
					return retVal;
				}
			}
		}
	}
	while (recvBufLength > 0);
	
	return 0;
}

int ReadFromFileAndSend()
{
	// Read and Send
	char readBuf[BUF_MAX_LENGTH];
	int readBufLength, sendBufLength;
	
	// Move the data file offset at the beginning of the file
	if (lseek(fdDataFile, 0, SEEK_SET) < 0)
	{
		// Error on lseek
		syslog(LOG_ERR, "Error %d (%s) on lseek", errno, strerror(errno));
		return -1;
	}
	
	do
	{
		memset(readBuf, 0, BUF_MAX_LENGTH);
		
		// Read from file
	 	readBufLength = read(fdDataFile, readBuf, BUF_MAX_LENGTH);
	 	if (readBufLength < 0)
		{
			// Error on reading
			syslog(LOG_ERR, "Error %d (%s) on read", errno, strerror(errno));
			return -1;
		}
		syslog(LOG_INFO, "Read %d bytes: %s", readBufLength, readBuf);
		if (readBufLength > 0)
		{
			// Send
			sendBufLength = send(fdAcceptedSocket, readBuf, readBufLength, 0);
		 	if (sendBufLength < 0)
			{
				// Error on sending
				syslog(LOG_ERR, "Error %d (%s) on send", errno, strerror(errno));
				return -1;
			}
			syslog(LOG_INFO, "Sent %d bytes", sendBufLength);
		}
	} while (readBufLength > 0);
	
	return 0;
}

int CloseConnection()
{
	if (fdAcceptedSocket)
	{
		if (close(fdAcceptedSocket) < 0)
		{
			// Error on close
			syslog(LOG_ERR, "Error %d (%s) on close for fdAcceptedSocket", errno, strerror(errno));
			return -1;
		}
	}
	fdAcceptedSocket = 0;
	
	return 0;
}

void CloseEverything()
{
	if (fdNewSocket)
	{
		if (close(fdNewSocket) < 0)
		{
			// Error on close
			syslog(LOG_ERR, "Error %d (%s) on close for fdNewSocket", errno, strerror(errno));
		}
	}
	fdNewSocket = 0;
	
	if (fdAcceptedSocket)
	{
		if (close(fdAcceptedSocket) < 0)
		{
			// Error on close
			syslog(LOG_ERR, "Error %d (%s) on close for fdAcceptedSocket", errno, strerror(errno));
		}
	}
	fdAcceptedSocket = 0;
	
	if (remove(DATA_FILE_PATH) < 0)
	{
		// Error on remove
		syslog(LOG_ERR, "Error %d (%s) on remove", errno, strerror(errno));
	}
	
	if (fdDataFile)
	{
		if (close(fdDataFile) < 0)
		{
			// Error on close
			syslog(LOG_ERR, "Error %d (%s) on close for fdDataFile", errno, strerror(errno));
		}
	}
	fdDataFile = 0;
	
	if (addrInfoResult)
	{
		freeaddrinfo(addrInfoResult);
	}
	addrInfoResult = NULL;
	
	closelog();
}

int main(int argc, char *argv[])
{
	int retVal;
	
	// Setup SIGINT and SIGTERM signals action
	retVal = SignalActionSetup();
	if (retVal != 0)
	{
		return retVal;
	}

	// Open data file
	fdDataFile = open(DATA_FILE_PATH, O_RDWR | O_CREAT | O_APPEND, S_IRWXU | S_IRWXG | S_IRWXO);
	if (fdDataFile < 0)
	{
		syslog(LOG_ERR, "Error %d (%s) on opening the data file", errno, strerror(errno));
		return -1;
	}
	
	// Setup socket and listen for connections
	retVal = CreateSocketAndListenConnection();
	if (retVal != 0)
	{
		return retVal;
	}
	
	// Check if going into Daemon mode or not
	if ((argc > 1) && (strcmp(argv[1], "-d") == 0))
	{
		// Daemon mode
		syslog(LOG_INFO, "Continuing in daemon mode");
		
		pid_t pid = fork();
		if (pid < 0)
		{
			// Error on fork
			syslog(LOG_ERR, "Error %d (%s) on fork", errno, strerror(errno));
			return -1;
		}
		if (pid > 0)
		{
			// Parent
			syslog(LOG_INFO, "Exiting from the parent");
			return 0;
		}
		if (pid == 0)
		{
			// Child
			syslog(LOG_INFO, "Continuing from the child");
		}
	}
	
	// Loop: accept connection, receive & write to file until the packet is complete, read from file & send, close connection
	while (1)
	{
		retVal = ProcessConnection();
		if (retVal != 0)
		{
			return retVal;
		}
	}

	return 0;
}

