#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct s_data{
	int sockfd, port, id, connectedClients, connMax;
	fd_set readfds;
	socklen_t len;
	struct sockaddr_in servaddr, cli;
}t_data;

typedef struct s_clients{
	int connfd, id;
}t_clients;

char	*str_join(char *buf, char *add);
int		find_nl(char* s);
void	serverLaunch(t_data* data);
void	printLogin(t_clients* client, int clientID, int connMax);
void	printLogout(t_clients* client, int clientID, int connMax);
void	writeToClients(t_clients* client, char* message, int clientID, int connMax);
void	ft_run(t_data* data, t_clients* client);
void	ft_accept(t_data* data, t_clients* client);
void	ft_recv(t_data* data, t_clients* client);

void	ft_run(t_data* data, t_clients* client){

	while(1){
		if(data->connectedClients == 0)
			data->id = 0;
		FD_ZERO(&data->readfds);
		FD_SET(data->sockfd, &data->readfds);

		int maxfd = data->sockfd;
		for(int i = 0; i < data->connMax; i++){

			if(client[i].connfd != -1){
				FD_SET(client[i].connfd, &data->readfds);
				if(client[i].connfd > maxfd)
					maxfd = client[i].connfd;
			}
		}
		if(select(maxfd +1, &data->readfds, NULL, NULL, NULL) < 0){
			printf("Error select\n"); // TO DEL
		}
		ft_accept(data, client);
		ft_recv(data, client);
	}
}

void	ft_accept(t_data* data, t_clients* client){

	if(FD_ISSET(data->sockfd, &data->readfds)){

		data->len = sizeof(data->cli);
		int new = accept(data->sockfd, (struct sockaddr *)&data->cli, &data->len);
		if(new < 0){
			printf("accept failed\n"); // TO DEL
		}
		else{
			int i = 0;
			for( ; i < data->connMax; i++){
				if(client[i].connfd == -1){
					client[i].id = data->id;
					data->id++;
					client[i].connfd = new;
					break;
				}
			}
			if(i < data->connMax){
				data->connectedClients++;
				printLogin(client, client[i].id, data->connMax);
			}
			else{
				close(new);
			}
		}
	}
}

void	ft_recv(t_data* data, t_clients* client){

	for(int i = 0; i < data->connMax; i++){

		if (client[i].id != -1 && FD_ISSET(client[i].connfd, &data->readfds)){

			int bufferSize = 4096;
			int bytesRead;
			char buffer[bufferSize];
			char* message = calloc(1, sizeof(char));
			while(1){

				bytesRead = 0;
				bytesRead = recv(client[i].connfd, buffer, sizeof(buffer), 0);
				buffer[bytesRead] = '\0';
				message = str_join(message, buffer);
				memset(buffer, 0, bufferSize);

				if(bytesRead < 0){
					printf("Error recv\n"); // TO DEL
				}
				if(bytesRead == 0){
					writeToClients(client, message, client[i].id, data->connMax);
					free(message);
					printLogout(client, client[i].id, data->connMax);
					close(client[i].connfd);
					client[i].id = -1;
					client[i].connfd = -1;
					data->connectedClients--;
					break;
				}
				if(message[strlen(message) -1] == '\n'){
					break;
				}
			}
			if(bytesRead > 0){
				writeToClients(client, message, client[i].id, data->connMax);
				free(message);
			}
		}
	}
}

int find_nl(char* s){
	int i = 0;
	while(s[i]){
		if (s[i] == '\n')
			return 1;
		i++;
	}
	return 0;
}

void	serverLaunch(t_data* data){

	data->sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (data->sockfd == -1) { 
		printf("socket creation failed...\n"); // TO DEL
		exit(0); 
	} 
	else
		printf("Socket successfully created..\n"); // TO DEL
	bzero(&data->servaddr, sizeof(data->servaddr)); 

	data->servaddr.sin_family = AF_INET; 
	data->servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	data->servaddr.sin_port = htons(data->port); 
  
	if ((bind(data->sockfd, (const struct sockaddr *)&data->servaddr, sizeof(data->servaddr))) != 0) { 
		
		printf("socket bind failed...\n"); // TO DEL 
		
		write(2, "Fatal error\n", 12);
		exit(1); 
	} 
	else
		printf("Socket successfully binded..\n"); // TO DEL
}

void	printLogin(t_clients* client, int clientID, int connMax){

	char mess[50];
	sprintf(mess, "server: client %d just arrived\n", clientID);
	for(int i = 0; i < connMax; i++){
		if(client[i].id != -1 && client[i].id != clientID){
			send(client[i].connfd, mess, strlen(mess), 0);
		}
	}
	memset(mess, 0, 50);
}
void	printLogout(t_clients* client, int clientID, int connMax){

	char mess[50];
	sprintf(mess, "server: client %d just left\n", clientID);
	for(int i = 0; i < connMax; i++){
		if(client[i].id != -1 && client[i].id != clientID){
			send(client[i].connfd, mess, strlen(mess), 0);
		}
	}
	memset(mess, 0, 50);
}

void	writeToClients(t_clients* client, char* message, int clientID, int connMax){

	if(find_nl(message) == 0)
		return;
	
	char prefix[50];
	sprintf(prefix, "client %d: ", clientID);
	for(int i = 0; i < connMax; i++){
		if(client[i].id != -1 && client[i].id != clientID){

			int j = 0;
			while(message[j]){
				
				send(client[i].connfd, prefix, strlen(prefix), 0);
				while(message[j] != '\n'){
					send(client[i].connfd, &message[j], 1, 0);
					j++;
				}
				send(client[i].connfd, "\n", 1, 0);
				j++;
				if(find_nl(message + j) == 0)
					break;
			}
		}
	}
	memset(prefix, 0, 50);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

int main(int ac, char** av) {
	
	if(ac != 2){
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}

	t_data data;
	data.id = 0;
	data.connectedClients = 0;
	data.connMax = 100;
	data.port = atoi(av[1]);
	if(data.port < 1024 || data.port > 65535){

		write(2, "Fatal error\n", 12);
		exit(1);	
	}
	t_clients client[data.connMax];
	for (int i = 0; i < data.connMax; i++){
		client[i].id = -1;
		client[i].connfd = -1;
	}
	serverLaunch(&data);

	if (listen(data.sockfd, data.connMax) != 0) {
		
		printf("cannot listen\n"); // TO DEL
		
		write(2, "Fatal error\n", 12);
		exit(1);	
	}
	ft_run(&data, client);
	return 0;
}
