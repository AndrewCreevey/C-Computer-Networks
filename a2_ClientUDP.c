//159.334 - Networks
// Jonny Jackson  ID: 14089365 // Andrew Creevey ID: 12236284 // Jordan Smith  	ID: 12194358
///////////////   2016   ////////////////////
// CLIENT: Prototype for assignment 2. 
// This code is different than the one used in previous semesters...
//************************************************************************/
//COMPILE WITH: gcc client_Unreliable_2016.c -o client_Unreliable_2016
//with no losses nor damages, RUN WITH: ./client_Unreliable_2016 127.0.0.1 1234 0 0 
//with losses RUN WITH: ./client_Unreliable_2016 127.0.0.1 1234 1 0 
//with damages RUN WITH: ./client_Unreliable_2016 127.0.0.1 1234 0 1 
//with losses and damages RUN WITH: ./client_Unreliable_2016 127.0.0.1 1234 1 1  
//*************************************************************************/
#if defined __unix__ || defined __APPLE__
#include <unistd.h>
#include <errno.h>
#include <time.h>
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
#include <stdlib.h>
#include <winsock2.h>
#define WSVERS MAKEWORD(2,0)
WSADATA wsadata;
#endif

#include "UDP_supporting_functions_2016.c"
#define BUFFERSIZE 80 
//remember, the BUFFERSIZE has to be at least big enough to receive the answer from the server
#define SEGMENTSIZE 78
#define GENERATOR 0x8005
struct sockaddr_in localaddr,remoteaddr;
//segment size, i.e., if fgets gets more than this number of bytes is segments the message in smaller parts.

bool Number (char n) {
	// Returns true if character is number //
	if(n >= '0' && n <= '9') {
		return true;
	}
	else return false;
}

