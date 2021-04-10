#ifndef __LIST_H__
#define __LIST_H__
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"helpers.h"
// structura utilizata la formarea mesajului pentru tcp client
// cand primim un mesaj de la client udp.
typedef struct messageTcp {
	char ip[20];
	uint16_t port;
	char buffer[1600];
}MessageTcp;

// strct utilizata la stocarea clientilor ce s-au subscris
// la un anumit topic
typedef struct client{
	int id;
	int sock;
	int SF;
	struct client *next;
}*Client;
// Structura pentru stocarea topicurilor.
typedef struct topic_zip {
	char topic[50];
	Client clienti;
	struct topic_zip *next;
}*Topic_zip;

Client initClient(int id,int sock,int sf);
Topic_zip initTopic(char top[50],int id,int sock,int sf);
Topic_zip addTopic(Topic_zip topics, char top[50], int id,int sock, int sf);
Topic_zip delClient(Topic_zip topics,char top[50],int id);
Client addClient(Client clienti,int id,int sock,int sf);
Client getClients(Topic_zip topics,char top[50]);
#endif