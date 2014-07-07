#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <poll.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

int main(int argc,char** argv){
	msg r, t;  //len, payload
	frame f;   //seq_nr, *payload, checksum
	
	init(HOST,PORT);

//--------------------------fisier de log---------------------------------------
	int log;
 	char message_r[40] = {"[RECEIVER] Am primit urmatorul pachet:\n"};
 	char message_ack[40] = {"[RECEIVER] Trimit ACK: "};
 	char message_nack[] = {"[RECEIVER]Am calculat checksum, nu corespunde. \
 	Deci trimit ACK pentru Seq_Nr: "};
 	char data[12], timp[10], buffer[300];
 	char new_line[60] = {"-------------------------------------------------\n"};
 	
//---------------------creez fisier de out--------------------------------------	
	char filename[50];
	/* primesc ca argument numele fisierului de out */
	strcpy(filename, argv[1]);
	/* sau scriu aici numele fisierului de out */
	//strcpy(filename, "fisier1.out");
	
	int fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	lseek(fd, 0, SEEK_SET);
	
//----------------------structura pentru data si timp---------------------------
	time_t now;
	struct tm *timeinfo;
	
//-------------------incep sa primesc mesaje------------------------------------
	char seq_nr = 0;
	char parity;
	char cs_buffer[9];
	
	srand(time(NULL));
	int deelay;
	
	while(1) {
		/* intarziere pachete la un numar random generat doar daca este 5 */
		deelay = rand() % 10 + 1;
		if(deelay == 5){
			usleep(100000);
		}
		
		if (recv_message(&r) < 0){
			perror("Receive message");
			return -1;
		}
		
		f.seq_nr = r.payload[0];
	  	f.checksum = r.payload[1];
	  	get_binary_checksum(f.checksum, cs_buffer);
	  	f.payload = (char*)malloc((r.len + 1) * sizeof(char));
	  	memset(f.payload, 0, (r.len + 1) * sizeof(char));
	  	memcpy(f.payload, r.payload + 2, r.len * sizeof(char)); 
		
		/*------------------obtin ora curenta-----------------*/		
		time(&now);
		timeinfo = localtime(&now);
		
		sprintf(data, "%02d-%02d-%02d", timeinfo->tm_mday, 1 + timeinfo->tm_mon,
		 1900 + timeinfo->tm_year);
	
		sprintf(timp, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min,
		 timeinfo->tm_sec);
		 
		/*----creez mesaj pentru fisier de log daca se primeste corect----*/
		sprintf(buffer, "[%s %s] %sSeq Nr: %d\nPayload: %s\nChecksum: %s\n%s",
		 		 data, timp, message_r, f.seq_nr, f.payload, cs_buffer, 
		 		  new_line);
		log = open("log.txt", O_WRONLY | O_APPEND);
		write(log, buffer, strlen(buffer));
		close(log);		 
		/*----------------------------------------------------*/ 
		if(r.len == 100 && f.checksum == -1) {
			f.seq_nr = seq_nr++;
			f.checksum = 255;
			get_binary_checksum(f.checksum, cs_buffer);
			f.payload = (char*)malloc(sizeof(char));
			f.payload = "";
		
			memcpy(t.payload, &f.seq_nr, sizeof(char));           
			memcpy(t.payload + 1, &f.checksum, sizeof(char));
		  	memcpy(t.payload + 2, f.payload, sizeof(char));
			
			t.len = 100;
			send_message(&t);
			break;
		}
		
		parity = calc_parity(f.payload, r.len);
		
		if(f.seq_nr == seq_nr && parity == f.checksum){
			/* scriu in fisierul de out ce am primit */
			write(fd, f.payload, r.len); 
			
			f.seq_nr = seq_nr++;
			f.checksum = parity;
			get_binary_checksum(f.checksum, cs_buffer);
			free(f.payload);
			f.payload = (char*)malloc(sizeof(char));
			f.payload = "";
			
			memcpy(t.payload, &f.seq_nr, sizeof(char));            //pun frameul in t
			memcpy(t.payload + 1, &f.checksum, sizeof(char));
		  	memcpy(t.payload + 2, f.payload, sizeof(char));

			t.len = strlen(t.payload + 1);
			send_message(&t);	
			/*------------------obtin ora curenta-----------------*/		
			time(&now);
			timeinfo = localtime(&now);
		
			sprintf(data, "%02d-%02d-%02d", timeinfo->tm_mday, 1 + timeinfo->tm_mon,
			 1900 + timeinfo->tm_year);
	
			sprintf(timp, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min,
			 timeinfo->tm_sec);
			 
			/*----creez mesaj pentru fisier de log daca se primeste corect----*/
			sprintf(buffer, "[%s %s] %s%d\n%s", data, timp, message_ack,
					 f.seq_nr, new_line);
			
			log = open("log.txt", O_WRONLY | O_APPEND);
			write(log, buffer, strlen(buffer));
			close(log);

		}
		else{
			//trimit ACK pe secventa anterioara
			sprintf(buffer, "[%s %s] %s %d\n%s",data, timp, message_nack,
					 seq_nr, new_line);
			
			log = open("log.txt", O_WRONLY | O_APPEND);
			write(log, buffer, strlen(buffer));
			close(log);
		
			/*--------------creez mesaj pentru confirmare----------*/ 
			f.seq_nr = seq_nr - 1;
			f.checksum = parity;
			get_binary_checksum(f.checksum, cs_buffer);
			
			f.payload = (char*)malloc(sizeof(char));
			f.payload = "";
		
			memcpy(t.payload, &f.seq_nr, sizeof(char));          
			memcpy(t.payload + 1, &f.checksum, sizeof(char));
		  	memcpy(t.payload + 2, f.payload, sizeof(char));
			
			t.len = strlen(t.payload + 1);
			send_message(&t);

		}	
	}
	
 //----------------------------------------------------------------------------- 
	close(fd);		
  
	return 0;
}
