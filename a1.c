/* -- Andrew Creevey -- 12236284 -- */
/* -- Default port used is 1234  -- */

//159.334 - Networks
//FTP server prototype for assignment 1 2013
//This prototype should compile with gcc (or g++) in Linux(or any Unix), Windows or OSX
//To see the differences, just follow the ifdefs
//please, send me an email listing the error messages if you are unable to compile this source
////////////////////////////////////////////////////////////////
////////// PORT and LIST (NLST) implemented ////////////////////
////////////////////////////////////////////////////////////////
#if defined __unix__ || defined __APPLE__ 
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h> 
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>
#elif defined _WIN32 
#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <winsock2.h>
//#include <ws2tcpip.h>
#define WSVERS MAKEWORD(2,0)
WSADATA wsadata;
#endif

#define BUFFERSIZE 800
#if defined __unix__ || defined __APPLE__ 
#elif defined _WIN32 
#endif
//*******************************************************************
//MAIN
//*******************************************************************
int main(int argc, char *argv[]) {
//********************************************************************
// INITIALIZATION
//********************************************************************
   struct sockaddr_in localaddr,remoteaddr;
   struct sockaddr_in remoteaddr_act;
#if defined __unix__ || defined __APPLE__ 
   int s,ns;
   int s_data_act = 0;
#elif defined _WIN32 
   SOCKET s,ns;
   SOCKET s_data_act = 0;
#endif
   char send_buffer[BUFFERSIZE],receive_buffer[BUFFERSIZE];
   //char remoteIP[INET_ADDRSTRLEN];
   //int remotePort;
   //int localPort;//no need for local IP...
   int n,bytes,addrlen;
   memset(&localaddr,0,sizeof(localaddr));//clean up the structure
   memset(&localaddr,0,sizeof(remoteaddr));//clean up the structure
//*******************************************************************
//WSASTARTUP 
//*******************************************************************
#if defined __unix__ || defined __APPLE__ 
//nothing to do in Linux
#elif defined _WIN32 
   if (WSAStartup(WSVERS, &wsadata) != 0) {
	   WSACleanup();
	   printf("WSAStartup failed\n");
	   exit(1);
   }
#endif
//********************************************************************
//SOCKET
//********************************************************************

   s = socket(PF_INET, SOCK_STREAM, 0);
   if (s <0) {
      printf("socket failed\n");
   }
   localaddr.sin_family = AF_INET;
   if (argc == 2) localaddr.sin_port = htons((u_short)atoi(argv[1]));
   else localaddr.sin_port = htons (1234);//default listening port 
   localaddr.sin_addr.s_addr = INADDR_ANY;//server address should be local
//********************************************************************
//BIND
//********************************************************************
   if (bind(s,(struct sockaddr *)(&localaddr),sizeof(localaddr)) != 0) {
      printf("Bind failed!\n");
      exit(0);
   }
//********************************************************************
//LISTEN
//********************************************************************
   listen(s,5);
//********************************************************************
//INFINITE LOOP
//********************************************************************
   while (1) {
      addrlen = sizeof(remoteaddr);
      //********************************************************************
      //NEW SOCKET newsocket = accept
      //********************************************************************
		#if defined __unix__ || defined __APPLE__ 
      ns = accept(s, (struct sockaddr *)(&remoteaddr),(socklen_t*)&addrlen);
		#elif defined _WIN32
      ns = accept(s, (struct sockaddr *)(&remoteaddr),&addrlen);
		#endif
      if (ns < 0) break;
		printf("-- Andrew Creevey -- 12236284 --\n");
      printf("Accepted a connection from client IP %s port %d \n",inet_ntoa(remoteaddr.sin_addr),ntohs(localaddr.sin_port));
      //********************************************************************
      //Respond with welcome message
      //*******************************************************************
		sprintf(send_buffer, "220 Welcome \r\n");
      bytes = send(ns, send_buffer, strlen(send_buffer), 0);
      while (1) {
         n = 0;
         while (1) {
//********************************************************************
//RECEIVE
//********************************************************************
				bytes = recv(ns, &receive_buffer[n], 1, 0);//receive byte by byte...
//********************************************************************
//PROCESS REQUEST
//********************************************************************
            if (bytes <= 0) break;
            if (receive_buffer[n] == '\n') { /*end on a LF*/
               receive_buffer[n] = '\0';
               break;
            }
            if (receive_buffer[n] != '\r') n++; /*ignore CRs*/
			}
			
         if (bytes <= 0) break;
			printf("-->DEBUG: the message from client reads: '%s' \r\n", receive_buffer);
			if (strncmp (receive_buffer, "USER", 4) == 0)  {
				printf("Logging in \n");
				sprintf(send_buffer,"331 Password required \r\n");
				bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				if (bytes < 0) break;
			}
			else if (strncmp (receive_buffer, "PASS", 4) == 0)  {
				printf("Typing password (anything will do...) \n");
				sprintf(send_buffer,"230 Public login sucessful \r\n");
				bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				if (bytes < 0) break;
			}
			else if (strncmp (receive_buffer, "SYST", 4) == 0)  {
				system ("ver > ver.txt");
				FILE *fin = fopen ("ver.txt","r"); //open ver.txt file
            sprintf (send_buffer,"150 Transfering... \r\n");
            bytes = send (ns, send_buffer, strlen(send_buffer), 0);
            char temp_buffer[80];
            while (!feof(fin)) {
					fgets(temp_buffer,78,fin);
               sprintf(send_buffer,"%s",temp_buffer);
					send(s_data_act, send_buffer, strlen(send_buffer), 0);
				}
				fclose(fin);
				sprintf(send_buffer,"226 File transfer completed... \r\n");
				bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				if (bytes < 0) break;
			}
///////////////////////////////////////////////////////
//        PORT                                       //
///////////////////////////////////////////////////////	
			else if (strncmp (receive_buffer, "PORT", 4) == 0) {
				s_data_act = socket(AF_INET, SOCK_STREAM, 0);
            //local variables
				unsigned char act_port[2];
				int act_ip[4], port_dec;
				char ip_decimal[40];
				sscanf(receive_buffer, "PORT %d,%d,%d,%d,%d,%d",&act_ip[0],&act_ip[1],&act_ip[2],&act_ip[3],(int*)&act_port[0],(int*)&act_port[1]);
				remoteaddr_act.sin_family=AF_INET;//local_data_addr_act
				sprintf(ip_decimal, "%d.%d.%d.%d", act_ip[0], act_ip[1], act_ip[2],act_ip[3]);
				printf("IP is %s\n",ip_decimal);
				remoteaddr_act.sin_addr.s_addr=inet_addr(ip_decimal);
				port_dec=act_port[0]*256+act_port[1];
				printf("port %d\n",port_dec);
				remoteaddr_act.sin_port=htons(port_dec);
				
				if (connect(s_data_act, (struct sockaddr *)&remoteaddr_act, (int) sizeof(struct sockaddr)) != 0){
					printf("Trying connection in %s %d\n",inet_ntoa(remoteaddr_act.sin_addr),ntohs(remoteaddr_act.sin_port));
					sprintf(send_buffer, "425 Something is wrong, can't start the active connection... \r\n");
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					#if defined __unix__ || defined __APPLE__ 
					close(s_data_act);
					#elif defined _WIN32 
					closesocket(s_data_act);
					#endif
				} else {
					sprintf(send_buffer, "200 Valid command\r\n");
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					printf("Data connection to client created (active connection)\r\n");
				}
			}
///////////////////////////////////////////////////////
//        LIST or NLST                               //
///////////////////////////////////////////////////////	
			else if ((strncmp (receive_buffer, "LIST", 4) == 0) 
					|| (strncmp (receive_buffer, "NLST", 4) == 0))   {
				// We need to check if the port has been opened
				// use bool to check if its been in or not
				#if defined __unix__ || defined __APPLE__ 
				system ("ls > tmp.txt");
				#elif defined _WIN32 
				system ("dir > tmp.txt");
				#endif		
				FILE *fin = fopen ("tmp.txt","r");//open tmp.txt file
				sprintf (send_buffer,"150 Transfering... \r\n");
				bytes = send(ns, send_buffer, strlen (send_buffer), 0);
				char temp_buffer[80];
				
				while (!feof (fin)){
					fgets (temp_buffer, 78, fin);
					sprintf (send_buffer, "%s", temp_buffer);
					send (s_data_act, send_buffer, strlen (send_buffer), 0);
				}
				fclose (fin);
				sprintf (send_buffer, "226 File transfer completed... \r\n");
				bytes = send (ns, send_buffer, strlen (send_buffer), 0);
				#if defined __unix__ || defined __APPLE__ 
				close (s_data_act);
				#elif defined _WIN32
				closesocket (s_data_act);
				#endif		
				//OPTIONAL, delete the temporary file after listing
				system("del tmp.txt");
			}
			//-----------------------------------------------------------------------------
			
			else if (strncmp (receive_buffer, "STOR", 4) == 0)  {
				//Store a file
				printf("Receive buffer = %s\r\n", receive_buffer); //error checking
				char fileName[80];									//get filename
				int i = 5, j = 0;										//start i=5 so you skip 'RETR '
				int len = strlen(receive_buffer);				//get length of receive buffer
				
				//put filename into a string
				while (receive_buffer[i] != '\0' && i < len) {
					fileName[j] = receive_buffer[i];
					i++; j++;
				}
				fileName[j] = '\0'; 									//adds '\0' to end of the string
				
				if (fopen (fileName, "w") == NULL) {				//if file is empty or doesnt exist
					sprintf (send_buffer , "450 Requested file action not taken\r\n");
					bytes = send (ns, send_buffer, strlen(send_buffer), 0);
				} else { 
					FILE *fout = fopen (fileName, "w");			//open file ready to write
					sprintf (send_buffer, "150 File status okay; about to open data connection\r\n");
					bytes = send (ns, send_buffer, strlen(send_buffer), 0);
					while (1) {
						n = 0;
						while (1) {
							//Receive byte by byte
							bytes = recv (s_data_act, &receive_buffer[n], 1, 0);
							if (bytes <= 0) break;					//if nothing more, break
							if (receive_buffer[n] != '\r') n++;	//if buffer[n] is not equal to \r, n++;
						}
						receive_buffer[n] = '\0'; 					//end of line is here
						fprintf (fout, "%s", receive_buffer);	//print out buffer
						if (bytes <= 0) break;						//if nothing more, break
					}
					sprintf (send_buffer, "226 File transfer completed...\r\n");
					bytes = send (ns, send_buffer, strlen(send_buffer), 0);
					//close file
					fclose(fout);
					//close socket
					#if defined __unix__ || defined __APPLE__ 
					close (s_data_act);
					#elif defined _WIN32
					closesocket (s_data_act);
					#endif
				}
			}
			
			//-----------------------------------------------------------------------------
			
			else if (strncmp (receive_buffer, "RETR", 4) == 0) {
				//Retrieve a file
				printf("Receive buffer = %s\r\n", receive_buffer); //error checking
				char fileName[80];									//get filename
				int i = 5, j = 0;										//start i=5 so you skip 'RETR '
				int len = strlen(receive_buffer);				//get length of receive buffer
				
				//put filename into a string
				while (receive_buffer[i] != '\0' && i < len) {
					fileName[j] = receive_buffer[i];
					i++; j++;
				}
				fileName[j] = '\0'; 									//adds '\0' to end of the string
				
				if (fopen (fileName, "r") == NULL) {
					sprintf (send_buffer , "450 Requested file action not taken\r\n");
					bytes = send (ns, send_buffer, strlen(send_buffer), 0);
				} else {
					FILE *fin = fopen (fileName, "r"); 			//open file ready to read
					sprintf (send_buffer, "150 File status okay; about to open data connection\r\n");
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					
					char temp_buffer[BUFFERSIZE];					//create temp buffer
					while (true) {										//while not at end of file
						fgets (temp_buffer, 798, fin);
						sprintf (send_buffer, "%s\r\n", temp_buffer);
						//++++++++++++++++++++++++++++++++++
						//the following is to fix the transfer putting an empty new line at the end
						//of the file, this was not copying the file 'exactly'
						if (feof(fin)) {
							sprintf (send_buffer, "%s", temp_buffer);
							send (s_data_act, send_buffer, strlen(send_buffer), 0);
							break;
						}
						//++++++++++++++++++++++++++++++++++
						send (s_data_act, send_buffer, strlen(send_buffer), 0);
					}
					fclose (fin);										//close file
					sprintf (send_buffer, "226 File transfer completed...\r\n"); //confirm close
					bytes = send (ns, send_buffer, strlen (send_buffer), 0);
				}
				//close socket
				#if defined __unix__ || defined __APPLE__ 
				close (s_data_act);
				#elif defined _WIN32
				closesocket (s_data_act);
				#endif
			}
			
			//-----------------------------------------------------------------------------
			else if (strncmp (receive_buffer, "QUIT", 4) == 0)  {
				printf("Quit \n");
				sprintf(send_buffer,"221 Connection closed by the FTP client\r\n");
				bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				if (bytes < 0) break;
				
				#if defined __unix__ || defined __APPLE__ 
				close(ns);
				#elif defined _WIN32 
				closesocket(ns);
				#endif
			}
			else { 
				printf("202 Command not implemented\n");
				sprintf(send_buffer,"202 Command not implemented\r\n");
				bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				if (bytes < 0) break;
			}
      }
//********************************************************************
//CLOSE SOCKET
//********************************************************************
		#if defined __unix__ || defined __APPLE__ 
      close(ns);
		#elif defined _WIN32 
      closesocket(ns);
		#endif
      printf("disconnected from %s\n",inet_ntoa(remoteaddr.sin_addr));
   }
	#if defined __unix__ || defined __APPLE__ 
   close(s);//it actually never gets to this point....use CTRL_C
	#elif defined _WIN32 
   closesocket(s);//it actually never gets to this point....use CTRL_C
	#endif
}
