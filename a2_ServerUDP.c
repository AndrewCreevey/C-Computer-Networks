//159.334 - Networks 
// Jonny Jackson  ID: 14089365 // Andrew Creevey ID: 12236284 // Jordan Smith  	ID: 12194358
///////////////   2016   ////////////////////
// SERVER: Prototype for assignment 2.
// This code is different than the one used in previous semesters...
//************************************************************************/
//COMPILE WITH: gcc server_Unreliable_2016.c -o server_Unreliable_2016
//with no losses nor damages, RUN WITH: ./server_Unreliable_2016 1234 0 0 
//with losses RUN WITH: ./server_Unreliable_2016 1234 1 0 
//with damages RUN WITH: ./server_Unreliable_2016 1234 0 1 
//with losses and damages RUN WITH: ./server_Unreliable_2016 1234 1 1  
//*********************************************************************/
#if defined __unix__ || defined __APPLE__
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h> 
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <netinet/in.h> 
#include <netdb.h>
#elif defined _WIN32 
#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <winsock2.h>
#define GENERATOR 0x8005
#endif

#include "UDP_supporting_functions_2016.c"

#define BUFFERSIZE 80 
//remember, the BUFFESIZE has to be at least big enough to receive the answer from the serv1
#define SEGMENTSIZE 78
//segment size, i.e., if fgets gets more than this number of bytes is segments the message in smaller parts.

//*******************************************************************
//Function to save lines and discarding the header
//*******************************************************************
//You ARE allowed to change this. You will need to alter the NUMBER_OF_WORD_IN_THE_HEADER if you add a CRC
#define NUMBER_OF_WORDS_IN_THE_HEADER 2

#if defined __unix__ || defined __APPLE__

#elif defined _WIN32 
#define WSVERS MAKEWORD(2,0)
WSADATA wsadata;
#endif

//*******************************************************************
//FUNCTIONS
//*******************************************************************
unsigned int CRCpolynomial(char *buffer) {
	unsigned char i;
	unsigned int rem = 0x0000;
	int bufsize = strlen((char*)buffer);
	while (bufsize-- != 0) {
		for (i = 0x80; i != 0; i /= 2) {
			if ((rem&0x8000) != 0) {
				rem = rem << 1;
				rem ^= GENERATOR;
			} else {
			  rem = rem << 1;
			}
	  		if ((*buffer&i) != 0) {
			   rem ^= GENERATOR;
			}
		}
		buffer++;
	}
	rem = rem&0xffff;//clean up values on the left
	return rem;
}

void save_line_without_header(char * receive_buffer,FILE *fout){
	char sep[2] = " "; //separation is space
	char *word;
	int wcount=0;
	char lineoffile[BUFFERSIZE]="\0";
	for (word = strtok(receive_buffer, sep);word;word = strtok(NULL, sep)) {
		wcount++;
		if(wcount > NUMBER_OF_WORDS_IN_THE_HEADER) {
			strcat(lineoffile,word);
			strcat(lineoffile," ");
		}	
	}
	lineoffile[strlen(lineoffile)-1]=(char)'\0';//get rid of last space
	if (fout!=NULL) fprintf(fout,"%s\n",lineoffile);
	else {
		printf("Error when trying to write...\n");
		exit(0);
	}	
}

unsigned int StripPacketNumber(char* strip) {
	int i = 7, j = 0; 
	char tempBuffer[80];
	while(strip[i] != ' ') {
		tempBuffer[j] = strip[i];
		tempBuffer[j+1] = '\0';		
		i++, j++;
	}
	unsigned int packetNumber = atoi(tempBuffer);
	return packetNumber; 
}

void StripCRC(char* strip, int CRC_len) {
	int i = 0, j = 0;
	j = (strlen(strip)-CRC_len);
	char temp[j+1];
	strcpy (temp, strip);
	for (i = 0; i < j; i++) {
		strip[i] = temp[i+CRC_len];
	}
	strip[i] = '\0';
}

bool Number (char n) {
	// Returns true if character is number //
	if(n >= '0' && n <= '9') {
		return true;
	}
	else return false;
}

