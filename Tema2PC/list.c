#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"
#include "list.h"
// initializarea lista clienti
Client initClient(int id,int sock,int sf){
	Client new = NULL;
	new = (Client)malloc(sizeof(struct client));
	new->id = id;
	new->sock = sock;
	new->SF = sf;
	new->next = NULL;
	return new;
}
// initializarea lista topicuri
Topic_zip initTopic(char top[50],int id,int sock,int sf){
	Topic_zip new = NULL;
	new = (Topic_zip)malloc(sizeof(struct topic_zip));
	new->clienti = initClient(id,sock,sf);
	strcpy(new->topic,top);
	new->next = NULL;
	return new;
}
// adaugarea unui topic/client nou,daca exista topicul se adauga clientul
// in caz contrar se adauga topicul nou impreun cu clientul
Topic_zip addTopic(Topic_zip topics, char top[50], int id,int sock, int sf){
	if(topics == NULL){
		Topic_zip new = initTopic(top,id,sock,sf);
		return new;
	}
	Topic_zip head = topics;
	Topic_zip head1 = topics;
	while(head->next != NULL){
		if(strcmp(head->topic,top) == 0){
			head->clienti = addClient(head->clienti,id,sock,sf);
			return topics;
		}
		head = head->next;
	}
	if(strcmp(head->topic,top) == 0){
		head->clienti = addClient(head->clienti,id,sock,sf);
		return topics;
		}
	head->next = initTopic(top,id,sock,sf);
	return topics;
}
// adaugarea unui client nou.
Client addClient(Client clienti,int id,int sock,int sf){
	Client new = initClient(id,sock,sf);
	if(clienti == NULL)
		return new;
	new->next = clienti;
	return new;
}
// la comanda unsubscribe se sterge un client dintr-un anumit topic.
Topic_zip delClient(Topic_zip topics,char top[50],int id){
	Topic_zip head = topics;

	while(strcmp(head->topic,top) != 0)
		head = head->next;
	Client headc = head->clienti;
	if(headc->id == id){
		Client temp = headc;
		headc = temp->next;
		topics->clienti = headc;
		free(temp);
		return topics;
	}
	Client temp = headc;
	while(headc->id != id){
		temp = headc;
		headc = headc->next;
	}
	temp->next = headc->next;

	free(headc);
	return topics;
}
// se obtin toti clientii pentru un anumit topic
Client getClients(Topic_zip topics,char top[50]){
	if(topics == NULL)
		return NULL;
	Topic_zip head = topics;
	while( head != NULL){
		if(strcmp(head->topic,top) == 0)
			return head->clienti;
		head = head->next;
	}
	if(head == NULL)
		return NULL;
}

