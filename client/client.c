
/*header file inclusion*/
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/ip.h> 
#include <arpa/inet.h>
#include <fcntl.h>
#include "header.h"

/*MACRO defination*/
#define SERVER_PORT      20000
#define CLIENT_BUFF      150

/*main function block*/
int main()
{

	/*local variable declaration*/
	int sock_fd, data_fd, buff_len, flag = 0, conn_flag = 0, size, idx = 0, num = 1;
	unsigned long long int bytes_wr;
	struct sockaddr_in server_addr;
	char client_buff[CLIENT_BUFF], *ptr;
	fd_set rfds;
	struct timeval tv;
	int retval;

	/*socket creation*/
	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

	socklen_t server_len = sizeof(server_addr);
	server_addr.sin_family = AF_INET;
	//	server_addr.sin_port = htons(SERVER_PORT);

	RDWR_t rw_pk;
	ERR_t err_pk;
	ACK_t ack_pk;
	DATA_t data_pk;
	ANY_t packet;

	int fd;

	while (1)
	{
		/*prompt*/
		printf("< tftp >  ");
		scanf("\n%[^\n]", client_buff);

		ptr = strtok(client_buff, " ");

		/*if connect*/
		if (strstr(client_buff, "connect"))
		{
			/*error handling*/
			if (conn_flag == 1)
			{
				printf("\nAlready connected to server\n");
				continue;
			}

			ptr = strtok(NULL, " ");

			/*assigning the server address*/
			server_addr.sin_addr.s_addr = inet_addr(ptr);
			conn_flag = 1;
			continue;
		}

		/*if get*/
		else if (strstr(client_buff, "get"))
		{
			bytes_wr = 0;

			/*error handling*/
			if (conn_flag == 0)
			{
				printf("\nPlease connect to a server\n");
				continue;
			}

			ptr = strtok(NULL, " ");

			strcpy(rw_pk.file_name, ptr);

			printf("\nFile Name : %s\n", ptr);

			/*open the file*/
			fd = open(rw_pk.file_name, O_CREAT | O_EXCL | O_WRONLY, 00777);

			/*error handling*/
			if (fd == -1)
			{
				if (errno == EEXIST)
				{
					printf("\nFile Already Exists\n");
					continue;
				}
			}

			/*set the opcode*/
			rw_pk.op_code_rw = RRQ;

			server_addr.sin_port = htons(SERVER_PORT);
			/*send the read packet*/
			sendto(sock_fd, (void *)&rw_pk, sizeof (rw_pk), 0, (struct sockaddr *)&server_addr, sizeof (server_addr));


			while (1)
			{

				tv.tv_sec = 1;
				tv.tv_usec = 0;

				FD_ZERO(&rfds);
				FD_SET(sock_fd, &rfds);

				retval = select(sock_fd + 1, &rfds, NULL, NULL, &tv);

				if (retval == -1)
					perror("select:");


				/*reveive the packet from the server*/
				else if (retval)
					size = recvfrom(sock_fd, (void *)&packet, sizeof (packet), 0, (struct sockaddr *) &server_addr, &server_len);

				else
				{
					if	(sendto(sock_fd, (void *)&ack_pk, sizeof (ack_pk), 0, (struct sockaddr *)&server_addr, sizeof (server_addr)) == -1)
						continue;
				}

				/*if error packet*/
				if (packet.opcode == ERR)
				{
					printf("\n%s\n", packet.err_pk.err_msg);
					break;
				}

				/*if data packet*/
				if (packet.opcode == DAT)
				{
					printf("Received data packet block %d    ", packet.data_pk.blk_num_dt);

					ack_pk.op_code_ack = ACK;

					if (packet.data_pk.blk_num_dt == num)
					{
						ack_pk.blk_num_ack = packet.data_pk.blk_num_dt;
						++num;
					}

					else if (packet.data_pk.blk_num_dt == (num - 1))
					{
						ack_pk.blk_num_ack = packet.data_pk.blk_num_dt;
						flag = 1;
					}

					printf("Sending Acknowledgement packet\n");

					/*sending acknowledgement*/
					if (idx != 6)
						if	(sendto(sock_fd, (void *)&ack_pk, sizeof (ack_pk), 0, (struct sockaddr *)&server_addr, sizeof (server_addr)) == -1)
						{
							perror("sendto:");
							exit(1);
						}
					++idx;					
					/*writing the content to file*/
					if (flag == 0)
						bytes_wr += 	write(fd, packet.data_pk.data, size - 4);

					if (flag == 1)
					{
						flag = 0;
						continue;
					}
				}

				if ((size - 4) < 512)
				{
					printf("\nfile has been received\nNumber of bytes received %Ld\n\n", bytes_wr);
					close(fd);
					break;
				}
			}
		}

		/*if "put"*/
		else if (strstr(client_buff, "put"))
		{
			/*error handling*/
			if (conn_flag == 0)
			{
				printf("\nPlease connect to a server\n");
				continue;
			}

			ptr = strtok(NULL, " ");

			strcpy(rw_pk.file_name, ptr);

			printf("\nFile Name : %s\n", ptr);

			/*open the file*/
			fd = open(rw_pk.file_name, O_RDONLY);

			if (fd == -1)
			{
				if (errno == EACCES)
				{
					printf("\nPermission denied to open file in client\n");
					continue;
				}

				else if (errno == ENOENT || errno == EDQUOT)
				{
					printf("\nFile does not exixt in client in client\n");
					continue;
				}
			}

			rw_pk.op_code_rw = WRQ;

			server_addr.sin_port = htons(SERVER_PORT);
			/*send the request packet*/
			sendto(sock_fd, (void *)&rw_pk, sizeof (rw_pk), 0, (struct sockaddr *)&server_addr, sizeof (server_addr));

			/*receive the acknowledgement*/
			recvfrom(sock_fd, (void *)&packet, sizeof (packet), 0, (struct sockaddr *) &server_addr, &server_len);

			if(packet.opcode == ACK)
			{
				if(packet.ack_pk.blk_num_ack != 0)
				{
					printf("\nAcknowledgement failure from server for write\n");
					continue;
				}
			}

			int num = 1, bytes_read;
			data_pk.op_code_dt = DAT;

			while (1)
			{
				/*read from file*/
				bytes_read = read(fd, data_pk.data, sizeof (data_pk.data));
				data_pk.blk_num_dt = num++;

				while (1)
				{
					printf("Sending %d data packet    ", num - 1);

					/*send the data packet*/
					sendto(sock_fd, (void *)&data_pk, bytes_read + 4, 0, (struct sockaddr *)&server_addr, sizeof (server_addr));

					tv.tv_sec = 1;
					tv.tv_usec = 0;

					FD_ZERO(&rfds);
					FD_SET(sock_fd, &rfds);

					retval = select(sock_fd + 1, &rfds, NULL, NULL, &tv);

					if (retval == -1)
						perror("select:");


					/*receive the acknowledgement*/
					if (retval)
						recvfrom(sock_fd, (void *)&packet, sizeof (packet), 0, (struct sockaddr *) &server_addr, &server_len);

					else
					{
						printf("\nAcknowledgement not received in specified time\n");
						continue;
					}
					if (packet.ack_pk.op_code_ack == ACK)
					{
						printf("Received Acknowledgement packet\n");
						if (packet.ack_pk.blk_num_ack != (num - 1))
						{
							printf("\nAcknowledgement failure\n");
							break;
						}
					}
				}
				if (bytes_read < 512)
				{
					printf("\nfile has been sent to server\n");
					break;
				}
			}

		}

		else if (strcmp(client_buff, "bye") == 0)
		{
			close(sock_fd);
			exit(1);
		}

	}
}