//*******************************************************************
//MAIN
//*******************************************************************
int main(int argc, char *argv[]) {
//********************************************************************
// INITIALIZATION
//********************************************************************
	struct sockaddr_in localaddr,remoteaddr;
	#if defined __unix__ || defined __APPLE__
	int s;
	#elif defined _WIN32 
	SOCKET s;
	#endif
	char send_buffer[BUFFERSIZE],receive_buffer[BUFFERSIZE];
	int n,bytes,addrlen;
	addrlen = sizeof(remoteaddr);
	memset(&localaddr,0,sizeof(localaddr));//clean up the structure
	memset(&remoteaddr,0,sizeof(remoteaddr));//clean up the structure
	randominit();
	
	#if defined __unix__ || defined __APPLE__
	#elif defined _WIN32 
//********************************************************************
// WSSTARTUP
//********************************************************************
	if (WSAStartup(WSVERS, &wsadata) != 0) {
		WSACleanup();
		printf("WSAStartup failed\n");
	}
	#endif
//********************************************************************
//SOCKET
//********************************************************************
	s = socket(PF_INET, SOCK_DGRAM, 0);
	if (s <0) {
		printf("socket failed\n");
	}
	localaddr.sin_family = AF_INET;
	localaddr.sin_addr.s_addr = INADDR_ANY;//server address should be local
	if (argc != 4) {
		printf("2016 code USAGE: server port  lossesbit(0 or 1) damagebit(0 or 1)\n");
		exit(1);
	}
	localaddr.sin_port = htons((u_short)atoi(argv[1]));
	int remotePort=1234;
	packets_damagedbit=atoi(argv[3]);
	packets_lostbit=atoi(argv[2]);
	if (packets_damagedbit<0 || packets_damagedbit>1 || packets_lostbit<0 || packets_lostbit>1){
		printf("2016 code USAGE: server port  lossesbit(0 or 1) damagebit(0 or 1)\n");
		exit(0);
	}
//********************************************************************
//REMOTE HOST IP AND PORT
//********************************************************************
	remoteaddr.sin_family = AF_INET;
	remoteaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	remoteaddr.sin_port = htons(remotePort);
	//int counter=0;
//********************************************************************
//BIND
//********************************************************************
	if (bind(s,(struct sockaddr *)(&localaddr),sizeof(localaddr)) != 0) {
		printf("Bind failed!\n");
		exit(0);
	}
//********************************************************************
// Open file to save the incoming packets
//********************************************************************
	FILE *fout=fopen("file1_saved.txt","w");
//********************************************************************
//INFINITE LOOP
//********************************************************************
	printf("Server connected successfully.\nWaiting...\n");							/* Print out confirmation that server is working */
	
	int expectedPacketNumber = 0; 
	char lastAckedPacket[80]; 
	while (1) {
		//********************************************************************
		//RECEIVE
		//********************************************************************
		#if defined __unix__ || defined __APPLE__
		bytes = recvfrom(s, receive_buffer, SEGMENTSIZE-1, 0,(struct sockaddr *)(&remoteaddr),(socklen_t*)&addrlen);
		#elif defined _WIN32
		bytes = recvfrom(s, receive_buffer, SEGMENTSIZE-1, 0,(struct sockaddr *)(&remoteaddr),&addrlen);
		#endif
		//printf("Received %d bytes\n",bytes);
		//********************************************************************
		//PROCESS REQUEST
		//********************************************************************
		n = 0;
		while (n < bytes) {
			n++;
			if ((bytes < 0) || (bytes == 0)) break;
			if (receive_buffer[n] == '\n') { //end on a LF
				receive_buffer[n] = '\0';
				break;
			}
			if (receive_buffer[n] == '\r') //ignore CRs
				receive_buffer[n] = '\0';
		}
		if ((bytes < 0) || (bytes == 0)) break;
		printf("\n================================================\n");
		
		/*======================================================Checksum=================================================================*/
		/* Checksum - Get the crc value from the front of the packet and convert to an int.*/
		
		int i = 0;
		char CRCStr[BUFFERSIZE]; 
		while(receive_buffer[i] != ' ') { 										/*While not at the space, i++.*/
			CRCStr[i] = receive_buffer[i];
			i++;
		} 
		CRCStr[i] = '\0'; 
		i++;																			/*Increase by 1 so looking at next char.*/
		if (strncmp(receive_buffer, "CLOSE", 5) != 0) StripCRC(receive_buffer, i);											/*Strip the CRC value off the front of the receive_buffer.*/
		unsigned int CRCValue = atoi(CRCStr);								/*Convert string to integer.*/
		unsigned int receive_bufferCRC = 0;
		receive_bufferCRC = CRCpolynomial(receive_buffer);
		
		/*Get the packet number from the packet.*/
		int packetNumber = StripPacketNumber(receive_buffer);
		
		if (strncmp(receive_buffer, "CLOSE", 5) == 0) {
			//if client says "CLOSE", the last packet for the file was sent. Close the file
			//Remember that the packet carrying "CLOSE" may be lost or damaged!
			fclose(fout);
			#if defined __unix__ || defined __APPLE__
			close(s);
			#elif defined _WIN32
			closesocket(s);
			#endif	
			printf("Server saved file1_saved.txt \n");//you have to manually check to see if this file is identical to file1.txt
			printf("Closing server...\n");
			exit(0);
		}
		if(receive_bufferCRC != CRCValue) {
			printf("Error: Packet is corrupt!\n");
		}
		else if (packetNumber == expectedPacketNumber) { //See if the packet is the expected packet number. 
			printf("RECEIVED --> %s \n",receive_buffer);
			//********************************************************************
			//SEND ACK
			//********************************************************************
			sprintf(send_buffer,"ACKNOW %d",expectedPacketNumber);			//Put the ack into the sendBuffer. 
			//sprintf(lastAckedPacket,"ACKNOW %d",expectedPacketNumber);    //Store a copy of the last acked packet. 
			
			//attach CRC value here
			char crc[80];
			unsigned int ackCRC = 0;
			ackCRC = CRCpolynomial(send_buffer);
			
			//this adds the CRC value onto the front
			sprintf(crc, "%d ", ackCRC);
			strcat (crc, send_buffer);
			strcpy (send_buffer, crc);
			
			//this adds the size of string onto the front
			int size = strlen (crc);
			strcpy (crc, "");
			sprintf(crc, "%d ", size);
			strcat (crc, send_buffer);
			strcpy (send_buffer, crc);
			
			strcpy(lastAckedPacket, send_buffer);
			
			expectedPacketNumber++; 										//Increment the next expected packet. 
			send_unreliably(s,send_buffer,remoteaddr);				//Send it. 
			save_line_without_header(receive_buffer,fout);			//Save. 
			crc[0] = '\0';
			send_buffer[0] = '\0';
		} else { /*Packet in wrong order*/
			//send the last ack.
			printf("RECEIVED PACKET IN WRONG ORDER --> %s \n",receive_buffer);
			sprintf(send_buffer,lastAckedPacket); //put last acked packet into send_buffer. 
			send_unreliably(s,send_buffer,remoteaddr);
		}
	}
	#if defined __unix__ || defined __APPLE__
	close(s);	
	#elif defined _WIN32 
	closesocket(s);
	#endif
}
