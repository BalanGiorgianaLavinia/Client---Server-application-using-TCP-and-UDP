#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>

#define EXIT 0
#define ARGC_ERR 1
#define SERV_SOCK_ERR 2
#define INET_ATON_ERR 3
#define CONNECT_ERR 4
#define SETSOCKOPT_ERR 5
#define SELECT_ERR 6
#define ID_SENT_ERR 7

#define BUFF_SIZE 2000


int main(int argc, char *argv[])
{

	// verific sa fie fix 3 argumente
	if (argc != 4) {
		exit(ARGC_ERR);
	}


	// preiau informatiile despre subscriber
	char ID[11];
	char PORT[6];
	char IP[16];


	sprintf(ID, "%s", argv[1]);
	sprintf(IP, "%s", argv[2]);
	sprintf(PORT, "%s", argv[3]);


	int server_sock;
	int result;
	int option = 1;


	struct sockaddr_in addr_servTCP;
	fd_set allFDS;	
	fd_set activeFDS;	


	int max_fds;
	char buffer[BUFF_SIZE];


	// deschid socketul cu serverul
	server_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(server_sock < 0){
		exit(SERV_SOCK_ERR);
	}

	// dezactivez algoritmul lui Neagle
	result = setsockopt(server_sock, SOL_TCP, TCP_NODELAY, &option, sizeof(int));
	if(result < 0){
		exit(SETSOCKOPT_ERR);
	}

	// zero peste toti fd
	FD_ZERO(&allFDS);
	FD_ZERO(&activeFDS);

	// adaug in multime FD-ul STDIN si socket-ul serverului
	FD_SET(STDIN_FILENO, &allFDS);
	FD_SET(server_sock, &allFDS);

	max_fds = server_sock;


	// configurez adresa
	addr_servTCP.sin_family = AF_INET;
	addr_servTCP.sin_port = htons(atoi(PORT));

	result = inet_aton(IP, &addr_servTCP.sin_addr);
	if(result == 0){
		exit(INET_ATON_ERR);
	}


	// ma conectez la server
	result = connect(server_sock, (struct sockaddr*) &addr_servTCP, sizeof(addr_servTCP));
	if(result < 0){
		exit(CONNECT_ERR);
	}

	// trimit ID-ul catre server
	result = send(server_sock, ID, sizeof(ID), 0);
	if(result < 0){
		exit(ID_SENT_ERR);
	}



	while (1) {
		activeFDS = allFDS;

		result = select(max_fds + 1, &activeFDS, NULL, NULL, NULL);

		// verific care FD este activ in momentul curent
		if(FD_ISSET(STDIN_FILENO, &activeFDS)){

			// citesc de la tastatura o comanda
			memset(buffer, 0, sizeof(buffer));
			fgets(buffer, sizeof(buffer), stdin);

			// daca este exit atunci anunt serverul si inchid socket-ul
			if (strncmp(buffer, "exit", 4) == 0) {
				
				result = send(server_sock, "exit", strlen("exit") + 1, 0);
				close(server_sock);
				exit(EXIT);

			}


			// daca este altceva parsez comanda si verific daca este valida
			char *cmd;
			char *topic;
			char *sf;
			char string[100];

			cmd = strtok(buffer, "\n ");
			if(cmd == NULL) {
				continue;
			}


			// daca este subscribe atunci salvez topic-ul si SF-ul
			if (strncmp(cmd, "subscribe", strlen("subscribe")) == 0) {
				
				topic = strtok(NULL, "\n ");
				if(topic == NULL) {
					continue;
				}

				sf = strtok(NULL, "\n ");
				if(sf == NULL || (atoi(sf) != 0 && atoi(sf) != 1)){
					continue;
				}

				if(strtok(NULL, "\n ") != NULL) {
					continue;
				}

				// construiesc comanda care va fi trimisa catre server
				sprintf(string, "%s %s %s", cmd, topic, sf);


				// trimit comanda catre server
				result = send(server_sock, string, strlen(string) + 1, 0);
				if(result < 0) {
					continue;
				} else {

					// daca server-ul a receptionat-o pot afisa confirmarea
					printf("subscribed %s\n", topic);

				}

			}


			// la fel si pentru unsubscribe
			if (strncmp(cmd, "unsubscribe", strlen("unsubscribe")) == 0) {
				
				topic = strtok(NULL, "\n ");
				if(topic == NULL) {
					continue;
				}

				if(strtok(NULL, "\n ") != NULL) {
					continue;
				}

				sprintf(string, "%s %s", cmd, topic);

				result = send(server_sock, string, strlen(string) + 1, 0);
				if(result < 0) {
					continue;
				} else {
					printf("unsubscribed %s\n", topic);
				}

			}

			continue;
		}


		// FD-ul curent este socketul serverului
		if(FD_ISSET(server_sock, &activeFDS)) {
			
			memset(buffer, 0, sizeof(buffer));
			result = recv(server_sock, buffer, sizeof(buffer), 0);

			// daca a esuat o primire de date de la server inchid socket-ul
			// si subscriberul
			if(result < 0){
				close(server_sock);
				exit(SELECT_ERR);
			}

			// daca am primit comanda exit de la server procedez ca mai sus
			if(strncmp(buffer, "exit", 4) == 0) {
				close(server_sock);
				exit(EXIT);
			}

			// in celelalte cazuri afisez mesajul de la server deja formatat
			// conform regulilor din cerinta temei
			printf("%s\n", buffer);

			continue;
		}

		
	}

	close(server_sock);

	return 0;
}
