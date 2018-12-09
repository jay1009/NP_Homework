#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#define MAX_SIZE 50

void *thread_recv(void *arg);
void *thread_send(void *arg);

int main(int argc, char **argv){
	int listenfd;
	struct sockaddr_in servaddr;
	if(argc!=2){
		perror("Please input UserName");
		exit(1);
	}

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family	 = AF_INET;
	servaddr.sin_port	 = htons(8700);

	inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr.s_addr);

	if(connect(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0){
		perror("Failed to connect to server");
		return -1;
	}
	write(listenfd, argv[1], sizeof(argv[1]));
	
	printf("Success to connect to server\n");
	pthread_t tid1, tid2;
	pthread_create(&tid1, NULL, thread_recv, &listenfd);
	pthread_create(&tid2, NULL, thread_send, &listenfd);
	pthread_join(tid1, NULL);
	close(listenfd);

	return 0;
}
/*
void read_file(int arg){
	int connfd = arg;
	FILE *fp = fopen(read_filename, "ab");
	int bytesReceived = 0;
	char recvBuff[1024];
	memset(recvBuff,'0',sizeof(recvBuff));

	while((bytesReceived = read(connfd, recvBuff, 1024))>0){
		fflush(stdout);
		fwrite(recvBuff,1,bytesReceived,fp);
	}
	printf("Received success\n");
	return;
}
*/
/*
void send_file(int arg){
	int connfd = arg;
	FILE *fp = fopen(send_filename, "rb");
	if(fp==NULL){
		perror("File open error");
		return;
	}
	printf("-----Open file-----\n");
	unsigned char buff[1024];
	memset(buff,'0',sizeof(buff));
	int nread;
	while(nread = fread(buff,1,1024,fp)>0){
		write(connfd,buff,nread);
		memset(buff,'0',sizeof(buff));
	}
	printf("Success send\n");
	return;
}
*/

void *thread_recv(void *arg){
	int serv = *(int *)arg;
	char s[1024];
	int rlen = 0;
	char *ptr=NULL;
	while(1){
		memset(s,0,sizeof(s));
		if(rlen = read(serv, s, sizeof(s))<0){
			perror("Read error");
			exit(1);
		}
		printf("%s", s);
		
/*		if(!strcmp(s,"OK")){
			send_file(serv);
		}
		if(!strcmp(s,"Wait")){
			read_file(serv);
		}
*/		
	}
	pthread_detach(pthread_self());
	return NULL;
}

void *thread_send(void *arg){
	int serv = *(int *)arg;
	char s[1024];
	int slen = 0;
	char *ptr=NULL;
	while(1){
		memset(s,0,sizeof(s));
		read(STDIN_FILENO, s, sizeof(s));

		write(serv, s, sizeof(s));
		
/*		if(ptr==strstr(s,"\\SEND")){
			if(ptr){
				ptr=strtok(s," ");
				if(ptr){
					ptr=strtok(NULL, " ");
					strcpy(send_filename,ptr);
					printf("filename:%s\n", send_filename);
				}
			}
			ptr=NULL;
		}
*/		
	}
	pthread_detach(pthread_self());
	return NULL;
}