int getAckNumber(char* receiveBuffer)
{
	char ackNumberBuffer[50] = { 0 }; //this zero-initalizes the array
	int i = 0; 
	int bufferCount = 7; //start at index 7 to ignore the "ACKNOW ". The receive_buffer should contain something like this: "ACKNOW 12 \r\n"
	
	//while the character at current position is a number, keep reading
	while(Number(receiveBuffer[bufferCount]) == true)
	{
		ackNumberBuffer[i] = receiveBuffer[bufferCount]; //This should grab the number out of the receive_buffer;
		bufferCount++; 
	}
	int ackNum = atoi(ackNumberBuffer); //Convert the String to an int.
	return ackNum; 
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

void Strip(char* strip, int CRC_len) {
	int i = 0, j = 0;
	j = (strlen(strip)-CRC_len);
	char temp[j+1];
	strcpy (temp, strip);
	for (i = 0; i < j; i++) {
		strip[i] = temp[i+CRC_len];
	}
	strip[i] = '\0';
}

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

// Simple usage: client IP port, or client IP (use default port) 
int main(int argc, char *argv[]) {
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
//*******************************************************************
// Initialization
//*******************************************************************
   memset(&localaddr, 0, sizeof(localaddr));//clean up
   //char localIP[INET_ADDRSTRLEN]="127.0.0.1";
   int localPort=1234;
   localaddr.sin_family = AF_INET;
   //localaddr.sin_addr.s_addr = inet_addr(localIP);
   localaddr.sin_addr.s_addr = INADDR_ANY;//server address should be local
//********************************************************************
   localaddr.sin_port = htons(localPort);
   memset(&remoteaddr, 0, sizeof(remoteaddr));//clean up  
   randominit();
#if defined __unix__ || defined __APPLE__
   int s;
#elif defined _WIN32
   SOCKET s;
#endif
   char send_buffer[BUFFERSIZE], receive_buffer[BUFFERSIZE];
   remoteaddr.sin_family = AF_INET;
//*******************************************************************
//	Dealing with user's arguments
//*******************************************************************
   if (argc != 5) {
	   printf("2011 code USAGE: client remote_IP-address port  lossesbit(0 or 1) damagebit(0 or 1)\n");
      exit(1);
   }
   remoteaddr.sin_addr.s_addr = inet_addr(argv[1]);//IP address
   remoteaddr.sin_port = htons((u_short)atoi(argv[2]));//always get the port number
   packets_lostbit = atoi(argv[3]);
   packets_damagedbit = atoi(argv[4]);
   if (packets_damagedbit<0 || packets_damagedbit>1 || packets_lostbit<0 || packets_lostbit>1){
	   printf("2011 code USAGE: client remote_IP-address port  lossesbit(0 or 1) damagebit(0 or 1)\n");
	   exit(0);
   }
//*******************************************************************
//CREATE CLIENT'S SOCKET 
//*******************************************************************
   s = socket(AF_INET, SOCK_DGRAM, 0);//this is a UDP socket
   if (s < 0) {
      printf("Socket failed\n");
   	exit(1);
   }
#if defined __unix__ || defined __APPLE__

#elif defined _WIN32
   //***************************************************************//
   //NONBLOCKING OPTION for Windows.
   //***************************************************************//
   u_long iMode=1;
   ioctlsocket(s,FIONBIO,&iMode);
#endif
//*******************************************************************
//SEND A TEXT FILE. 
//*******************************************************************
	struct Packet {	
		char content[80]; 
		int position;
	};
	
	#if defined __unix__ || defined __APPLE__
   FILE *fin=fopen("file1.txt","r");
	#elif defined _WIN32
   FILE *fin=fopen("file1_Windows.txt","r");
	#endif
	
   if(fin == NULL){
	   printf("Cannot open file\n");
	   exit(0);
   }
	
	Packet packetArray[1000]; 
   int ackcounter = 0;
	unsigned int CRCresult = 0;
	int packetCount = 0;
	char str[80];
	char temp[80]; 
	bool ackedPackets[1000] = { false }; //This defaults all variables to false
	
	/////////////////////////////////////////////////////////////////////////
	// This loop reads in all the packets and adds them to a Packet array. //
	/////////////////////////////////////////////////////////////////////////
	while (!feof (fin)) 
	{
		fgets(str,SEGMENTSIZE,fin); //get the next line from the file
		if(str[0] == '\n' || str[0] == '\r' || str[0] == '\0' || strcmp(str, "") == 0)
		{
			//if string is empty, then dont add to the structure
			break;
		}
		Packet tempPacket;		//Create a temp packet
     
		//Concatentate packet number onto the front of the string
		sprintf(temp,"PACKET %d ", packetCount);
		strcat(temp, str);
      int size = strlen(temp+1); //size +1 incase of out of bounds
     
		//declare and allocate memory to a temp variable.
    	char* test = NULL;
      test = (char*) malloc ((size+1)*sizeof(char)); //size +1 incase of out of bounds
      strcpy(test, temp);
      size = strlen(test);
		
      ///We make variable so that we can get rid of the trailing '\n' to use for CRC
      char tempVar[size];
      int i = 0;
      while (test[i] != '\n' && test[i] != '\0') {
        tempVar[i] = test[i];
        i++;
      }
      tempVar[i] = '\0'; //close the string
		
    	CRCresult = CRCpolynomial(tempVar); //Get the crc value of the string
		
		//add the crc result onto the front of the string
		strcpy (temp , ""); //set to null
		sprintf(temp, "%d ", CRCresult);
      strcat (temp, test);
      strcpy (str, temp);
		
		//copy data into the structure
     	tempPacket.position = packetCount;
   	strncpy(tempPacket.content, str, 80);
     	packetArray[packetCount] = tempPacket;
		packetCount++; //increase count
     
      //free test variables
      test = NULL;
    	free(test);
      strcpy(tempVar, "");	//Reset the str string back to null, this ensures no memory leaks
		strcpy(str, ""); 		
	}
	
	//Windows size is 3 packets. So leftOfWindow + 2 is the end of the window. 
	int window = 0; //Window starts off at 0. E.g [0,1,2],3,4,5,6. 
	bool sendPackets = true;
	/*Make our variables for each packet.*/
	int packet1Index = 0, packet2Index = 0, packet3Index = 0;  /*Index of the packets that need to be send out.*/
	float packet1Total, packet2Total, packet3Total; 
   clock_t packet1Start = 0, packet2Start = 0, packet3Start = 0;
	clock_t endTime1 = 0, endTime2 = 0, endTime3 = 0;
	packet1Total = packet2Total = packet3Total = 0;
	
	//////////////////////////////////////
	// This loop deals with the content //
	//////////////////////////////////////
	while (1) {
		if (ackcounter < packetCount-1) { /*If we still have packets that need acking.*/
			// ^ needs to be -1 because the ackCount includes 0, whereas packetCount does not
			if(sendPackets == true) {
				/*Determine the indexes of the packetArray we need to send out.*/
				//										//  0   1   2
				packet1Index = window; 			//[Here, #, #]
				packet2Index = (window + 1); 	//[#, Here, #]
				packet3Index = (window + 2);  //[#, #, Here]
				
				/*Send out the packets, this will always happen if ackcount < packetcount-1 */
				send_unreliably(s,packetArray[packet1Index].content, remoteaddr); //Send first packet
				packet1Start = clock(); //This starts the timer.
				Sleep(50);
				
				if(ackcounter + 1 < packetCount-1) { 
					//if we are not out of bounds, send second packet
					send_unreliably(s,packetArray[packet2Index].content, remoteaddr); //Send second packet 
					packet2Start = clock(); //This starts the timer.
					Sleep(50);
				}
				
				if(ackcounter + 2 < packetCount-1) {
					//if we are not out of bounds, send third packet
					send_unreliably(s,packetArray[packet3Index].content, remoteaddr); //Send third packet
					packet3Start = clock(); //This starts the timer.
					Sleep(50);
				}
				sendPackets = false; //The packets have been send so don't send them again next time.
				
				/*Sleep*/
				#if defined __unix__ || defined __APPLE__
				sleep(1);
				#elif defined _WIN32
				Sleep(1000);
				#endif
				
				/*Calculate packet flight time. Should be around 1000 - 1050MS I'm assuming.*/
				packet1Total = (clock() - packet1Start) / CLOCKS_PER_SEC;
				if(ackcounter+1 < packetCount-1) packet2Total = (clock() - packet2Start) / CLOCKS_PER_SEC;
				if(ackcounter+2 < packetCount-1) packet3Total = (clock() - packet3Start) / CLOCKS_PER_SEC;
			}
			
			//********************************************************************
			//RECEIVE
			//********************************************************************
			/*if timers are not past the time out time and the packets have been sent out.)*/
			/*time out is set to 1500 MS. This might need to be changed.*/
			
			//Check if under 3sec
			bool inTime = false;
			//this function checks if the time is under 3sec, depending on the number of variables.
			if (ackcounter+2 < packetCount-1) { //if all 3 variables sent then check all under 3sec
				if (packet1Total <= 3 && packet2Total <= 3 && packet3Total <= 3) inTime = true;
			} else if (ackcounter+1 < packetCount-1) { //if both variables sent then check both under 3sec
				if (packet1Total <= 3 && packet2Total <= 3) inTime = true;
			} else if (ackcounter < packetCount-1) { //if only 1 variables sent then check it's under 3sec
				if (packet1Total <= 3) inTime = true;
			}
			
			//if timers are under 3 sec and have sent packets.
			if (inTime == true && sendPackets == false) {
				//get ack num
				recv_nonblocking (s, receive_buffer, remoteaddr); //you can replace this, but use MSG_DONTWAIT to get non-blocking recv.
				
				//Retrieves the number containing the length of string we need to read
				//We had to do this because our sizing was getting messed up, this ensures we read the correct numbers in
				int i = 0;
				char number[5] = { "" };
				int size = 0;
				while (receive_buffer[i] != ' ') { //this gets the length from start of string
					number[i] = receive_buffer[i];
					i++;
				} number[i] = '\0';
				i++;
				size = atoi (number);	//Convert the size into a number
				Strip(receive_buffer, i); //This will strip the length off the start of the string
				
				//read into a tempArray, the number of characters defined by our size ^
				char tempArray[size+1];
				for (i = 0; i < size; i++) {
					tempArray[i] = receive_buffer[i];
				} tempArray[i] = '\0';
				
				strcpy(receive_buffer, tempArray); //copy back into the receive buffer
				
				//check if CRC value is correct
				char CRCStr[BUFFERSIZE];
				i = 0;
				while(receive_buffer[i] != ' ') {									//While not at the space, i++.
					CRCStr[i] = receive_buffer[i];
					i++;
				} 
				CRCStr[i] = '\0'; i++;
				CRCresult = atoi (CRCStr);
				Strip(receive_buffer, i);
				
				if (CRCresult == CRCpolynomial(receive_buffer)) {
					if (strncmp(receive_buffer, "ACKNOW", 6) == 0) { 	  //ACKNOW
						int ackNum = getAckNumber (receive_buffer);
						
						if (ackedPackets[ackNum] == false && ackNum < packetCount && ackNum != -1) { 
							//checks ackNum is not entered, is in bounds and didnt return an error
							if (ackNum > ackcounter) ackcounter = ackNum; //this sets ackCount to the biggest received ackNum
							ackedPackets[ackNum] = true;
							printf("RECEIVE --> %s \n",receive_buffer);
							
							/*Get the ack number as an int from the receive_buffer.*/
							if(ackNum == window) { /*Check what ack the packet is.*/
								window++; 
							}
							else if(ackNum > window) { //If we get an ack bigger than the one we are expecting it's safe to assume the other packets made it to the sever. 
								window = ackNum + 1;
							}
							
							#if defined __unix__ || defined __APPLE__
							sleep(1);//wait for a bit before trying the next packet
							#elif defined _WIN32
							Sleep(1000);
							#endif
						} else if (ackedPackets[ackNum] == true) {
							//if duplicate ack number received.
							printf("Error: Number has already been acked, '%s' ignored.\n", receive_buffer);
						} else { 
							//else, num is out of bounds, print error.
							printf("Error: AckNum was out of bounds.\n");
						}
						receive_buffer[0] = '\0';
						CRCStr[0] = '\0';
					}
				} else {
					printf("CRC values were not the same.\n");
				}
			} else {
				/*If the packets have timed out then set send packets to true to send the packets in the window out again.*/
				printf("Packets have timed out, resending...\n");
				sendPackets = true; 
			}
		} else {
			//else end of file
			printf("End of the file!\n"); 
			strcpy (send_buffer, "");
			sprintf(send_buffer,"CLOSE \r\n");
			send_unreliably(s,send_buffer,remoteaddr); //we actually send this reliably, read UDP_supporting_functions_2016.c
			break;
		}
		
		/*Calculate the time that each packet have been in flight.*/
		endTime1 = endTime2 = endTime3 = clock();
		packet1Total = (endTime1 - packet1Start) / CLOCKS_PER_SEC;
		packet2Total = (endTime2 - packet2Start) / CLOCKS_PER_SEC;
		packet3Total = (endTime3 - packet3Start) / CLOCKS_PER_SEC;
   }
//*******************************************************************
//CLOSESOCKET   
//*******************************************************************
   printf("closing everything on the client's side ... \n");
   fclose(fin);
	#if defined __unix__ || defined __APPLE__
   close(s);
	#elif defined _WIN32
   closesocket(s);
	#endif
   exit(0);
}
