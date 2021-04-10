#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <math.h>
#include "list.h"
#include "helpers.h"
#define BUFMAX 2000
void usage(char *file) {
    fprintf(stderr, "Usage: %s id_client server_address server_port\n", file);
    exit(0);
}

// mesajul primit de la server este impartit in ip,port
// topic,type si insusi continutul mesajul,dupa care afisate.
void messageReceived(char buffer[]){
    MessageTcp *message = (MessageTcp *)buffer;
    //ip si port
    int port = message->port;
    char *ip = message->ip;
    char topic[50];
    char *copy = message->buffer;
    // topic
    strncpy(topic,copy,50);
    char buff[BUFMAX];
    // continutul mesajului
    strcpy(buff,copy+51);
    int int_number;
    double real_number;
    // type
    int type = *(copy + 50);
    // din enunt reies aceste operatii de prelucrare
    // a continutului mesajului.
    switch(type){
        case 0:
            if(buff[0] == 0)
            int_number = ntohl(*(uint32_t *)(buff + 1));
            else int_number = (-1) * ntohl(*(uint32_t *)(buff + 1));
            printf("%s:%d - %s - INT - %d\n", ip, port, topic, int_number);
            break;
        case 1:
            real_number = ntohs(*(uint16_t *)(buff));
            real_number = real_number / 100;
            printf("%s:%d - %s - SHORT_REAL - %.2f\n",ip,port,topic,real_number);
            break;
        case 2:
            real_number = ntohl(*(uint32_t *)(buff + 1));
            real_number = real_number / pow(10 , buff[5] );
            if(buff[0] != 0)
                real_number = (-1) * real_number ;
            printf("%s:%d - %s - FLOAT - %lf\n", ip , port , topic , real_number);
            break;
        case 3:
            printf("%s:%d - %s - STRING - %s\n",ip , port , topic ,buff);
            break;
    }
}
int main(int argc, char *argv[]) {
    int sockfd, n, ret;
    struct sockaddr_in serv_addr;
    char buffer[BUFMAX];

    fd_set read_fds;
    fd_set tmp_fds;
    FD_ZERO(&tmp_fds);
    FD_ZERO(&read_fds);
	int fdmax;

    if (argc < 4) {
        usage(argv[0]);
    }
    // deschiderea socketului
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket");
    FD_SET(sockfd, &read_fds);
    FD_SET(0, &read_fds);
    int flag = 1;
    setsockopt(sockfd,IPPROTO_TCP,TCP_NODELAY,(const void*)&flag,sizeof(int));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[3]));
    ret = inet_aton(argv[2], &serv_addr.sin_addr);
    DIE(ret == 0, "inet_aton sin addr");
    // conexiune cu serverul prin portul primit ca argument
    ret = connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    DIE(ret < 0, "connect");
    // trimiterea id-ului primit ca argument.
    ret = send(sockfd, argv[1], strlen(argv[1]), 0);
    DIE(ret < 0, "send");
    fdmax = sockfd;
    while (1) {
        tmp_fds = read_fds;

        ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(ret < 0, "select");
        // se verifica daca este primita comanda exit
        // sau o alta comanda de la tastatura 
        if (FD_ISSET(0, &tmp_fds)) {
            memset(buffer, 0, BUFLEN);
            fgets(buffer, BUFLEN - 1, stdin);

            if(strncmp(buffer,"exit",4) == 0){
                close(sockfd);
                return 0;
            }
            // obtinerea comenzii,topicului si sf-ului
            // in caz contrat se afiseaza" not good input"
            char copy[BUFLEN];
            char *typeCom;
            char *topic;
            char *topic1;
            char help[5];
            int SF;
            strcpy(copy,buffer);
            typeCom = strtok(copy," \n\0");
            if(strcmp(typeCom, "subscribe") == 0 || strcmp(typeCom, "unsubscribe") == 0) {
                int err = send(sockfd, buffer, strlen(buffer), 0);
                DIE(err < 0, "send message client");
                if(strcmp(typeCom,"subscribe") == 0){
                    topic1 = strtok(NULL,"\n\0");
                    strcpy(help,topic1+strlen(topic1)-1);
                    topic = strtok(topic1," \n\0");
                    SF = atoi(help);
                    printf("subscribed %s\n",topic);
                }
                else {
                    topic1 = strtok(NULL,"\n\0");
                    strcpy(help,topic1+strlen(topic1)-1);
                    topic = strtok(topic1," \n\0");
                    printf("unsubscribed %s\n",topic);
                }
            } else {
                printf("Not good input %s",buffer);
               }
        }else {
            // se verificare daca este primit ceva prin sockfd
            // daca este primit se transmite mesajul in fucntia de sus.
            if (FD_ISSET(sockfd, &tmp_fds)) {
                int nrbytes;
            	memset(buffer, 0, BUFMAX);
                nrbytes = recv(sockfd, buffer, BUFMAX, 0);
                DIE(nrbytes <  0 , "Error receive from server.\n");
                if(nrbytes == 0)
                    break;
                messageReceived(buffer);
            }
        }
    }
    close(sockfd);
    return 0;
}