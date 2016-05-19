#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

void disconnected();

int serverSock = -1;
pthread_t out, in;
int count = 0;
// 0 means outside of session, 1 means inside of session

void* outputFromServer(){

	char buffer[256];

	while(read(serverSock, buffer, 255) > 0){
		printf("%s\n", buffer);
		bzero(buffer, 256);
	}
	printf("Server has disconnected, shutting down...\n");
	pthread_detach(in);
	pthread_exit(0);
}

void* writeToServer(){
	
	char buffer[256];
	size_t length;
	
	printf("------------- Welcome to Bank -------------\n");

	while(1){
		printf("\n");
		printf("Commands:\n open <Account Name> : to open an account\n start <Account Name> : to start an account session\n debit <Amount> : to withdraw from an account\n credit <Amount> : to deposit to an account\n finish : to leave an account session\n exit : to disconnect from the server\n");
		printf("NOTE:\n You must be within an account session to credit and debit an account\n");
		fgets(buffer, 255, stdin);
		if(strncmp(buffer, "exit", 4) == 0){
			disconnected();
		}
		sleep(2);
		length = strlen(buffer);
		buffer[length - 1] = '\0';
		write(serverSock, buffer, 255);
		bzero(buffer, 256);
	}


	pthread_exit(0);
}

int main(int argc, char* argv[]){
	
	signal(SIGINT, disconnected);
	int portnum = -1;
	int monitor = -1;
	char buffer[256];
	struct sockaddr_in serverStruct;
	struct hostent* serverIP;

	if(argc < 3){
		printf("Error: not enough arguements");
		exit(0);
	}

	portnum = atoi(argv[2]);
	serverIP = gethostbyname(argv[1]);
	if(serverIP == NULL){
		printf("Error: host does not exist");
		exit(0);
	}

	// building socket
	serverSock = socket(AF_INET, SOCK_STREAM, 0);
	if(serverSock < 0){
		printf("Error: socket was not created");
	exit(0);
	}

	// initializing server struct
	bzero((char*) &serverStruct, sizeof(serverStruct));
	serverStruct.sin_family = AF_INET;
	serverStruct.sin_port = htons(portnum);
	bcopy((char *)serverIP->h_addr, (char *)&serverStruct.sin_addr.s_addr, serverIP->h_length);
	
	// connecting to server
	while(connect(serverSock, (struct sockaddr*) &serverStruct, sizeof(serverStruct))){
		printf("Waiting to connect...\n");
		sleep(3);
	}

	printf("You are connected to the server\n");
	
	//User Write Thread (OUTPUT)
	if(pthread_create(&out, NULL, outputFromServer, NULL) < 0){
		printf("Error: output thread was not created\n");
		exit(0);
	}
	

	//User Read Thread (INPUT)	
	if(pthread_create(&in, NULL, writeToServer, NULL) < 0){
		printf("Error: input thread was not created\n");
		exit(0);
	}
	
	pthread_join(out);
	pthread_join(in);

	return 0;
}

void disconnected(){
	printf("You are disconnecting...\n");
	close(serverSock);
	exit(0);
}
