#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <vector>
#include <iterator>

using namespace std;

#define NR_CLIENTS 288
#define BUFF_SIZE 2000

#define SERVER_EXIT 0
#define NO_PORT_ERR 1
#define TCP_SOCK_ERR 2
#define UDP_SOCK_ERR 3
#define TCP_BIND_ERR 4
#define UDP_BIND_ERR 5
#define TCP_LISTEN_ERR 6
#define SELECT_ERR 7

typedef struct {
	char topic[50];
	char data_type;
	char message[1500];
}UDPmessage;


typedef struct {
	char ID[11];
	char IP[16];
	uint16_t PORT;
	int sock;
	vector<char*> topics_SF0;
	vector<char*> topics_SF1;
	vector<char*> not_sent;
}TCP_client;


int main(int argc, char *argv[])
{
	int TCPsocket;
	int UDPsocket;
	int server_port;
	int result;
	int option = 1;


	// verific daca s-a dat ca parametru port-ul
	if(argc != 2){
		exit(NO_PORT_ERR);
	}else {
		server_port = atoi(argv[1]);
	}

	// configurez adresele pentru socketul de TCP
	struct sockaddr_in addr_servTCP, addr_clientTCP;

	memset((char *) &addr_servTCP, 0, sizeof(addr_servTCP));
	addr_servTCP.sin_family = PF_INET;
	addr_servTCP.sin_port = htons(server_port);
	addr_servTCP.sin_addr.s_addr = INADDR_ANY;

	// deschid socket-ul de TCP
	TCPsocket = socket(PF_INET, SOCK_STREAM, 0);
	if(TCPsocket < 0){
		exit(TCP_SOCK_ERR);
	}

	// dezactivez delay-ul pana cand se elibereaza portul pe care vreau sa ma (re)conectez
	setsockopt(TCPsocket, SOL_SOCKET, TCP_NODELAY, &option, sizeof(int));

	//asignez port
	result = bind(TCPsocket, (struct sockaddr *) &addr_servTCP, sizeof(struct sockaddr));
	if(result < 0){
		exit(TCP_BIND_ERR);
	}
	
	// pun portul de TCP in asteptarea cererilor de la clienti
	result = listen(TCPsocket, NR_CLIENTS);
	if(result < 0){
		exit(TCP_LISTEN_ERR);
	}



	// aceeasi configurare si pentru UDP
	struct sockaddr_in addr_servUDP, addr_clientUDP;

	memset((char *) &addr_servUDP, 0, sizeof(addr_servUDP));
	addr_servUDP.sin_family = PF_INET;
	addr_servUDP.sin_port = htons(server_port);
	addr_servUDP.sin_addr.s_addr = INADDR_ANY;

	// deschid socket-ul de UDP
	UDPsocket = socket(PF_INET, SOCK_DGRAM, 0);
	if(UDPsocket < 0){
		exit(UDP_SOCK_ERR);
	}

	//asignez port
	result = bind(UDPsocket, (struct sockaddr *) &addr_servUDP, sizeof(struct sockaddr));
	if(result < 0){
		exit(UDP_BIND_ERR);
	}


	socklen_t addr_len;

	fd_set allFDS;	
	fd_set activeFDS;	

	int max_fds;
	int fd;		
	

	// scriu cu zero FD-urile si adaug in multime
	// socketurile TCP, UDP si FD-ul lui STDIN
	FD_ZERO(&allFDS);
	FD_ZERO(&activeFDS);
	FD_SET(STDIN_FILENO, &allFDS);
	FD_SET(TCPsocket, &allFDS);
	FD_SET(UDPsocket, &allFDS);


	// recalculez cel mai mare FD
	max_fds = TCPsocket > UDPsocket ? TCPsocket : UDPsocket;


	// declar lista cu clienti online si offline
	vector<TCP_client*> online_TCP_clients; 
	vector<TCP_client*> offline_TCP_clients;

	
	char buffer[BUFF_SIZE];



	while (1) {
		activeFDS = allFDS; 
		
		result = select(max_fds + 1, &activeFDS, NULL, NULL, NULL);
		if(result < 0){
			exit(SELECT_ERR);
		}


		// verific care FD este activ in momentul curent
		for (fd = 0; fd <= max_fds; fd++) {

			if (FD_ISSET(fd, &activeFDS)) {

				if(UDPsocket == fd){

					// salvez in structura mesajul provenit de la un client UDP
					UDPmessage msg;
					memset(&msg, 0, sizeof(UDPmessage));

					addr_len = sizeof(addr_clientUDP);

					result = recvfrom(fd, &msg, sizeof(UDPmessage), 0, 
										(struct sockaddr *) &addr_clientUDP, &addr_len);
					if(result < 0){
						continue;
					}	

					// preiau informatiile de la acest client
					char *client_IP = inet_ntoa(addr_clientTCP.sin_addr);
					uint16_t client_PORT = ntohs(addr_clientTCP.sin_port);

					char data_type[11];
					char message[1501];


					// parsez continutul acestui pachet
					if(msg.data_type == 0) {

						sprintf(data_type, "%s", "INT");

						char minus = 0;
						uint32_t not_converted = 0;
						uint32_t converted = 0;

						memcpy(&minus, msg.message, sizeof(char));
						memcpy(&not_converted, &msg.message[1], sizeof(uint32_t));

						converted = ntohl(not_converted);

						if(minus != 0) {
							converted *= -1;
						}

						sprintf(message, "%d", converted);


					}else if(msg.data_type == 1) {

						sprintf(data_type, "%s", "SHORT_REAL");
						uint16_t not_converted = 0;
						float converted = 0;
						
						memcpy(&not_converted, msg.message, sizeof(uint16_t));
						converted = ((float) ntohs(not_converted)) / 100;

						sprintf(message, "%.2f", converted);
						
					}else if(msg.data_type == 2) {

						sprintf(data_type, "%s", "FLOAT");

						char minus = 0;
						uint8_t power = 0;
						uint32_t not_converted = 0;
						float converted = 0;


						memcpy(&minus, msg.message, sizeof(char));
						memcpy(&not_converted, &msg.message[sizeof(char)], sizeof(uint32_t));
						memcpy(&power, &msg.message[sizeof(char)+sizeof(uint32_t)], sizeof(uint8_t));
						
						converted = ((float)ntohl(not_converted));

						// inmultesc numarul cu 10^(-power)
						while(power > 0) {
							converted /= 10;
							power--;
						}


						// introduc in numar semnul
						if(minus != 0) {
							converted *= -1;
						}

						sprintf(message, "%.4f", converted);

						
					}else {

						sprintf(data_type, "%s", "STRING");
						sprintf(message, "%s", msg.message);

					}

					// scriu in format-ul dorit informatia
					memset(buffer, 0, sizeof(buffer));
					sprintf(buffer, "%s:%d - %s - %s - %s", client_IP, client_PORT,
											msg.topic, data_type, message);


					// trimit mesajul clientilor online care sunt abonati la acest topic indiferent de SF
					vector<TCP_client*>::iterator it;
					for(it = online_TCP_clients.begin(); it != online_TCP_clients.end(); it++) {

						TCP_client *client = *it;

						vector<char*>::iterator it2;
						for(it2 = client->topics_SF0.begin(); it2 != client->topics_SF0.end(); it2++) {
							
							if(strcmp(*it2, msg.topic) == 0) {
								result = send(client->sock, buffer, strlen(buffer) + 1, 0);
							}

						} 

						for(it2 = client->topics_SF1.begin(); it2 != client->topics_SF1.end(); it2++) {
							
							if(strcmp(*it2, msg.topic) == 0) {
								result = send(client->sock, buffer, strlen(buffer) + 1, 0);
							}

						}

					}


					// aloc si salvez copie a mesajului in lista not_sent a fiecarui client offline care este
					// abonat la acest topic
					for(it = offline_TCP_clients.begin(); it != offline_TCP_clients.end(); it++) {
						
						TCP_client *client = *it;

						vector<char*>::iterator it2;
						for(it2 = client->topics_SF1.begin(); it2 != client->topics_SF1.end(); it2++) {
							
							if(strcmp(*it2, msg.topic) == 0) {
								
								char *sending = (char*)calloc(1, BUFF_SIZE);
								if(sending == NULL) {
									perror("Alocare mesaj in curs de trimitere");
									continue;
								}
								
								sprintf(sending, "%s", buffer);

								client->not_sent.push_back(sending);
							}

						}

					}


					continue;

				}
				

				if (TCPsocket == fd) {

					// iau de la client IP-ul, PORT-ul si socket-ul
					addr_len = sizeof(addr_clientTCP);
					int clientSocket = accept(TCPsocket, (struct sockaddr*) &addr_clientTCP, &addr_len);

					// astept sa trimita si ID-ul
					result = recv(clientSocket, buffer, sizeof(buffer), 0);
					if(result < 0) {
						// Daca nu s-a primit ID-ul trimit mesaj gol pentru a-l ignora
						// Nu ma intereseaza daca primeste sau nu
						send(clientSocket, "exit", strlen("exit") + 1, 0);
						continue;
					}

					// verific daca mai este cineva conectat cu acelasi ID
					vector<TCP_client*>::iterator it;
					int rejected = 0;

					for(it = online_TCP_clients.begin(); it != online_TCP_clients.end(); it++) {
						
						char *ID = (*it)->ID;
						if(strcmp(ID, buffer)) {
							continue;
						}

						TCP_client* client = (*it);

						// daca da, atunci ii trimit mesaj gol petru a-l ignora
						send(clientSocket, "exit", strlen("exit") + 1, 0);
						rejected++;

						break;

					}

					if(rejected == 1) {
						continue;
					}

					int reconected = 0;

					// verific daca a mai fost conectat pana acum
					for(it = offline_TCP_clients.begin(); it != offline_TCP_clients.end(); it++) {
						
						char *ID = (*it)->ID;
						if(strcmp(ID, buffer)) {
							continue;						
						}
					
						// retin structura si il sterg din lista offline
						TCP_client* client = *it;
						offline_TCP_clients.erase(it);
						online_TCP_clients.push_back(client);


						// daca da, atunci ii suprascriu noul socket, port, IP si
						// il mut in lista de online

						client->sock = clientSocket;
						client->PORT = ntohs(addr_clientTCP.sin_port);
						sprintf(client->IP, "%s", inet_ntoa(addr_clientTCP.sin_addr));

						reconected++;

						printf("New client (%s) connected from %s:%d.\n", client->ID, client->IP, client->PORT);

						// trimit toate mesajele care au fost salvate in lista not_sent si eliberez memoria
						while(!client->not_sent.empty()) {
							
							vector<char*>::iterator it2 = client->not_sent.begin();
							char *toFree = *it2;

							result = send(client->sock, toFree, BUFF_SIZE, 0);
							if(result < 0) {
								continue;
							}

							client->not_sent.erase(it2);
							free(toFree);

						}

						// adaug si socketul acestui subscriber reconectat in multime
						FD_SET(clientSocket, &allFDS);
						max_fds = clientSocket > max_fds ? clientSocket : max_fds;
						break;

					}

					if(reconected == 1) {
						continue;
					}

					// daca se conecteaza prima data atunci ii aloc o noua structura
					TCP_client* client = (TCP_client*)calloc(1, sizeof(TCP_client));
					if(client == NULL) {
						// daca nu a reusit alocarea structurii trimit clientului exit
						send(clientSocket, "exit", strlen("exit") + 1, 0);
						continue;
					}

					// populez structura cu informatiile subscriberului
					client->sock = clientSocket;
					client->PORT = ntohs(addr_clientTCP.sin_port);
					
					sprintf(client->IP, "%s", inet_ntoa(addr_clientTCP.sin_addr));
					sprintf(client->ID, "%s", buffer);

					online_TCP_clients.push_back(client);

					printf("New client (%s) connected from %s:%d.\n", client->ID, client->IP, client->PORT);

					// adaug socketul noului subscriber in multime si recalculez cel mai mare FD
					FD_SET(clientSocket, &allFDS);
					max_fds = clientSocket > max_fds ? clientSocket : max_fds;

					continue;

				}
				

				if (STDIN_FILENO == fd) {

					// citesc de la tastatura o comanda
					fgets(buffer, 5, stdin);
					
					// daca nu am comanda exit nu fac nimic
					if(strcmp(buffer, "exit")) {
						continue;
					}

					vector<TCP_client*>::iterator it;
					for(it = online_TCP_clients.begin(); it != online_TCP_clients.end(); it++) {
						// trimit la toti clientii online mesajul "exit" pentru deconectare
						int cli_sock = (*it)->sock;

						// nu ma intereseaza ce rezultat are trimiterea
						result = send(cli_sock, "exit", strlen("exit") + 1, 0);

						printf("Client (%s) disconnected.\n", (*it)->ID);
					}

					// dezaloc memoria alocata pentru fiecare client
					while(!online_TCP_clients.empty()) {
						
						it = online_TCP_clients.begin();
						TCP_client *client = *it;

						vector<char*>::iterator it2;

						// impreuna cu cea pentru mesajele netrimise
						while(!client->not_sent.empty()) {
							
							it2 = client->not_sent.begin();
							free(*it2);
							client->not_sent.erase(it2);

						}

						// si cu topicurile cu SF0
						while(!client->topics_SF0.empty()) {
							
							it2 = client->topics_SF0.begin();
							free(*it2);
							client->topics_SF0.erase(it2);

						}						
						
						// respectiv SF1
						while(!client->topics_SF1.empty()) {
							
							it2 = client->topics_SF1.begin();
							free(*it2);
							client->topics_SF1.erase(it2);

						}

						free(client);

						// si il sterg din lista de clienti
						online_TCP_clients.erase(it);
					}

					// urmand sa inchid socketurile TCP, UDP si serverul
					close(TCPsocket);
					close(UDPsocket);

					return SERVER_EXIT;

				}

					// un client trimite date serverului
					result = recv(fd, buffer, sizeof(buffer), 0);
					if(result < 0) {
						continue;
					}


					// caut clientul dupa socket in multime
					TCP_client *client = NULL;
					vector<TCP_client*>::iterator it;
					for(it = online_TCP_clients.begin(); it != online_TCP_clients.end(); it++) {
						
						int client_sock = (*it)->sock;
						if(client_sock == fd) {
							client = *it;
							break;
						}

					}

					// clientul nu s-a gasit in lista (nu ar trebui sa intre aici)
					if(client == NULL) {
						continue;
					}

					// clientul s-a deconectat
					if (strncmp(buffer, "exit", 4) == 0) {

						// il sterg din lista de ONLINE
						online_TCP_clients.erase(it);

						// si il introduc in cea de OFFLINE
						offline_TCP_clients.push_back(client);
						printf("Client (%s) disconnected.\n", client->ID);

						// urmand sa sterg din multimea de FD socketul acestuia
						FD_CLR(fd, &allFDS);
						close(fd);

					}

					char *topic;
					int sf;

					// parsez comenzile trimise de la client
					if (strncmp(buffer, "subscribe", strlen("subscribe")) == 0) {
						
						strtok(buffer, "\n ");
						topic = strtok(NULL, "\n ");
						sf = atoi(strtok(NULL, "\n "));

						char *topic2 = (char*)malloc(strlen(topic) + 1);
						if(topic2 == NULL) {
							continue;
						}

						sprintf(topic2, "%s", topic);

						// sterg din lista topicul daca a mai fost introdus
						vector<char*>::iterator it;
						for(it = client->topics_SF1.begin(); it != client->topics_SF1.end(); it++) {
							
							char *tpc = (*it);
							
							if(strcmp(tpc, topic2) == 0) {
								client->topics_SF1.erase(it);
								it--;
							}

						}


						// la fel si aici
						for(it = client->topics_SF0.begin(); it != client->topics_SF0.end(); it++) {
							
							char *tpc = (*it);
							
							if(strcmp(tpc, topic2) == 0) {
								client->topics_SF0.erase(it);
								it--;
							}

						}

						// inserez in lista corespunzatoare in functie de SF
						if(sf == 0) {
							client->topics_SF0.push_back(topic2);
							continue;
						}else {
							client->topics_SF1.push_back(topic2);
							continue;
						}

						continue;
					}


					if (strncmp(buffer, "unsubscribe", strlen("unsubscribe")) == 0) {
						
						strtok(buffer, "\n ");
						topic = strtok(NULL, "\n ");

						int deleted = 0;

						// sterg topicul din lista cu SF1 si ies
						vector<char*>::iterator it;
						for(it = client->topics_SF1.begin(); it != client->topics_SF1.end(); it++) {
							
							char *tpc = (*it);
							if(strcmp(tpc, topic) == 0) {
								client->topics_SF1.erase(it);
								deleted++;
								break;
							}

						}

						if(deleted != 0) {
							continue;
						}

						// sau, daca nu era in lista de sus, il sterg din lista asta
						for(it = client->topics_SF0.begin(); it != client->topics_SF0.end(); it++) {
							
							char *tpc = (*it);
							if(strcmp(tpc, topic) == 0) {
								client->topics_SF0.erase(it);
								break;
							}
							
						}

						continue;
					
				}
			}
		}
	}

	return 0;
}
