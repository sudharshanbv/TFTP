#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/ip.h> 
#include <arpa/inet.h>
#include <fcntl.h>
#include "header.h"

#define SERVER_IP       "127.0.0.1"
#define SERVER_LENGTH   10
#define SERVER_BUFF     150

unsigned long long int SERVER_PORT = 20000;

int main()
{
	int sock_fd, data_fd, bytes, flag = 0;
	struct sockaddr_in server_addr, client_addr;
	socklen_t client_len = sizeof (client_addr);
	char server_buff[SERVER_BUFF];
	char *ptr;

	ANY_t packet;

	char buff[50];

	printf("Server is waiting...\n");

	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
	server_addr.sin_port = htons(20000);
	bind(sock_fd, (struct sockaddr *) &server_addr, sizeof(server_addr));

	while (1)
	{

		recvfrom(sock_fd, &packet, sizeof (packet), 0, (struct sockaddr *) &client_addr, &client_len);
		printf("\nopcode = %d\n", packet.opcode);
		SERVER_PORT = SERVER_PORT + 1;
		switch (fork())
		{
			case 0:
				{

					int fd;
					RDWR_t rw_pk;
					ACK_t ack_pk;
					DATA_t data_pk;
					ERR_t err_pk;
					close(sock_fd);
					sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
					if (sock_fd == -1)
					{
						perror("socket : ");
						exit(2);
					}
					server_addr.sin_family = AF_INET;
					server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
					server_addr.sin_port = htons(SERVER_PORT);
					if ((bind(sock_fd, (struct sockaddr *) &server_addr, sizeof(server_addr))) == -1)
					{
						perror("bind: ");
						exit(1);
					}

					if (packet.opcode == 1)
					{
						fd = open(packet.rw_pk.file_name, O_RDONLY);
						printf("\nfile : %s\n", packet.rw_pk.file_name);
						if (fd == -1)	
						{
							err_pk.op_code_err = ERR; 
							err_pk.err_code = 1;
sendto(sock_fd, &ack_pk, sizeof (ack_pk), 0, (struct sockaddr *) &client_addr, sizeof(client_addr));
							return;
						}

						int num = 1, bytes_read;
						data_pk.op_code_dt = DAT;

						while (1)
						{
							bytes_read = read(fd, data_pk.data, sizeof (data_pk.data));
							data_pk.blk_num_dt = num++;
							sendto(sock_fd, &data_pk, bytes_read + 4, 0, (struct sockaddr *) &client_addr, sizeof(client_addr));
							recvfrom(sock_fd, &packet, sizeof (packet), 0, (struct sockaddr *) &client_addr, &client_len);

							if (packet.ack_pk.op_code_ack == ACK)
							{
								if (packet.ack_pk.blk_num_ack =! (num - 1))
								{
									printf("\nAcknowledgement failure\n");
									exit(2);
								}
							}
							printf("\nbytes_read = %d\n", bytes_read);
							if (bytes_read < 512)
							{
								printf("\nExit\n");
								exit(1);
							}
						}
					}

					else if (rw_pk.op_code_rw == 2)
					{
						ack_pk.op_code_ack = ACK; 
						ack_pk.blk_num_ack = 0;
						sendto(sock_fd, &ack_pk, sizeof (ack_pk), 0, (struct sockaddr *) &client_addr, sizeof(client_addr));
					}

					sendto(sock_fd, server_buff, SERVER_BUFF, 0, (struct sockaddr *) &client_addr, sizeof(client_addr));
					recvfrom(sock_fd, &rw_pk, sizeof (rw_pk), 0, (struct sockaddr *) &client_addr, &client_len);
					printf("The client data : %d\n%s\n\n", rw_pk.op_code_rw, rw_pk.file_name);
					return;
					if (strcmp(server_buff, "exit") == 0)
						break;

					ptr  = strtok(server_buff, " ");

					if (strstr(server_buff, "connect"))
					{
						ptr = strtok(NULL, " ");
					}

					else	if (strstr(server_buff, "get"))
					{
						ptr = strtok(NULL, " ");
					}

					else if (strstr(server_buff, "put"))
					{
						ptr = strtok(NULL, " ");
					}

					printf("Enter the data : ");
					scanf("\n%[^\n]", server_buff);
					sendto(sock_fd, server_buff, SERVER_BUFF, 0, (struct sockaddr *) &client_addr, sizeof(client_addr));

					close(sock_fd);
				}
				break;
		}
	}
	close(sock_fd);
}
