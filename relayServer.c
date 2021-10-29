/* Relay server - accept connections from remote client, toggle relay lines
 * based on input commands
 *
 * k theis 10/2021
 * runs on raspberry pi.
 * NOTE: only runs in IPv4, IPv6 not supported. 
 *
 * Before starting:
 * To access the GPIO hardware you may need to be part of the GPIO group.
 * Run this command if you get permission errors:  
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
#define TIMEOUT 30			/* ping timeout in seconds */

/* wire the relay circuit to the hardware pins defined below */
#define RELAY1PIN	4		/* GPIO 23 is ***hardware pin 16*** and wiringPi pin 4 */
/* add more hardware defines here */

#include <sys/socket.h>
#include <sys/ioctl.h>
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
#include <time.h>

int main(void)
{
  int listenfd = 0,connfd = 0;
  int n=0, bytesAvailable;
  int FLAG = 0;
  time_t curtime;

  struct sockaddr_in serv_addr, cli_addr;
  int clilen = sizeof(cli_addr);
  char sendBuff[1025], recvBuff[1024];  
  int numrv;  

  /* set up the hardware ports */
  wiringPiSetup();
  pinMode(RELAY1PIN, OUTPUT);
  /* add more pinMode commands for more hardware */

  /* initialize the TCP/IP port */
  printf("Starting server:\n");
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  
  memset(&serv_addr, '0', sizeof(serv_addr));
  memset(sendBuff, '0', sizeof(sendBuff));
  bytesAvailable = 0;    

  serv_addr.sin_family = AF_INET;    
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* same as 0.0.0.0 */
  serv_addr.sin_port = htons(PORT);    
 
  bind(listenfd, (struct sockaddr*)&serv_addr,sizeof(serv_addr));
  
  if(listen(listenfd, 10) == -1){
      printf("ERROR - Failed to listen\n");		/* this may happen due to bad permissions, etc */
      return -1;
  }     

resumeLoop:    /* goto's are bad, but... */
  while(1)
    {      
      connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen); // accept awaiting request
	  printf("Connection request from %s\n", inet_ntoa(cli_addr.sin_addr));

	  /* send initial message */
      strcpy(sendBuff, "READY");
      write(connfd, sendBuff, strlen(sendBuff));
 
	  /* look for commands from client */
	  
	  memset(&recvBuff,0,sizeof(recvBuff));
	  n = 0;
	  curtime = time(NULL);

	  while (1) {	// while connected
		  
		  usleep(5000);		// delay to keep cpu activity down

		  /* since the network connection can be lost, test it by pinging the client 
		   * every few seconds. If no response, close the connection, disable any active
		   * relays then go back and look for new connections.
		   */

		  if (time(NULL) > curtime+TIMEOUT) {		// see if network connection is still up
			  bytesAvailable = 0;
			  strcpy(sendBuff,"PING");
			  write(connfd, sendBuff, strlen(sendBuff));
			  usleep(500000);	// long wait for response
			  ioctl(connfd, FIONREAD, &bytesAvailable);
			  if (bytesAvailable == 0) {		// no response - close connection
				  fprintf(stderr,"No response from client - closing connection\n");
				  close(connfd);
				  FLAG=0;

				  /* copy/paste to add more hardware */
				  digitalWrite(RELAY1PIN,0); // turn off relay
				  fprintf(stderr,"relay 1 OFF\n");

				  goto resumeLoop;
			 }
			 // buffer has data - test for OK
			 n = read(connfd, recvBuff, sizeof(recvBuff)-1);
			 recvBuff[n] = 0;
			 if (strncmp(recvBuff,"OK",2) != 0) {	// bad response, close connection
				 fprintf(stderr,"Bad response from client (%s) - closing connection\n",recvBuff);
				 close(connfd);
				 
				 /* copy/paste to add more hardware */
				 digitalWrite(RELAY1PIN,0); // turn off relay
				 fprintf(stderr,"relay 1 OFF\n");

				 goto resumeLoop;
		     }
			 curtime = time(NULL);		// reset timer
		  }

		  /* see if client sent us anything */
		  ioctl(connfd, FIONREAD, &bytesAvailable);
		  if (bytesAvailable == 0) continue;

		  /* input available, get data */
		  n = read(connfd, recvBuff, sizeof(recvBuff)-1);
		  recvBuff[n] = 0;  	/* n is size of chars read */

		  /* test response from client */

		  /* client wants to know if we're still connected */
		  if (strncmp(recvBuff, "ping",4)==0) {
			  strcpy(sendBuff,"ok");
			  write(connfd, sendBuff, strlen(sendBuff));
			  continue;
		  }

		  /* terminate connection */
		  if (strncmp(recvBuff,"CLOSE",5)==0) {
			  close(connfd);
			  FLAG=0;
			  
			  /* copy/paste to add more hardware */
			  digitalWrite(RELAY1PIN,0); // turn off relay
			  fprintf(stderr,"relay 1 OFF\n");
			  fprintf(stderr,"Received CLOSE from client\n");

              goto resumeLoop;	// look for new connections
		  }

		 /* copy/paste these to add more hardware */

		 /* Relay #1 */
		 if (strncmp(recvBuff,"relay1_ON",9)==0) {
			 FLAG = 1;
			 digitalWrite(RELAY1PIN,1);	// turn on relay 
			 fprintf(stderr,"relay 1 ON\n");		// local display
			 strcpy(sendBuff,"r1on");
			 write(connfd, sendBuff, strlen(sendBuff));	// remote display
			 continue;
		 }
		 if (strncmp(recvBuff,"relay1_OFF",10)==0) {
		 	 FLAG = 0;
			 digitalWrite(RELAY1PIN,0);	// turn off relay 
			 fprintf(stderr,"relay 1 OFF\n");		// local display
			 strcpy(sendBuff,"r1off");
			 write(connfd, sendBuff, strlen(sendBuff));	// remote display
			 continue;
	     }



	}

	  goto resumeLoop;	/* connection broken - continue */
  }
  	  
  // should never get to this point
  fprintf(stderr,"Unexpected failure - stopping.\n");
  
  /* copy/paste to add more hardware */
  fprintf(stderr,"relay 1 OFF\n");
  digitalWrite(RELAY1PIN,0);		// turn OFF relay

  close(connfd);    
  return 0;
}
