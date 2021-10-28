/* Relay client - send network commands to server based on button press
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
 * 		cc -o relayClient relayClient.c -lwiringPi
*/

#define SERVER_ADDR	"127.0.0.1"
#define PORT 9000			/* server port - this server listens on this port # */

/* wire the relay circuit to the hardware pins defined below */
#define BUTTON1   	5		/* GPIO 24 is ***hardware pin 18*** and wiringPi pin 5 */
#define LEDPTT		24		/* GPIO 19 is ***hardware pin 35*** and wiringPi pin 24 */
#define DEBOUNCE        10000		/* switch debounce time (usec) */

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
  time_t sec_now, sec_past;
  int FLAG1 = 0;

  /* set up the hardware ports */
  wiringPiSetup();
  pinMode(BUTTON1, INPUT);			// set button as input
  pullUpDnControl(BUTTON1, PUD_UP);		// pull up resistor

  pinMode(LEDPTT, OUTPUT);			// led, lights when relay is activated (response from server)
  digitalWrite(LEDPTT, 0);

  /* initialize the TCP/IP port */
  memset(recvBuff,0,sizeof(recvBuff));
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0))<0) {
	  fprintf(stderr,"Could not create socket\n");
	  return 1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);
  serv_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
	  fprintf(stderr,"Connection Failed\n");
	  return 1;
  }

  memset(recvBuff,0,sizeof(recvBuff));
  n = read(sockfd, recvBuff, sizeof(recvBuff)-1);		// look for 'READY' sent at 1st connect
  recvBuff[n] = 0;
  if (strncmp(recvBuff,"READY",5)==0) {
	  printf("%s\n",recvBuff);
  } else {
	  fprintf(stderr,"unknown server response: %s\n",recvBuff);
	  close(sockfd);		// something wrong, abort now
	  fprintf(stderr,"Closing connection\n");
	  return 1;
  }

  FLAG1=1;		// initialize for later

  while (1) {		// do forever
	  usleep(5000);		// keep processor from hitting 100%

	  ioctl(sockfd, FIONREAD, &bytesAvailable);
	  if (bytesAvailable > 0) {		// server sent up a message
		  n = read(sockfd, recvBuff, sizeof(recvBuff)-1);
		  recvBuff[n]=0;
		  // see what was sent
		  if (strncmp(recvBuff,"PING",4)==0) {		// we got pinged - send response
			  strcpy(sendBuff,"OK");
			  write(sockfd, sendBuff, strlen(sendBuff));
			  continue;		// done
		  }
		  if (strncmp(recvBuff,"r1on",4)==0) {
			digitalWrite(LEDPTT,1);
			fprintf(stderr,"relay 1 on\n");
			continue;
		  }
		  if (strncmp(recvBuff,"r1off",5)==0) {
			digitalWrite(LEDPTT,0);
			fprintf(stderr,"relay 1 off\n");
			continue;
		  }			
		  fprintf(stderr,"Received unknown response from server: %s\n",recvBuff);
		  continue;
	  }

	  // user pressed button
	  if (digitalRead(BUTTON1)==0 && FLAG1) {		// button 1 pressed
	          usleep(DEBOUNCE);	// debounce
		  FLAG1 = 0;						// follows button value
		  strcpy(sendBuff,"relay1_ON");		// send command to server
		  write(sockfd, sendBuff, strlen(sendBuff));
		  continue;
	  }
	  
	  // user releases button
	  if (digitalRead(BUTTON1)==1 && !FLAG1) {
		  usleep(DEBOUNCE);		// debounce
		  FLAG1 = 1;
		  strcpy(sendBuff,"relay1_OFF");
		  write(sockfd, sendBuff, strlen(sendBuff));
		  continue;
	  }

  }

}
			
