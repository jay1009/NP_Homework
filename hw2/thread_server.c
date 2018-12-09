#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
#define MAX_CLIENTS 100
#define LISTENQ 1024

static unsigned int cli_count = 0;
static int uid = 1;

/* Client strcture */
typedef struct {
	struct sockaddr_in addr;
	int connfd;
	int uid;
	char name[30];
}client_t;

client_t *clients[MAX_CLIENTS];

void queue_add(client_t *cl);
void queue_delete(int uid);
int user_fd(char *n);
void sent_message_all(char *s);
void send_message_self(const char *s, int connfd);
void send_message_client(char *s, char *n);
void send_all_clients(int connfd);
void strip_newline(char *s);
void *handle_client(void *arg);

int
main(int argc, char **argv)
{
	int listenfd, connfd;
	pid_t childpid;
	socklen_t clilen;
	struct sockaddr_in cliaddr, servaddr;
	pthread_t tid;
	char nbuff[30];

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));	//initilize the servaddr
	servaddr.sin_family      = AF_INET;	//AF_INET means IPv4 protocols
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(8700);

	signal(SIGPIPE, SIG_IGN);

	if(bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0){
		perror("Binding failed");
		return 1;
	}

	if(listen(listenfd, LISTENQ)<0){
		perror("Listening failed");
		return 1;
	}

	printf("----------Server starts----------\n");
	for ( ; ; ) {

		clilen = sizeof(cliaddr);
		if ( (connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen)) < 0) {
			if (errno == EINTR)
				continue;		/* back to for() */
			else{
				perror("server: accept error");
				exit(1);
			}
		}

		/* if clients is full */
		if((cli_count+1) == MAX_CLIENTS){
			perror("Chat room is full now");
			close(connfd);
			continue;
		}
		char nbuff[30];
		read(connfd, nbuff, sizeof(nbuff));

		/* Client */
		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->addr = cliaddr;
		cli->connfd = connfd;
		cli->uid = uid++;
		strcpy(cli->name,nbuff);

		/* Add client to the queue and create new thread */
		queue_add(cli);
		pthread_create(&tid, NULL, &handle_client, (void*)cli);

		sleep(1);
	}

}

/* Add client to queue */
void queue_add(client_t *cl){
	int i;
	for(i=0;i<MAX_CLIENTS;i++){
		if(!clients[i]){
			clients[i]=cl;
			return;
		}
	}
}

/* Delete client to queue */
void queue_delete(int uid){
	int i;
	for(i=0;i<MAX_CLIENTS;i++){
		if(clients[i]){
			if(clients[i]->uid == uid){
				clients[i] = NULL;
				return;
			}
		}
	}
}

/* Find a user's name */
int user_fd(char *n){
	int i, ufd;
	for(i=0;i<MAX_CLIENTS;i++){
		if(clients[i]){
			if(!strcmp(clients[i]->name, n)){
				ufd = clients[i]->connfd;
				return ufd;
			}
		}
	}
	return 0;
}

/* Send message to all clients */
void send_message_all(char *s){
	int i;
	for(i=0;i<MAX_CLIENTS;i++){
		if(clients[i]){
			if(write(clients[i]->connfd, s, strlen(s))<0){
				perror("write failed");
				exit(-1);
			}
		}
	}
}

/* Send message to client by connfd*/
void send_message_self(const char *s, int connfd){
	if(write(connfd, s, strlen(s))<0){
		perror("write failed");
		exit(-1);
	}
}

/* Send message to client by name */
void send_message_client(char *s, char *n){
	int i;
	for(i=0;i<MAX_CLIENTS;i++){
		if(clients[i]){
			if(!strcmp(clients[i]->name, n)){
				if(write(clients[i]->connfd, s, strlen(s))<0){
					perror("write failed");
					exit(-1);
				}
				return;
			}
		}
	}
}

/* list all clients */
void send_all_clients(int connfd){
	int i;
	char s[64];
	for(i=0; i<MAX_CLIENTS;i++){
		if(clients[i]){
			sprintf(s, " (%d) %s\r\n", i+1, clients[i]->name);
			send_message_self(s, connfd);
		}
	}
}

/* clear empty word */
void strip_newline(char *s){
	while(*s != '\0'){
		if(*s == '\r' || *s == '\n'){
			*s='\0';
		}
		s++;
	}
}

/* transform file to another client */
/*
void send_file(int send_fd, int recv_fd, char *f){
	int bytesReceived = 0;
	char recvBuff[1024];
	memset(recvBuff,'0',sizeof(recvBuff));
	write(recv_fd,f,sizeof(f));
	while((bytesReceived = read(send_fd, recvBuff, 1024))>0){
		fflush(stdout);
		write(recv_fd, recvBuff, 1024);
	}
	write(send_fd, "Success to send\r\n", 16);
	return;
}
*/

