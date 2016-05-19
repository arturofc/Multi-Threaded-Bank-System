#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include "sorted-list.h"

// bank server //

typedef struct Customer{
	char name[100];
	float balance;
	int insession;
} Customer;

typedef struct Bank{
	SortedListPtr accounts;
	int activeAccts;
} Bank;

Bank* bank;

void disconnected();

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int serverSock = -1;
int clientSock = -1;
int connections = 0;

int compareAccts(void* acct1, void* acct2){
	
	Customer* acc1 = (Customer*)acct1;
	Customer* acc2 = (Customer*)acct2;
	if(strcmp(acc1->name, acc2->name) == 0){
		return 0;
	}
	
	return 1;
}

void destroyAccts(){
	return;
}
int initializeBank(){
	bank = (Bank*)malloc(sizeof(Bank));
	bank->accounts = SLCreate(compareAccts, destroyAccts);
	bank->activeAccts = 0;
	return 1;
}

void* printAccts(){
	
	while(1){
		Node* ptr = bank->accounts->head;
		if(ptr == NULL){
			continue;
		}
		Customer* cust = ptr->data;
		sleep(20);
		pthread_mutex_lock(&lock);
		printf("Bank Status:\n -------------- \n");
		while(ptr != NULL){
			cust = ptr->data;
			printf("Name: %s\n", cust->name);
			printf("\n");
			if(cust->insession == 1){
				printf("INSESSION\n");
			}else{
				printf("Balance: %0.2f\n", cust->balance);
			}
			ptr = ptr->next;
		}
		printf("done\n");
		pthread_mutex_unlock(&lock);
	}
	pthread_exit(0);
}

void openAccount(char* name){
	
	int status = 1;
	
	// if bank is full
	if(bank->activeAccts == 20){
		write(clientSock, "** Server: Bank is at capacity : 20, no longer accepting new accounts **", 72);
		return;
	}
	
	Customer* newCust = malloc(sizeof(Customer));
	strcpy(newCust->name, name);
	newCust->balance = 0;
	newCust->insession = 0;
			
	pthread_mutex_lock(&lock);
	status = SLInsert(bank->accounts, (void*)newCust);
	pthread_mutex_unlock(&lock);
	
	if(status == 0){
		//inform client account if failed to create
		write(clientSock, "** Server: Failed to create account, account with this name may already exist **", 80);
	}else{
		write(clientSock, "** Server: Account was created **", 33);
		bank->activeAccts++;
	}
	
	return;
}

int startAccount(char* name){
	
	int status = 0;
	Node* ptr = bank->accounts->head;
	if(ptr == NULL){
		write(clientSock, "** Server: The client does not exist, please open the account before starting a session **", 90);
		return 0;	
	}
	Customer* cust = ptr->data;
	while(ptr != NULL){
		cust = ptr->data;
		if(strcmp(cust->name, name) == 0){
			if(cust->insession == 1){
				write(clientSock, "** Server: The account is currently in session and can not be edited **", 71);
				return 0;
			}
			status = 1;
			break;
		}
		ptr = ptr->next;
	}
	
	if(status == 0){
		write(clientSock, "** Server: The client does not exist, please open the account before starting a session **", 90);
		return 0;	
	}
	
	write(clientSock, "** Server: Account Session Started **", 37);
	cust->insession = 1;
	return 1;
}

//function for debit starts here

void credit(char* name, float numberAdd){
		Node* ptr = bank->accounts->head;
		Customer* cust= ptr->data;
		
	while(ptr != NULL){
		cust = ptr->data;
		if (strcmp(cust->name, name) == 0){
			pthread_mutex_lock(&lock);
			cust->balance = cust->balance + numberAdd;
			pthread_mutex_unlock(&lock);
			write(clientSock, "** Server: Money added Sucessfully \n **", 40);
		}
		
		ptr=ptr->next;
	}

	return ;

}

void finish(char* name){

	Node* ptr = bank->accounts->head;
	Customer* cust = ptr->data;
	while(ptr != NULL){
		cust = ptr->data;
		if(strcmp(cust->name, name) == 0){
			cust->insession = 0;
			return;
		}
		ptr = ptr->next;
	}
	
	return;
	
}

void debit(char* name, float numberAdd){

	Node* ptr = bank->accounts->head;
	Customer* cust= ptr->data;
	
	while(ptr != NULL){
		cust = ptr->data;
		if (strcmp(cust->name, name) == 0){
			if (cust->balance - numberAdd <0){
				write(clientSock, "** Taking out too much funds **\n", 33);
				return ;
			} else {
			pthread_mutex_lock(&lock);
			cust->balance = cust->balance - numberAdd;
			pthread_mutex_unlock(&lock);
			write(clientSock,"** Server: Money widthdrawn Sucessfully \n **", 45);
			}
		}

		ptr=ptr->next;
	}

	return ;

}

