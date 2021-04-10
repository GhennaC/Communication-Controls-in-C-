#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "helpers.h"
#include "list.h"
#define BUFMAX 2000
void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int main(int argc , char *argv[]) { 
	if(argc < 2 )
		usage(argv[0]);

	int tcp_sock,udp_sock,newsock;
	int ret;
	char buffer[BUFMAX];
	struct sockaddr_in serv_tcp,cl_tcp;
	struct sockaddr_in serv_udp,cl_udp;
	// initializarea parametrilor pentru 
	// deschiderea unor 2 noi conexiuni : tcp,udp
	fd_set read_fds;
	fd_set tmp_fds;
	int fdmax;
	FD_ZERO(&read_fds);
	socklen_t socklen;
	tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_sock < 0 ,"deschidere tcp_sock");

	udp_sock = socket(PF_INET, SOCK_DGRAM, 0);
	DIE(udp_sock < 0 ,"deschidere tcp_sock");

	memset((char *) &serv_tcp, 0, sizeof(serv_tcp));
	serv_tcp.sin_family = AF_INET;
	serv_tcp.sin_port = htons(atoi(argv[1]));
	serv_tcp.sin_addr.s_addr = INADDR_ANY;
	int flag = 1;
	setsockopt(tcp_sock,IPPROTO_TCP,TCP_NODELAY,(const void*)&flag,sizeof(int));
	ret = bind(tcp_sock, (struct sockaddr*)&serv_tcp,sizeof(serv_tcp));
	DIE(ret < 0 , "bind tcp");

	inet_aton("127.0.0.1", &(serv_udp.sin_addr));
	serv_udp.sin_family = AF_INET;
	serv_udp.sin_port = htons(atoi(argv[1]));
	ret = bind(udp_sock,(struct sockaddr*)&serv_udp,sizeof(serv_udp));
	DIE(ret < 0 , "bind udp");
	// serverul "asculta" comenzi de la clientii tcp in nr maxim de 10000
	ret = listen(tcp_sock,10000);
	DIE(ret < 0 , "listen err");
	FD_SET(0,&read_fds);
	FD_SET(tcp_sock, &read_fds);
	FD_SET(udp_sock,&read_fds);
	fdmax = tcp_sock;
	Topic_zip topics = NULL;
	// vector utilizat la salvarea socketului si id-ul fiecarui client
	// socketul -> indicele , id -> continutul casutei.
	int *clientId = (int *)malloc(10000*sizeof(int));
	socklen_t sizesock = sizeof(cl_tcp);
	while(1){
		tmp_fds = read_fds;
		memset(buffer,0,BUFLEN);
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0 ,"select server.\n");

		for(int i = 0 ; i <= fdmax ; i++){
			if(FD_ISSET(i, &tmp_fds)){
				// se verifica daca este introdus exit
				// de la tastatura
				if(i == 0){
					fgets(buffer,BUFLEN-1,stdin);
					if(strcmp(buffer,"exit\n") == 0){
						for(int j = 2 ; j <= fdmax ; j++){
							if(FD_ISSET(j,&read_fds)){
								close(j);
								FD_CLR(j,&read_fds);
							}	
						}		
						close(tcp_sock);
						close(udp_sock);
						return 0;
					}
				} else {
					// in cazul in care i == socketul de udp
					// se asteapta un mesaj de la client udp,care este transformat
					// in mesaj pt client tcp si apoi trimis la toti clientii ce au
					// dat subscribe la acel topic.
					if(i == udp_sock){
						ret = recvfrom(udp_sock,buffer,BUFLEN,0,(struct sockaddr*)&cl_udp,&socklen);
						DIE(ret < 0 , "recv from udp.\n");
						struct messageTcp *message = (MessageTcp*)malloc(sizeof(struct messageTcp));
						char copy[BUFLEN];
						strcpy(copy,buffer);
						char *topic;
						topic = strtok(copy," \n\0");
						strcpy(message->ip ,inet_ntoa(cl_udp.sin_addr));
						message->port = ntohs(cl_udp.sin_port);
						memcpy(&message->buffer,buffer,sizeof(buffer));
						Client clienti = getClients(topics,topic);
						while(clienti != NULL){
							ret = send(clienti->sock,(char *)message,1600,0);
							DIE(ret < 0 , "eroare trimitere mesaj client.\n");
							clienti = clienti->next;
						}
						
					} else {
						// in cazul unei conexiuni tcp inseamna ca
						// s-a conectat un client nou ,fd sau este introdus
						// in multime si afisat evenimentul.
						if(i == tcp_sock){
							sizesock = sizeof(cl_tcp);
							newsock = accept(tcp_sock, (struct sockaddr *) &cl_tcp,&sizesock);
							DIE(newsock < 0 ,"problem accept tcp.\n");
							FD_SET(newsock, &read_fds);
							if (newsock > fdmax)
								fdmax = newsock;
							ret = recv(newsock,buffer,BUFLEN-1,0);
							DIE(ret < 0 , "recv new user");
							printf("New client %s connected from %s:%d\n",buffer,inet_ntoa(cl_tcp.sin_addr),ntohs(cl_tcp.sin_port));
							clientId[newsock] = atoi(buffer);
						} else {
							// daca nu este o conexiune tcp/udp/exit
							// inseamna ca este un mesaj primit de la client
							// in cazul in care buferul este gol
							// se deconecteaza acel user
							memset(buffer,0,BUFLEN);
							ret = recv(i,buffer,sizeof(buffer),0);
							DIE(ret < 0 , "nimic primit de la client.\n");
							if(ret == 0){
								printf("Client %d disconnected.\n",clientId[i]);
								clientId[i] = 0;
								FD_CLR(i,&read_fds);
								close(i);
								
							} else {
								// in cazul in care nu este exit
								// se verifica daca este o comanda de 
								// subscribe/unsubscribe
								char copy[BUFLEN];
								strcpy(copy,buffer);
								char *topic;
								char *topic1;
								char *command;
								command = strtok(copy," ");
								if(strcmp(command,"subscribe") == 0) {
									char help[5];
									topic1 = strtok(NULL,"\n\0");
	            					strcpy(help,topic1+strlen(topic1)-1);
	            					topic = strtok(topic1," \n\0");
	           						int SF = atoi(help);
									topics = addTopic(topics,topic,clientId[i],i,SF);
								} else {
									if(strcmp(command,"unsubscribe") == 0) {
										topic = strtok(NULL," \n\0");
										topics = delClient(topics,topic,clientId[i]);
									}
								 }
							 }
						 }
						
					 }
				 }
			}
		}
	}
	close(udp_sock);
	close(tcp_sock);
	return 0;
}