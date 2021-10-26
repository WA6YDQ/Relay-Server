/* Relay server - accept connections from remote client, toggle relay lines
 * based on input commands
 *
 * k theis 10/2021
 * runs on raspberry pi.
 * NOTE: only runs in IPV4
 *
 * Before starting:
 * To access the GPIO hardware you will need to be part of the GPIO group.
 * Run this command:  
 * 		sudo usermod -a -G gpio <your user name>
 *
 * This program uses the wiringPi version 2.50 libraries. Use apt-get install 
 * to get them (if you don't already have them):
 * 		sudo apt-get install wiringpi
 *
 * To compile this program use:
 * 		cc -o relayServer relayServer.c -lwiringPi
*/


#define PORT 9000			/* server port - this server listens on this port # */

/* wire the relay circuit to the hardware pins defined below */
#define RELAY1PIN	4		/* GPIO 23 is ***hardware pin 16*** and wiringPi pin 4 */
#define RELAY2PIN   5		/* GPIO 24 is ***hardware pin 18*** and wiringPi pin 5 */
#define RELAY3PIN	6		/* GPIO 25 is ***hardware pin 22*** and wiringPi pin 6 */
#define RELAY4PIN	27		/* GPIO 16 is ***hardware pin 36*** and wiringPi pin 27 */


#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <wiringPi.h>

int main(void)
{
  int listenfd = 0,connfd = 0;
  int n=0;

  struct sockaddr_in serv_addr, cli_addr;
  int clilen = sizeof(cli_addr);
  char sendBuff[1025], recvBuff[1024];  
  int numrv;  

  /* set up the hardware ports */
  wiringPiSetup();
  pinMode(RELAY1PIN, OUTPUT);
  pinMode(RELAY2PIN, OUTPUT);
  pinMode(RELAY3PIN, OUTPUT);
  pinMode(RELAY4PIN, OUTPUT);

  /* initialize the TCP/IP port */
  printf("Starting server:\n");
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  
  memset(&serv_addr, '0', sizeof(serv_addr));
  memset(sendBuff, '0', sizeof(sendBuff));
      
  serv_addr.sin_family = AF_INET;    
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* same as 0.0.0.0 */
  serv_addr.sin_port = htons(PORT);    
 
  bind(listenfd, (struct sockaddr*)&serv_addr,sizeof(serv_addr));
  
  if(listen(listenfd, 10) == -1){
      printf("ERROR - Failed to listen\n");
      return -1;
  }     

resumeLoop:    /* goto's are bad, but... */
  while(1)
    {      
      connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen); // accept awaiting request
	  printf("Connection request from %s\n", inet_ntoa(cli_addr.sin_addr));

	  /* send initial message */
      strcpy(sendBuff, "READY\n");
      write(connfd, sendBuff, strlen(sendBuff));
 
	  /* look for commands from client */
readloop:
	  /* init buffer */
	  memset(&recvBuff,0,sizeof(recvBuff));
	  n = 0;
	  sleep(1);

	  while((n = read(connfd, recvBuff, sizeof(recvBuff)-1)) > 0) {
		  recvBuff[n] = 0;  	/* n is size of chars read */

		  /* test response from client */

		  /* terminate connection */
		  if (strncmp(recvBuff,"CLOSE",5)==0) {
			  close(connfd);
			  fprintf(stderr,"Received CLOSE from client\n");
              goto resumeLoop;
		  }

		 /* network link */
		  if (strncmp(recvBuff,"PING",4)==0) {
			  strcpy(sendBuff,"OK\n");
			  write(connfd, sendBuff, strlen(sendBuff));
			  close(connfd);
			  goto resumeLoop;
	     }

		 /* show commands if requested */
		 if (strncmp(recvBuff,"HELP",4)==0) {
			 strcpy(sendBuff,"relay#_ON/relay#_OFF w/#=1-4\n");
			 write(connfd, sendBuff, strlen(sendBuff));
			 close(connfd);
			 goto resumeLoop;
		 }


		 /* Relay #1 */
		 if (strncmp(recvBuff,"relay1_ON",9)==0) {
			 digitalWrite(RELAY1PIN,1);	// turn on relay 
			 fprintf(stderr,"relay 1 ON\n");		// local display
			 strcpy(sendBuff,"r1on\n");
			 write(connfd, sendBuff, strlen(sendBuff));	// remote display
			 close(connfd);		// close connection
			 goto resumeLoop;
		 }
		 if (strncmp(recvBuff,"relay1_OFF",10)==0) {
		 	 digitalWrite(RELAY1PIN,0);	// turn off relay 
			 fprintf(stderr,"relay 1 OFF\n");		// local display
			 strcpy(sendBuff,"r1off\n");
			 write(connfd, sendBuff, strlen(sendBuff));	// remote display
		 	 close(connfd);
			 goto resumeLoop;
	     }


		/* Relay #2 */
         if (strncmp(recvBuff,"relay2_ON",9)==0) {
             digitalWrite(RELAY2PIN,1); 			// turn on relay 
             fprintf(stderr,"relay 2 ON\n");        // local display
             strcpy(sendBuff,"r2on\n");
             write(connfd, sendBuff, strlen(sendBuff)); // remote display
             close(connfd);     					// close connection
             goto resumeLoop;
         }
         if (strncmp(recvBuff,"relay2_OFF",10)==0) {
             digitalWrite(RELAY2PIN,0); 			// turn off relay
             fprintf(stderr,"relay 2 OFF\n");       // local display
             strcpy(sendBuff,"r2off\n");
             write(connfd, sendBuff, strlen(sendBuff)); // remote display
             close(connfd);							// close connection
             goto resumeLoop;
         }

		/* Relay #3 */
         if (strncmp(recvBuff,"relay3_ON",9)==0) {
             digitalWrite(RELAY3PIN,1); // turn on relay 1
             fprintf(stderr,"relay 3 ON\n");        // local display
             strcpy(sendBuff,"r3on\n");
             write(connfd, sendBuff, strlen(sendBuff)); // remote display
             close(connfd);     // close connection
             goto resumeLoop;
         }
         if (strncmp(recvBuff,"relay3_OFF",10)==0) {
             digitalWrite(RELAY3PIN,0); // turn off relay 1
             fprintf(stderr,"relay 3 OFF\n");       // local display
             strcpy(sendBuff,"r3off\n");
             write(connfd, sendBuff, strlen(sendBuff)); // remote display
             close(connfd);
             goto resumeLoop;
         }


		 /* Relay #4 */
         if (strncmp(recvBuff,"relay4_ON",9)==0) {
             digitalWrite(RELAY4PIN,1); // turn on relay 1
             fprintf(stderr,"relay 4 ON\n");        // local display
             strcpy(sendBuff,"r4on\n");
             write(connfd, sendBuff, strlen(sendBuff)); // remote display
             close(connfd);     // close connection
             goto resumeLoop;
         }
         if (strncmp(recvBuff,"relay4_OFF",10)==0) {
             digitalWrite(RELAY4PIN,0); // turn off relay 1
             fprintf(stderr,"relay 4 OFF\n");       // local display
             strcpy(sendBuff,"r4off\n");
             write(connfd, sendBuff, strlen(sendBuff)); // remote display
             close(connfd);
             goto resumeLoop;
         }



	}

	  goto readloop;	/* shouldn't get here, but just in case */
  }
	  printf("Unexpected close with client\n");
      close(connfd);    
      sleep(1);
 
 
  return 0;
}