void* clientHandle(){

	char buffer[256];
	int status = 0;
	
	while(recv(clientSock, buffer, sizeof(buffer), 0) > 0){
		if(strncmp(buffer, "open ", 5) == 0){
			char* name = &buffer[5];
			if(strlen(name) <= 0){
				write(clientSock, "** Server: enter a valid name **\n", 32);
				continue;
			}
			openAccount(name);
		}else if(strncmp(buffer, "start ", 6) == 0){
			char* storeName = (char*)malloc(sizeof(buffer));
			char* name = &buffer[6];
			strcpy(storeName, name);
			pthread_mutex_lock(&lock);
			status = startAccount(name);
			pthread_mutex_unlock(&lock);
			if(status == 0){
				continue;
			}
			while(recv(clientSock, buffer, sizeof(buffer), 0) > 0){
				if(strncmp(buffer, "open ", 5) == 0){
					write(clientSock, "** Server: Can not open a new account in session **\n", 53);
				}else if(strncmp(buffer, "start ", 6) == 0){
					write(clientSock, "** Server: Can not start a session while in session **\n", 58);
				}else if(strncmp(buffer, "finish", 6) == 0){
						pthread_mutex_lock(&lock);
						finish(storeName);
						pthread_mutex_unlock(&lock);
						write(clientSock, "** Server: Session was ended **\n", 35);
						break;
				}else if(strncmp(buffer, "credit ", 7) == 0){
					char* amount = &buffer[7];
					float numAmt = atof(amount);
					credit(storeName, numAmt);
				}else if(strncmp(buffer, "debit ", 6) == 0){
					char* amount = &buffer[6];
					float numAmt = atof(amount);
					debit(storeName, numAmt);					
				}else{
					write(clientSock, "** Server: Please enter a valid Command **\n", 44);
				}
			}
		}else{
			write(clientSock, "** Server: either open, start, or exit command **\n", 51);
		}
		bzero(buffer, 256);
	}
	close(clientSock);
	printf("Client %d has disconnected\n", connections);
	connections--;
	//printAccts();
	pthread_exit(0);
}

void* session(){	
	int clientSize = -1;
	pthread_t tid[20];
	struct sockaddr_in clientStruct;
	
	// finds client connection and connects
	
	// listening for 20 client connections
	listen(serverSock, 20);
	printf("Waiting for connection...\n");
	clientSize = sizeof(clientStruct);
		
	while(1){
		clientSock = accept(serverSock, (struct sockaddr *) &clientStruct, &clientSize);
		connections++;
		pthread_t tid[connections];
		printf("Client %d has connected\n", connections);
		if(pthread_create(&tid[connections], NULL, clientHandle, NULL) < 0){
			printf("Error: thread was not created\n");
			pthread_exit(0);
		}
			
	}
	
	pthread_exit(0);
}

int main(int argc, char* argv[]){
	
	signal(SIGINT, disconnected);
	
	initializeBank();
	int portnum = -1;
	int enableReuse = 1;
	char buffer[256];

	struct sockaddr_in serverStruct;

	portnum = atoi(argv[1]);

	serverSock = socket(AF_INET, SOCK_STREAM, 0);
	if(serverSock < 0){
		printf("Error: socket failed\n");
		pthread_exit(0);
	}

	// intializing sever address struct
	bzero((char*) &serverStruct, sizeof(serverStruct));
	serverStruct.sin_port = htons(portnum);
	serverStruct.sin_family = AF_INET;
	serverStruct.sin_addr.s_addr = INADDR_ANY;

	if(setsockopt(serverSock,SOL_SOCKET,SO_REUSEADDR,&enableReuse ,sizeof(int)) < 0){
		printf("Error: Fail to reuse socket\n");
		exit(0);
	}

	// server -- port binding
	if(bind(serverSock, (struct sockaddr *) &serverStruct, sizeof(serverStruct)) > 0){
		printf("Error: server failed to bind to specificied port");
		pthread_exit(0);
	}

	pthread_t timePrint,sess;
	
	if(pthread_create(&sess, NULL, session, NULL) < 0){
		printf("Error: failed to create session acceptor thread\n");
		exit(0);
	}
	
	if(pthread_create(&timePrint, NULL, printAccts, NULL) < 0){
		printf("Error: failed to create a timed status thread\n");
		exit(0);
	}
	
	pthread_join(sess, 0);
	//pthread_join(timePrint, 0);
	
	return 0;
}

void disconnected(){
	//printAccts();
	printf("Server is disconnecting...\n");
	close(clientSock);
	close(serverSock);
	exit(0);
}
