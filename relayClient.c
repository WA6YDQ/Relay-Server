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
#define BUTTON1		4		/* GPIO 23 is ***hardware pin 16*** and wiringPi pin 4 */
#define BUTTON2   	5		/* GPIO 24 is ***hardware pin 18*** and wiringPi pin 5 */
#define BUTTON3		6		/* GPIO 25 is ***hardware pin 22*** and wiringPi pin 6 */
#define BUTTON4		27		/* GPIO 16 is ***hardware pin 36*** and wiringPi pin 27 */
#define LEDPTT		25		/* GPIO 26 is ***hardware pin 37*** and wiringPi pin 25 */
#define LEDNTWK		24		/* GPIO 19 is ***hardware pin 35*** and wiringPi pin 24 */


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
  /*
  int listenfd = 0,connfd = 0;
  int n=0;

  struct sockaddr_in serv_addr, cli_addr;
  int clilen = sizeof(cli_addr);
  char sendBuff[1025], recvBuff[1024];  
  int numrv;  
  */

  int sockfd = 0, n=0, bytesAvailable = 0;
  char sendBuff[1024], recvBuff[1024];
  struct sockaddr_in serv_addr;
  time_t sec_now, sec_past;

  /* set up the hardware ports */
  wiringPiSetup();
  pinMode(BUTTON1, INPUT);				// set button as input
  pullUpDnControl(BUTTON1, PUD_UP);		// pull up resistor

  pinMode(BUTTON2, INPUT);
  pullUpDnControl(BUTTON2, PUD_UP);

  pinMode(BUTTON3, INPUT);
  pullUpDnControl(BUTTON3, PUD_UP);

  pinMode(BUTTON4, INPUT);
  pullUpDnControl(BUTTON4, PUD_UP);

  pinMode(LEDNTWK, OUTPUT);			// led, lights when network is live
  pinMode(LEDPTT, OUTPUT);			// led, lights when relay is activated (response from server)

  digitalWrite(LEDNTWK, 0);		// turn OFF initially
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
  n = read(sockfd, recvBuff, sizeof(recvBuff)-1);		// look for 'READY' send at 1st connect
  recvBuff[n] = 0;
  if (strncmp(recvBuff,"READY",5)==0) {
	  printf("%s\n",recvBuff);
  } else {
	  printf("unknown server response: %s\n",recvBuff);
  }


  while (1) {		// do forever

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
		  fprintf(stderr,"Received unknown response from server: %s\n",recvBuff);
		  continue;
	  }
continue;

	  // look for button press - 
	  if (digitalRead(BUTTON1) == 0) {		// button 1 pressed
		  strcpy(sendBuff,"relay1_ON");
		  write(sockfd, sendBuff, strlen(sendBuff));
		  memset(recvBuff,0,sizeof(recvBuff));
		  n = read(sockfd, recvBuff, sizeof(recvBuff)-1);
		  recvBuff[n] = 0;
		  printf("reply received: %s\n",recvBuff);
		  if (strncmp(recvBuff,"r1on",4)==0) {		// good reply
			  digitalWrite(LEDPTT,1);
		  } else {
			  digitalWrite(LEDPTT,0);			// bad or no replay - no LED
		  }
		  // now wait until user releases the button
		  while (digitalRead(BUTTON1) ==0) ;		// do forever
		  // user released button
		  strcpy(sendBuff,"relay1_OFF");
		  write(sockfd, sendBuff, strlen(sendBuff));
		  memset(recvBuff,0,sizeof(recvBuff));
		  n = read(sockfd, recvBuff, sizeof(recvBuff)-1);
		  recvBuff[n] = 0;
		  printf("reply received: %s\n",recvBuff);
		  if (strncmp(recvBuff,"r1off",5)==0) {		// good reply
			digitalWrite(LEDPTT,0);			// LED off
		  } else {
			digitalWrite(LEDPTT,1);			// keep LED on
		  }
		  // debounce the button
		  usleep(80000);		// 80 msec
		  continue;
	}
  }

}
			
