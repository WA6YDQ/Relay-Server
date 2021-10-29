/* Relay client - send network commands to server based on button press
 *
 * k theis 10/2021
 * runs on raspberry pi.
 * NOTE: only runs in IPv4
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
 * 		cc -o relayClient relayClient.c -lwiringPi
*/

#define SERVER_ADDR	"127.0.0.1"		/* CHANGE THIS TO YOUR SERVER IP ADDRESS! */
#define PORT 9000					/* server port - the server listens on this port # */

/* wire the relay circuit to the hardware pins defined below */
#define BUTTON1   	5		/* GPIO 24 is ***hardware pin 18*** and wiringPi pin 5 */
#define LEDPTT		24		/* GPIO 19 is ***hardware pin 35*** and wiringPi pin 24 */
/* copy/paste above to add new hardware */

#define DEBOUNCE        10000		/* switch debounce time (usec) */
#define DEBUG 0						/* set to 1 to display messages on stderr */


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

  int sockfd = 0, n=0, bytesAvailable = 0;
  char sendBuff[1024], recvBuff[1024];
  struct sockaddr_in serv_addr;
  time_t secs;
  int FLAG1 = 0, pingFLAG = 0;

  /* set up the hardware ports */
  wiringPiSetup();
  pinMode(BUTTON1, INPUT);			// set button as input
  pullUpDnControl(BUTTON1, PUD_UP);	// pull up resistor (still needs an external pull-up, but JIC)

  pinMode(LEDPTT, OUTPUT);			// led, lights when relay is activated (response from server)
  digitalWrite(LEDPTT, 0);			// set OFF initially
  /* copy/paste above to add new hardware */


  /* initialize the TCP/IP port */
  memset(recvBuff,0,sizeof(recvBuff));
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0))<0) {
	  fprintf(stderr,"Could not create socket\n");
	  return 1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);
  serv_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

recon:
  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
	  fprintf(stderr,"Connection Failed\n");
	  return 1;
  }

  memset(recvBuff,0,sizeof(recvBuff));
  n = read(sockfd, recvBuff, sizeof(recvBuff)-1);		// look for 'READY' sent at 1st connect
  recvBuff[n] = 0;
  if (strncmp(recvBuff,"READY",5)==0) {
	  fprintf(stderr,"%s\n",recvBuff);
  } else {
	  fprintf(stderr,"unknown server response: %s\n",recvBuff);
	  close(sockfd);		// something wrong, abort now
	  fprintf(stderr,"Closing connection\n");
	  return 1;
  }

  FLAG1=1;		// initialize for later
  secs = time(NULL);

  while (1) {		// do forever
	  usleep(5000);		// keep processor from hitting 100%

	  ioctl(sockfd, FIONREAD, &bytesAvailable);
	  if (bytesAvailable != 0) {		// server sent us a message
		  n = read(sockfd, recvBuff, sizeof(recvBuff)-1);
		  recvBuff[n]=0;

		  // see what was sent
		  if (DEBUG)
		  	  fprintf(stderr,"server sent: %s - ",recvBuff);
		  
		  if (strncmp(recvBuff,"PING",4)==0) {		// we got pinged - send response
			  strcpy(sendBuff,"OK");
			  write(sockfd, sendBuff, strlen(sendBuff));
			  if (DEBUG) 
			  	  fprintf(stderr,"sent OK\n");
			  secs = time(NULL);		// reset timeout clock
			  continue;		// done
		  }
		  
		  /* copy/paste to add hardware */
		  if (strncmp(recvBuff,"r1on",4)==0) {
			digitalWrite(LEDPTT,1);
			if (DEBUG) 
				fprintf(stderr,"relay 1 on\n");
			continue;
		  }
		  
		  /* copy/paste to add hardware */
		  if (strncmp(recvBuff,"r1off",5)==0) {
			digitalWrite(LEDPTT,0);
			if (DEBUG) 
				fprintf(stderr,"relay 1 off\n");
			continue;
		  }			
		  
		  fprintf(stderr,"Received unknown response from server: %s\n",recvBuff);
		  continue;
	  }

	  // user pressed button

	  /* copy/paste to add more hardware */
	  if (digitalRead(BUTTON1)==0 && FLAG1==0) continue;	// button currently pressed and ack'd
	  if (digitalRead(BUTTON1)==1 && FLAG1==1) continue;	// button currently released and ack'd

	  if (digitalRead(BUTTON1)==0 && FLAG1==1) {		// button 1 pressed
	          usleep(DEBOUNCE);	// debounce
		  if (digitalRead(BUTTON1)==1) continue;		// glitch
		  FLAG1 = 0;						// follows button value
		  strcpy(sendBuff,"relay1_ON");		// send command to server
		  write(sockfd, sendBuff, strlen(sendBuff));
		  continue;
	  }
	  
	  /* copy/paste to add more hardware */
	  // user released button
	  if (digitalRead(BUTTON1)==1 && FLAG1==0) {
		  usleep(DEBOUNCE);		// debounce
		  FLAG1 = 1;
		  strcpy(sendBuff,"relay1_OFF");
		  write(sockfd, sendBuff, strlen(sendBuff));
		  continue;
	  }
	
	  /* keep alive - no response from server, test connection */
     	 if (time(NULL) > secs+35) {
        	if (DEBUG) 
			 	fprintf(stderr,"pinging server\n");
        	secs = time(NULL);      // reset timer
        	strcpy(sendBuff,"ping");
        	write(sockfd, sendBuff, strlen(sendBuff));
        	n = read(sockfd, recvBuff, sizeof(recvBuff)-1);
			recvBuff[n] = 0;
			if (strncmp(recvBuff,"ok",2)==0) continue;		// still connected
			// lost our connection
			goto recon;
	  }


	 continue;		// not needed, but eases clarity

  }

}
			
