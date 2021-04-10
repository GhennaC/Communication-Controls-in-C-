#include <stdio.h>      
#include <stdlib.h>     
#include <unistd.h>    
#include <string.h>     
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netdb.h>      
#include <arpa/inet.h>
#include "parson.h"
#include "helpers.h"
#include "requests.h"
#define PORT 8080


int main() {
	char *message;
	char *cookie=NULL,*token=NULL;
	char *response;
	int sockfd;
	while(1) {

		char *command = (char *) malloc(20);
		fgets(command,20,stdin);

		if(strncmp(command,"register",8) == 0){
			JSON_Value *val = json_value_init_object();
			JSON_Object *buff = json_value_get_object(val);
			char *username;
			username = (char *)malloc(50);
			printf("username=");
            fgets(username, 100, stdin);
            char *password;
            password = (char *)malloc(50);
            printf("password=");
            fgets(password, 100, stdin);
            strtok(username, "\n");
            strtok(password, "\n");
			json_object_set_string(buff,"username",username);
			json_object_set_string(buff,"password",password);
			char *string = json_serialize_to_string_pretty(val);
			message = compute_post_request("3.8.116.10","/api/v1/tema/auth/register","application/json",string,NULL,NULL);
			sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			if(strstr(response,"HTTP/1.1 201 Created") != 0)
				printf("Succes, account %s created!\n",username);
			else printf("Fail,please try again with another login.\n");
		} else if(strncmp(command,"login",5) == 0){
			JSON_Value *val = json_value_init_object();
			JSON_Object *buff = json_value_get_object(val);
			char *username;
			username = (char *)malloc(50);
			printf("username=");
            fgets(username, 100, stdin);
            char *password;
            password = (char *)malloc(50);
            printf("password=");
            fgets(password, 100, stdin);
            strtok(username, "\n");
            strtok(password, "\n");
			json_object_set_string(buff,"username",username);
			json_object_set_string(buff,"password",password);
			char *string = json_serialize_to_string_pretty(val);
			message = compute_post_request("3.8.116.10","/api/v1/tema/auth/login","application/json",string,NULL,NULL);
			sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			if(strstr(response,"HTTP/1.1 200 OK") != 0)
				printf("Succes,logged in!\n");
			else printf("Wrong login or password.\n");
			char *help;
			help  = strtok(response,"\n");
			while( help != NULL ){
				help = strtok(0,"\n\0 ");
				if(strncmp("Set-Cookie",help,10) == 0){
					help = strtok(help,";");
					cookie = strtok(help," ");
					cookie = strtok(NULL," ");
					break;
				}
			}
		} else if(strcmp(command,"enter_library\n") == 0){
			message = compute_get_request("3.8.116.10","/api/v1/tema/library/access",NULL,cookie,NULL);
			sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);
			send_to_server(sockfd, message);
			char *help;
			response = receive_from_server(sockfd);
			if(strstr(response,"HTTP/1.1 200 OK") != 0)
				printf("Succes,entered in library!\n");
			else printf("You are not logged in or wrong cookie.\n");
			help  = strtok(response,"\n");
			while( help != NULL ){
				if(strstr(help,"token") != NULL){
					help = help + 10;
					token = strtok(help,"\"");
					break;
				}
				help = strtok(NULL,"\n");
			}	
		} else if(strcmp(command,"get_books\n") == 0){
		 	message = compute_get_request("3.8.116.10","/api/v1/tema/library/books",NULL,NULL,token);
			sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			if(strstr(response,"HTTP/1.1 200 OK") != 0){
				char *help;
				help = strtok(response,"[");
				help = strtok(NULL,"]");
				printf("Your books are : \n%s\n",help);
			}else printf("Can not get your books.\n");
		} else if(strstr(command,"get_book") != 0){
		 	char *p = strtok(command, " ");
		 	p = strtok(NULL,"\n ");
		 	char s[50] = "/api/v1/tema/library/books/";
		 	strcat(s,p);
		 	message = compute_get_request("3.8.116.10", s , NULL, NULL, token);
			sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			if(strstr(response,"HTTP/1.1 200 OK") != 0){
				char *help;
				help = strtok(response,"[");
				help = strtok(NULL,"]");
				printf("Your book parameters are : \n%s\n",help);
			}else printf("Can not get a book with this id.\n");
		} else if(strcmp(command,"add_book\n") == 0){
			JSON_Value *val = json_value_init_object();
			JSON_Object *buff = json_value_get_object(val);
			char *title,*author,*genre,*page_count,*publisher;
			title = (char *)malloc(50);
			author = (char *)malloc(50);
			genre = (char *)malloc(50);
			page_count = (char *)malloc(50);
			publisher = (char *)malloc(50);
			printf("title = ");
            fgets(title, 100, stdin);
			printf("author = ");
			fgets(author,50,stdin);
			printf("genre = ");
			fgets(genre,50,stdin);
			printf("page_count = ");
			fgets(page_count,50,stdin);
			int i = atoi(page_count);
			printf("publisher = ");
			fgets(publisher,50,stdin);
			title = strtok(title," \n");
			author = strtok(author," \n");
			genre = strtok(genre," \n");
			publisher = strtok(publisher," \n");
			json_object_set_string(buff,"title",title);
			json_object_set_string(buff,"author",author);
			json_object_set_string(buff,"genre",genre);
			json_object_set_number(buff,"page_count",i);
			json_object_set_string(buff,"publisher",publisher);
			char *string = json_serialize_to_string_pretty(val);
			message = compute_post_request("3.8.116.10","/api/v1/tema/library/books","application/json",string,NULL,token);
			sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			if(strstr(response,"HTTP/1.1 200 OK") != 0){
				printf("Your book with title : \"%s\" is added.\n",title);
			}else printf("You did not added a field or you are not logged in.\n");
		} else if(strstr(command,"delete_book") != 0){
			char *p = strtok(command, " ");
		 	p = strtok(NULL,"\n ");
		 	char s[50] = "/api/v1/tema/library/books/";
		 	strcat(s,p);
		 	message = compute_get_request("3.8.116.10", s , "DELETE", NULL, token);
			sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			if(strstr(response,"HTTP/1.1 200 OK") != 0){
				printf("Book with id : %s was deleted.\n",p);
			}else printf("No book was deleted!\n");
		} else if(strcmp(command,"logout\n") == 0){
			message = compute_get_request("3.8.116.10", "/api/v1/tema/auth/logout" ,NULL, cookie, NULL);
			sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			cookie = NULL;
			token = NULL;
			if(strstr(response,"HTTP/1.1 200 OK") != 0)
				printf("Logged out!\n");
			else printf("You are not logged in!\n");
		} else if(strcmp(command,"exit\n") == 0){
					break;
		} else {
		 	printf("Wrong command. Enter new command.\n");
		}
	}

	close(sockfd);
}