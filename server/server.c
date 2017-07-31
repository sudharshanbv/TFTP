/*header file inclusion*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/ip.h> 
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include "header.h"

/*Macro defination*/
//#define SERVER_IP       "192.168.1.102"
#define SERVER_IP       "127.0.0.1"
#define SERVER_LENGTH   10
#define SERVER_BUFF     150

/*global variable declaration*/
unsigned int SERVER_PORT = 20000;

int main()
{

	/*local variable declaration*/
	int sock_fd, data_fd, bytes, flag = 0, child_sock_fd, num;
	struct sockaddr_in server_addr, client_addr;
	socklen_t client_len = sizeof (client_addr);
	char server_buff[SERVER_BUFF];
	char *ptr;
	fd_set rfds;
	struct timeval tv;
	int retval;
	char buff[50];


	/*declaring a variable*/
	ANY_t packet;

	printf("Server is waiting...\n");

	/*creating the socket*/
	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
	server_addr.sin_port = htons(20000);

	/*binding the socket*/
	bind(sock_fd, (struct sockaddr *) &server_addr, sizeof(server_addr));

	while (1)
	{

		/*waiting for the packet*/
		recvfrom(sock_fd, &packet, sizeof (packet), 0, (struct sockaddr *) &client_addr, &client_len);
		//printf("\nopcode = %d\n", packet.opcode);

		/*new port number*/
		SERVER_PORT = SERVER_PORT + 1;

		/*creating a child process*/
		switch (fork())
		{

			case 0:
				{

					/*local variable declaration*/
					int fd, size, idx = 0;

					RDWR_t rw_pk;
					ACK_t ack_pk;
					DATA_t data_pk;
					ERR_t err_pk;

					/*closing the socket copy parent*/
					close(sock_fd);

					/*creating a new socket*/
					child_sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

					/*error handling*/
					if (child_sock_fd == -1)
					{
						perror("socket : ");
						exit(2);
					}

					server_addr.sin_family = AF_INET;
					server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
					server_addr.sin_port = htons(SERVER_PORT);

					/*binding the socket*/
					if ((bind(child_sock_fd, (struct sockaddr *) &server_addr, sizeof(server_addr))) == -1)
					{
						perror("bind: ");
						exit(1);
					}
					//while(1)
					//{
					//	if (idx != 0)
					//		recvfrom(sock_fd, &packet, sizeof (packet), 0, (struct sockaddr *) &client_addr, &client_len);
					/*if the opcode is 1, i.e "get"*/
					if (packet.opcode == 1)
					{

						/*open the file in read mode*/
						fd = open(packet.rw_pk.file_name, O_RDONLY);
						//	printf("\nfile : %s\n", packet.rw_pk.file_name);

						/*error handling*/
						if (fd == -1)	
						{

							/*set the error opcode*/
							err_pk.op_code_err = ERR;

							if (errno == EACCES)
							{
								err_pk.err_code = 2;
								sprintf(err_pk.err_msg, "Permission Denied to Open file on Server");
							}

							else if (errno == EDQUOT || errno == ENOENT)
							{
								printf("\nfile does not exist\n");
								err_pk.err_code = 1;
								sprintf(err_pk.err_msg, "File Does Not Exist On Server");
							}

							/*send the error packet*/
							sendto(child_sock_fd, &err_pk, sizeof (err_pk), 0, (struct sockaddr *) &client_addr, sizeof(client_addr));

							exit(1);
						}

						/*local variable declaration*/
						int num = 1, bytes_read;
						data_pk.op_code_dt = DAT;

						while (1)
						{

							/*read from the file*/
							bytes_read = read(fd, data_pk.data, sizeof (data_pk.data));

							/*set the block number*/
							data_pk.blk_num_dt = num++;


							while (1)
							{
								/*send the data packet*/
								sendto(child_sock_fd, &data_pk, bytes_read + 4, 0, (struct sockaddr *) &client_addr, sizeof(client_addr));


								/*setting the time to 1 sec for simple select*/
								tv.tv_sec = 1;
								tv.tv_usec = 0;
								/*select*/
								FD_ZERO(&rfds);
								FD_SET(child_sock_fd, &rfds);
								retval = select(child_sock_fd + 1, &rfds, NULL, NULL, &tv);

								/*error handling*/
								if (retval == -1)
									perror("select:");

								/*receive acknowledgement*/
								else if (retval)
									recvfrom(child_sock_fd, &packet, sizeof (packet), 0, (struct sockaddr *) &client_addr, &client_len);

								/*if select time violates*/
								else
								{
									printf("\nAcknowledgement not received in specified time\n");
									continue;
								}

								/*check for acknowledgement packet*/
								if (packet.ack_pk.op_code_ack == ACK)
								{
									/*if the acknowledgement packet mismatches, send again*/
									if (packet.ack_pk.blk_num_ack != (num - 1))
										continue;

									else
										break;
								}
							}
							//	printf("\nbytes_read = %d\n", bytes_read);

							/*if the bytes_read is less than 512*/
							if (bytes_read < 512)
							{
								close (fd);
								break;
							}
						}
					}

					/*if opcode is 2, i.e "put"*/
					else if (packet.opcode == 2)
					{
						/*set the ack packet*/
						ack_pk.op_code_ack = ACK; 
						ack_pk.blk_num_ack = 0;
						num = 1;
						flag  = 0;

						/*send the acknowledgement packet*/
						sendto(child_sock_fd, &ack_pk, sizeof (ack_pk), 0, (struct sockaddr *) &client_addr, sizeof(client_addr));

						/*open the file*/
						fd = open(packet.rw_pk.file_name, O_CREAT | O_EXCL | O_WRONLY, 00777);
						//printf("\nfile : %s\n", packet.rw_pk.file_name);

						/*error handling*/
						if (fd == -1)
						{
							err_pk.op_code_err = ERR; 

							if (errno == EEXIST)
							{
								err_pk.err_code = 7;
								sprintf(err_pk.err_msg, "File Already Exists On Server");
							}

							/*send error packet*/
							sendto(child_sock_fd, &ack_pk, sizeof (ack_pk), 0, (struct sockaddr *) &client_addr, sizeof(client_addr));

							exit(1);
						}

						while (1)
						{

							/*setting the time to 1 sec for simple select*/
							tv.tv_sec = 1;
							tv.tv_usec = 0;

							/*select*/
							FD_ZERO(&rfds);
							FD_SET(child_sock_fd, &rfds);

							retval = select(child_sock_fd + 1, &rfds, NULL, NULL, &tv);

							/*error handling*/
							if (retval == -1)
								perror("select:");

							/*receive from client*/
							if (retval)
								size = recvfrom(child_sock_fd, &packet, sizeof (packet), 0, (struct sockaddr *) &client_addr, &client_len);

							else 
							{
								/*send the acknowledgement packet*/
								sendto(child_sock_fd, &ack_pk, sizeof (ack_pk), 0, (struct sockaddr *) &client_addr, sizeof(client_addr));
								continue;	
							}

							/*if err packet*/
							if (packet.opcode == ERR)
							{
								printf("\n%s\n", packet.err_pk.err_msg);
								break;
							}

							/*if data packet*/
							if (packet.opcode == DAT)
							{

								/*set the acknowledgement packet*/
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
								/*send the acknowledgement packet*/
								sendto(child_sock_fd, &ack_pk, sizeof (ack_pk), 0, (struct sockaddr *) &client_addr, sizeof(client_addr));
								/*write the contents to file*/
								if (flag == 0)
									write(fd, packet.data_pk.data, size - 4);

								if (flag == 1)
								{
									flag = 0;
									continue;
								}
							}

							if ((size - 4) < 512)
							{
								close(fd);
								break;
							}
						}
					}
					//					++idx;
					//}
					close(child_sock_fd);
					exit(1);
				}
				//	break;
		}
	}
	close(sock_fd);
}
