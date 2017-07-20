#ifndef TFTP
#define TFTP

#define RRQ 1
#define WRQ 2
#define DAT 3
#define ACK 4
#define ERR 5

#pragma pack(1)

char *req;

typedef struct Rd_Wr
{
	unsigned short int op_code_rw;
	char file_name[50];
	char mode[50];
}RDWR_t;

typedef struct data
{
	unsigned short int op_code_dt;
	unsigned short int blk_num_dt;
	char data[512];
}DATA_t;

typedef struct ack
{
	unsigned short int op_code_ack;
	unsigned short int blk_num_ack;
}ACK_t;

typedef struct err
{
	unsigned short int op_code_err;
	unsigned short int err_code;
	char *err_msg;
}ERR_t;

typedef union 
{
	unsigned short int opcode;
RDWR_t rw_pk;
	DATA_t data_pk;
	ACK_t ack_pk;
	ERR_t err_pk;
} ANY_t;

#endif