/* Handle all communication with the client */
void *handle_client(void *arg){
	char buff_out[2048];
	char buff_in[1024];
	char buff_final[1024];
	int rlen;

	cli_count++;
	client_t *cli = (client_t *)arg;
	
	printf("%s joins the chat room\r\n", cli->name);
	sprintf(buff_out, "%s joins the chat room\r\n", cli->name);
	send_message_all(buff_out);

	send_message_self("You can input \\HELP to check options\r\n", cli->connfd);

	/* Receive input from client */
	while((rlen = read(cli->connfd, buff_in, sizeof(buff_in)-1)) > 0){
	        buff_in[rlen] = '\0';
	        buff_out[0] = '\0';
		buff_final[0]='\0';

		/* Ignore empty buffer */
		strip_newline(buff_in);
		if(!strlen(buff_in)){
			continue;
		}

		/* Special options */
		if(buff_in[0] == '\\'){
			char *command, *param;
			command = strtok(buff_in," ");
			if(!strcmp(command, "\\QUIT")){	// If client input \QUIT
				break;
			}else if(!strcmp(command, "\\RENAME")){
				strcat(buff_final, "--------------------RENAME--------------------\r\n");
				param = strtok(NULL, " ");
				if(param){
					char *old_name = strdup(cli->name);
					strcpy(cli->name, param);
					sprintf(buff_out, "do RERENAME, %s -> %s\r\n", old_name, cli->name);
					strcat(buff_final, buff_out);
					free(old_name);
					printf("%s", buff_out);
					send_message_all(buff_final);
				}else{
					send_message_self("RENAME CANNOT BE NULL\r\n", cli->connfd);
				}
			}else if(!strcmp(command, "\\PRIVATE")){
				param = strtok(NULL, " ");
				char rbuf[30];
				strcpy(rbuf, param);
				if(param){
					param = strtok(NULL, " ");
					if(param){
						sprintf(buff_out, "[From %s]:", cli->name);
						while(param != NULL){
							strcat(buff_out, " ");
							strcat(buff_out, param);
							param = strtok(NULL, " ");
						}
						strcat(buff_out,"\r\n");
						send_message_client(buff_out, rbuf);
						printf("To %s -> %s", rbuf, buff_out);
					}else{
						send_message_self("MESstruct sockaddrGE CANNOT BE NULL\r\n", cli->connfd);
					}
				}else{
					send_message_self("REFERENCE CANNOT BE NULL\r\n", cli->connfd);
				}
			}else if(!strcmp(command, "\\ALL")){
				strcat(buff_final, "--------------------ALL--------------------\r\n");
				sprintf(buff_out, "Total clients: %d\r\n", cli_count);
				strcat(buff_final, buff_out);
				send_message_self(buff_final, cli->connfd);
				send_all_clients(cli->connfd);
			}else if(!strcmp(command, "\\SEND")){
				param = strtok(NULL, " ");
				char rbuf[30];
				strcpy(rbuf, param);
				if(param){
					param = strtok(NULL, " ");
					char fbuf[30];
					strcpy(fbuf, param);
					strip_newline(fbuf);
					if(param){
						char sbuf[1024];
						sprintf(sbuf, "%s asks to send file to you. input yes/no\r\n", cli->name);
						send_message_client(sbuf, rbuf);
						int recvfd;
						recvfd=user_fd(rbuf);
						char nbuff_in[10];
						read(recvfd, nbuff_in, sizeof(nbuff_in));
						strip_newline(nbuff_in);
						if(!strcmp(nbuff_in, "yes")){
							sprintf(sbuf, "%s accepts to receive\r\n", rbuf);
							send_message_self(sbuf, cli->connfd);
							send_message_client("Success to receive\r\n", rbuf);
							//send_file(cli->connfd, recvfd, fbuf);
						}else if(!strcmp(nbuff_in, "no")){
							sprintf(sbuf, "%s refuses to receive\r\n", rbuf);
							send_message_self(sbuf, cli->connfd);
							send_message_client("Success to refuse\r\n", rbuf);
						}else{
							send_message_client("UNKNOWN COMMAND\r\n", rbuf);
							send_message_self("The rreceiver doesn't respond\r\n", cli->connfd);
						}
					}else{
						send_message_self("File doesn't exist\r\n", cli->connfd);
					}
				}else{
					send_message_self("User doesn't exist\r\n", cli->connfd);
				}
			}else if(!strcmp(command, "\\HELP")){
				strcat(buff_out, "--------------------HELP--------------------\r\n");
				strcat(buff_out, "\\QUIT                        Quit chatroom\r\n");
				strcat(buff_out, "\\RENAME <name>               Change username\r\n");
				strcat(buff_out, "\\PRIVATE <name> <message>    Send private message\r\n");
				strcat(buff_out, "\\ALL                         Show all clients\r\n");
				strcat(buff_out, "\\SEND <name> <filename>      Send file to another client\r\n");
				strcat(buff_out, "\\HELP                        Show help\r\n");
				send_message_self(buff_out, cli->connfd);
			}else{
				send_message_self("UNKNOWN COMMAND\r\n", cli->connfd);
			}
		}else{
			/* Send message */
			snprintf(buff_out, sizeof(buff_out), "%s: %s\r\n", cli->name, buff_in);
			send_message_all(buff_out);
			printf("%s",buff_out);
		}
	}

	/* Close connection */
	sprintf(buff_out, "%s leaves the chat room\r\n", cli->name);
	printf("%s leaves the chat room\r\n", cli->name);
	send_message_all(buff_out);
	close(cli->connfd);

	/* Delete client from queue and yeild thread */
	queue_delete(cli->uid);
	free(cli);
	cli_count--;
	pthread_detach(pthread_self());

	return NULL;
}

