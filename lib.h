#ifndef LIB
#define LIB

#define TYPE1		1
#define TYPE2		2
#define TYPE3		3
#define TYPE4		4
#define ACK_T1		"ACK(TYPE1)"
#define ACK_T2		"ACK(TYPE2)"
#define ACK_T3		"ACK(TYPE3)"

#define MSGSIZE		1400
#define PKTSIZE		1396

#include <poll.h>

struct pollfd fds[1];

typedef struct {
	char seq_nr;
	char *payload;
	char checksum;	
} frame;

typedef struct {
  	int len;
  	char payload[MSGSIZE];
} msg;

typedef struct {
	int type;
	char payload[PKTSIZE];	
} my_pkt;

/* returneaza checksum-ul unui string */
char calc_parity(char* msg, int len){ 
	char p = 0;
	int i;
	for(i = 0; i < len; i++)
		p = p ^ msg[i];
	return p;
}
/* calculez checksum-ul pe 8 biti pentru scriere in log.txt */
void get_binary_checksum(char checksum, char cs_buffer[9]) {
	int i;
	for(i = 0; i < 8; i++) {
		cs_buffer[i] = (checksum & (1 << i)) ? '1' : '0';
	}
	cs_buffer[8] = '\0';
}

void init(char* remote,int remote_port);
void set_local_port(int port);
void set_remote(char* ip, int port);
int send_message(const msg* m);
int recv_message(msg* r);
//msg* receive_message_timeout(int timeout);

/* functie pentru receive_message cu timeout */
int recv_message_timeout(int timeout, msg *message) {
	int ret = poll(fds, 1, timeout);
	
	if (ret > 0) {
		if (fds[0].revents & POLLIN) 
			return recv_message(message); 
	}
	return ret; 
}


#endif

