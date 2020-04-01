#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>

int getContentType(char* filePath);

int main(int argc, char** argv){
	//checks for valid arguments
	if(argc != 3){
		printf("Usage: ./server <port> <path to root>\n");
		return 0;
	}
	else if(atoi(argv[1]) < 1024 || atoi(argv[1]) > 65535){
		printf("Bad port: %d\n", atoi(argv[1]));
		return 0;
	}
	else if(chdir(argv[2]) != 0){
		printf("Could not change to directory: %s\n", argv[2]);
		return 0;
	}

	//creates required structs to be filled
	int port = atoi(argv[1]);
	int value = 1;
	struct sockaddr_in address;
	struct sockaddr_storage clientInfo;
	socklen_t addrlen = sizeof(clientInfo);
	struct stat fileInfo;

	//creation of sockets
	int clientSocket, serverSocket;
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));

	//actual filling of structs with info for port, endianess, and IP version
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = INADDR_ANY;
	//binds socket and port
	int bound = bind(serverSocket, (struct sockaddr*)&address, sizeof(address));
	listen(serverSocket, 1);

	//if port is not already in use...
	if(bound == 0){
		while(1){
			clientSocket = accept(serverSocket, (struct sockaddr *) &clientInfo, &addrlen);

			if(clientSocket != -1){
				printf("Got a client\n");
				char command[2048];
				int length = recv(clientSocket, command, sizeof(command)-1, 0);
				if(length > 0){
					//Both commands need from here...
					command[length] = '\0';
					char request[8];
					char filePath[1024];
					char protocol[64];
					sscanf(command, "%s %s %s", request, filePath, protocol);
					if(strcmp(request, "GET") == 0 || strcmp(request, "HEAD") == 0){
						char* updatedPath = filePath;
						int pathLength = strlen(filePath);

						//if the specified path ends in a '/', append 'index.html' to the end
						if(updatedPath[pathLength-1] == '/')
							updatedPath = strcat(updatedPath, "index.html");
						//if the path starts with a '/', ignore and check from the next character
						if(updatedPath[0] == '/')
							updatedPath++;

						int fd = open(updatedPath, O_RDONLY);

						if(fd != -1){//checks if the file descriptor is valid
							//gets info of file descriptor
							fstat(fd, &fileInfo);
							off_t fileLength = fileInfo.st_size;
							char buffer[fileLength];
							read(fd, buffer, sizeof(char) * fileLength);

							char contentType[64];
							int type = getContentType(updatedPath);
							//gets content type of the file
							switch(type){
							case 1:
								sprintf(contentType, "text/html");
								break;
							case 2:
								sprintf(contentType, "image/jpeg");
								break;
							case 3:
								sprintf(contentType, "image/gif");
								break;
							case 4:
								sprintf(contentType, "image/png");
								break;
							case 5:
								sprintf(contentType, "text/plain");
								break;
							case 6:
								sprintf(contentType, "application/pdf");
								break;
							}

							char toSend[2048];
							sprintf(toSend, "HTTP/1.0 200 OK\nContent-Length: %ld\nContent-Type: %s\n\n", fileLength, contentType);
							//sends necessary info to the client
							send(clientSocket, toSend, strlen(toSend), 0);
							//..to here

							//this way, it will send the data anyway, and only the rest if it is a GET command
							if(strcmp(request, "GET") == 0)
								send(clientSocket, buffer, sizeof(char) * fileLength, 0);

							//prints for server side
							printf("\t%s %s %s\n", request, filePath, protocol);
							printf("Sent file: %s\n\n", updatedPath);
						}

						//if file descriptor is -1
						else{
							//opens the 404.html
							int errorFD = open("404.html", O_RDONLY);
							fstat(errorFD, &fileInfo);
							off_t fileLength = fileInfo.st_size;
							char error[fileLength];

							char toSend[2048];
							sprintf(toSend, "HTTP/1.0 404 Not Found\n\n");
							send(clientSocket, toSend, strlen(toSend), 0);
							sprintf(error,"<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD><BODY><H1>Not Found</H1>The request URL %s was not found on this server.<P></P></BODY></HTML>\n",updatedPath);
							send(clientSocket, error, sizeof(char) * fileLength, 0);
							//prints for server side
							printf("\t%s %s %s\n", request, filePath, protocol);
							printf("File not found: %s\n\n", filePath);
						}
					}
					else
						printf("Unknown request: %s\n", request);
				}
			}
			close(clientSocket);
		}
	}
	//if the port specified by the user is already in use
	else
		printf("bind: Address already in use\nCould not bind to port %d(%d)\n", port, port);
	return 0;
}

/*
  function used to check extension of a file, and allowing the server to treat it as that extension
*/
int getContentType(char* filePath){
	char* extension = strrchr(filePath, '.');
	extension++;
	if(strcmp(extension, "html") == 0 || strcmp(extension, "htm") == 0)
		return 1;
	else if(strcmp(extension, "jpeg") == 0 || strcmp(extension, "jpg") == 0)
		return 2;
	else if(strcmp(extension, "gif") == 0)
		return 3;
	else if(strcmp(extension, "png") == 0)
		return 4;
	else if(strcmp(extension, "txt") == 0 || strcmp(extension, "c") == 0 || strcmp(extension, "h") == 0)
		return 5;
	else if(strcmp(extension, "pdf") == 0)
		return 6;

	return 0;
}

/*
1 = text/html
2 = image/jpeg
3 = image/gif
4 = image/png
5 = text/plain
6 = application/pdf
*/


