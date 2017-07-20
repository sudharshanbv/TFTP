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

#define SERVER_PORT      20000
#define CLIENT_BUFF      150

int main()
{
	int sock_fd, data_fd, buff_len, flag = 0, conn_flag = 0, size;
	unsigned long long int bytes_wr;
	struct sockaddr_in server_addr;
	char client_buff[CLIENT_BUFF], *ptr;
	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
   socklen_t server_len = sizeof(server_addr);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);

	RDWR_t rw_pk;
	ERR_t err_pk;
	ACK_t ack_pk;
	DATA_t data_pk;
	ANY_t packet;

	int fd;

	while (1)
	{
		printf("< tftp >  ");
		scanf("\n%[^\n]", client_buff);

		ptr = strtok(client_buff, " ");

		if (strstr(client_buff, "connect"))
		{
			if (conn_flag == 1)
			{
				printf("\nAlready connected to server\n");
				continue;
			}
			ptr = strtok(NULL, " ");
			server_addr.sin_addr.s_addr = inet_addr(ptr);
			conn_flag = 1;
			continue;
		}

		else if (strstr(client_buff, "get"))
		{
			bytes_wr = 0;
			if (conn_flag == 0)
			{
				printf("\nPlease connect to a server\n");
				continue;
			}
			ptr = strtok(NULL, " ");
			strcpy(rw_pk.file_name, ptr);
			printf("\nFile Name : %s\n", ptr);
			rw_pk.op_code_rw = RRQ;

			sendto(sock_fd, (void *)&rw_pk, sizeof (rw_pk), 0, (struct sockaddr *)&server_addr, sizeof (server_addr));

			fd = open(rw_pk.file_name, O_CREAT | O_WRONLY, 00777);
			if (fd == -1)
			{
				printf("\nFile cannot be opened in client\n");
			return 0;
			}

			while (1)
			{
				size = recvfrom(sock_fd, (void *)&packet, sizeof (packet), 0, (struct sockaddr *) &server_addr, &server_len);
				
				if (packet.opcode == ERR)
				{
					printf("\n%s\n", packet.err_pk.err_msg);
					break;
				}
			
				if (packet.opcode == DAT)
				{
					printf("Received data packet block %d    ", packet.data_pk.blk_num_dt);
			
				bytes_wr += 	write(fd, packet.data_pk.data, size - 4);
					ack_pk.op_code_ack = ACK;
					ack_pk.blk_num_ack = data_pk.blk_num_dt;
					
					printf("Sending Acknowledgement packet\n");
					
					sendto(sock_fd, (void *)&ack_pk, sizeof (ack_pk), 0, (struct sockaddr *)&server_addr, sizeof (server_addr));
				}
if ((size - 4) < 512)
{
printf("\nfile has been received\nNumber of bytes received %Ld\n\n", bytes_wr);
close(fd);
break;
}
			}
		}

		else if (strstr(client_buff, "put"))
		{
			if (conn_flag == 0)
			{
				printf("\nPlease connect to a server\n");
				continue;
			}
			ptr = strtok(NULL, " ");
			strcpy(rw_pk.file_name, ptr);
			printf("\nFile Name : %s\n", ptr);
			rw_pk.op_code_rw = WRQ;
			sendto(sock_fd, (void *)&rw_pk, sizeof (rw_pk), 0, (struct sockaddr *)&server_addr, sizeof (server_addr));

		}

		if (strcmp(client_buff, "exit") == 0)
			break;
	}
	close(sock_fd);
}
