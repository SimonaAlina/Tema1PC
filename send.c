#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <poll.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10000

int main(int argc,char** argv){

 	init(HOST,PORT);
 	
 	msg t, r;    //len, payload
 	frame f;      //seq_nr, *payload, checksum

	int log = open("log.txt", O_CREAT | O_WRONLY | O_TRUNC, 0644);
	lseek(log, 0, SEEK_SET);
 	close(log);
 	
//---------------------mesaje ajutatoare pentru log.txt------------------------- 	
 	char message_t[40] = {"[SENDER] Am trimis urmatorul pachet:\n"};
 	char cronometru[40] ={"[SENDER] Am pornit cronometrul!\n"};
 	char data[12], timp[10], buffer[500];
 	char new_line[60] = {"-------------------------------------------------\n"};
  
//------------------deschid fisier pentru citire date---------------------------
	char filename[50];
	strcpy(filename, argv[1]);
	int fd = open(filename, O_RDONLY);
	lseek(fd, 0, SEEK_SET);
	
	/* dimensiunea fisierului */
	struct stat s;
	fstat(fd, &s);
	//int size = s.st_size; 
	
//--------------------creez primul pachet de date-------------------------------
	srand(time(NULL));
	int x = rand() % 60 + 1;
	
	char cs_buffer[9];
	int ret;
	
	f.payload = (char*)malloc((x+1) * sizeof(char)); //aloc memorie
	memset(f.payload, 0, (x+1) * sizeof(char));
	int temp = read(fd, f.payload, x);			     //citesc din fisier
	
	f.seq_nr = 0;								     //nr de secventa
	char copy_seq = 0;
	f.checksum = calc_parity(f.payload, temp); //checksum-ul
	get_binary_checksum(f.checksum, cs_buffer);
	
	/* pun frame-ul in t */
	memcpy(t.payload, &f.seq_nr, sizeof(char));           
	memcpy(t.payload + 1, &f.checksum, sizeof(char));
  	memcpy(t.payload + 2, f.payload, temp);
	
	/* lungime text trimis */	
	t.len = temp;
	
	/* creez mesaj pentru fisierul de log */
	time_t now;
	struct tm *timeinfo;
	
	time(&now);
	timeinfo = localtime(&now);
	
	sprintf(data, "%02d-%02d-%02d", timeinfo->tm_mday, 1 + timeinfo->tm_mon,
		 1900 + timeinfo->tm_year);
	
	sprintf(timp, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min,
		 timeinfo->tm_sec);
		 
	sprintf(buffer, "[%s %s] %sSeq Nr: %d\nPayload: %s\nChecksum: %s\n%s",
			data, timp, message_t, f.seq_nr, f.payload, cs_buffer, new_line);
			
	
//------------------------trimit restul fisierului------------------------------	
	while(1) { 
		
		send_message(&t);					//trimit mesaj
		
		/* scriu in log ca am trimis un pachet */
		log = open("log.txt", O_WRONLY | O_APPEND);
		write(log, buffer, strlen(buffer));
		close(log);
		
		/* actualizez ora pornirii cronometrului */
		time(&now);					
		timeinfo = localtime(&now);
	
		sprintf(data, "%02d-%02d-%02d", timeinfo->tm_mday, 1 + timeinfo->tm_mon,
		 		1900 + timeinfo->tm_year);
	
		sprintf(timp, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min,
			 timeinfo->tm_sec);
			 
		sprintf(buffer, "[%s %s] %s%s", data, timp, cronometru, new_line);
		
		/* scriu in log.txt ca se porneste cronometrul */
		log = open("log.txt", O_WRONLY | O_APPEND);
		write(log, buffer, strlen(buffer));
		close(log);    
		
		ret = recv_message_timeout(300, &r);
		if (ret < 0 ) {   
			perror("receive error");
			return 1;
		
		}
		else if(ret == 0) {
			/* scriu in log.txt ca este timeout */
			sprintf(buffer, "[%s %s] Timeout!\n%s", data, timp, new_line);
			log = open("log.txt", O_WRONLY | O_APPEND);
			write(log, buffer, strlen(buffer));
			close(log);    
		}
		
		else { /* daca se primeste ACK sau NACK */
	  			f.seq_nr = r.payload[0];
			  	f.checksum = r.payload[1];                                               
				
				/* creez mesaj de primire pachet pentru fisier de log */ 
				time(&now);
				timeinfo = localtime(&now);					 //actualizez ora 
	
				sprintf(data, "%02d-%02d-%02d", timeinfo->tm_mday, 1 + timeinfo->tm_mon,
						 1900 + timeinfo->tm_year);
	
				sprintf(timp, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min,
						 timeinfo->tm_sec);
						 
				sprintf(buffer, "[%s %s] [SENDER] Am primit ACK - Seq Nr: %d - \n%s",
						 data, timp, f.seq_nr, new_line);
				
				/* scriu in log ca am primit un pachet */		
				log = open("log.txt", O_WRONLY | O_APPEND); //APPEND???
				write(log, buffer, strlen(buffer));
				close(log);
				 
				/* daca primesc ACK corect trimit urmatorul pachet */ 
	  			if(f.seq_nr == copy_seq) { 
					
					/* daca am terminat de trimis pachete si am primit 
					    confirmare ca SENDER-UL a aflat asta */
					if(r.len == 100 && r.payload[1] == -1){
						break;
					}
					
					/* daca mai am de trimis date */
					x = rand() % 60 + 1;
					free(f.payload);
					f.payload = (char*)malloc((x+1) * sizeof(char)); 
					memset(f.payload, 0, (x+1) * sizeof(char));
					temp = read(fd, f.payload, x);			 	
					copy_seq++;
					
					if(temp > 0) { //daca mai am date de citit din fisier
						
						/* nr de secventa */
						f.seq_nr = copy_seq;
						/* checksum-ul */			 
						f.checksum = calc_parity(f.payload, temp); 
						get_binary_checksum(f.checksum, cs_buffer);
						/* pun frameul in t */
						memcpy(t.payload, &f.seq_nr, sizeof(char));          
						memcpy(t.payload + 1, &f.checksum, sizeof(char));
					  	memcpy(t.payload + 2, f.payload, temp);
						/* lungime text trimis */
						t.len = temp;								 
					}
					else { //daca ma aflu la ultima secevnta care anunta ca s-a terminat transferul

						f.seq_nr = copy_seq;			
						f.checksum = 255;
						get_binary_checksum(f.checksum, cs_buffer);
						f.payload = (char*)malloc(sizeof(char));
						f.payload = "";
						t.len = 100; 
						memcpy(t.payload, &f.seq_nr, sizeof(char));            
						memcpy(t.payload + 1, &f.checksum, sizeof(char));
					  	memcpy(t.payload + 2, f.payload, sizeof(char));
						}
					
					/* creez mesaj pentru log de trimitere pachet nou */
					time(&now);
					timeinfo = localtime(&now);
	
					sprintf(data, "%02d-%02d-%02d", timeinfo->tm_mday, 1 + timeinfo->tm_mon,
						 1900 + timeinfo->tm_year);
	
					sprintf(timp, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min,
						 timeinfo->tm_sec);
					
					sprintf(buffer, "[%s %s] %sSeq Nr: %d\nPayload: %s\nChecksum: %s\n%s",
						data, timp, message_t, f.seq_nr, f.payload, cs_buffer, new_line);
			
			}
		
		}
	}
	
	
	close(fd);	

  return 0;
}
