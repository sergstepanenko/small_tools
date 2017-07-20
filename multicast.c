#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>

#include <arpa/inet.h> //warning free
#include <ctype.h>     //warning free

#define BUFLEN 65536
//#define PORT 55555
//#define MULT_IP "225.1.1.1"
//#define PORT 4809
//#define MULT_IP "224.0.0.180"

 
void *send_multicast_datagram(void *ptr);
void *receive_multicast_datagram(void *ptr);
void *loopback_multicast_datagram(void *ptr);
void send_init (void);
void recv_init (void);

void handle_signals(int signo);

struct in_addr        localInterface;
struct sockaddr_in    groupSock;
struct sockaddr_in    localSock;
struct ip_mreq        group;
int                   sd;
int                   datalen;
char                  databuf[BUFLEN];

int                   sd2;
int                   datalen2;
char                  databuf2[BUFLEN];
struct ip_mreq        group2;

pthread_t threadSend = 0, threadRecv = 0;
const char *message1 = "Thread 1";
const char *message2 = "Thread 2";
int  iret1, iret2;
char * send_ip = NULL;
char * recv_ip = NULL;
char * MULT_IP = NULL;
int PORT = 4809;
sem_t mutex;


int packet_size = 1400; //64;
int interval = 0; // 1000 us = 1 ms
int recv_buf = 131072; // 128k

unsigned int count=0;
unsigned int count_send=0;
unsigned int count_recv=0;
unsigned int err_count=0;
unsigned int end_count=0;
unsigned int echo_packet_count=10000;
int loop=0;
int loopmain=0;

double bandwidth_sum=0;
unsigned int bandwidth_count=0;


int main (int argc, char *argv[])
{
  int c;
  opterr = 0;
  if (signal(SIGINT, handle_signals) == SIG_ERR) {
    fprintf (stderr, "failed to register interrupts with kernel\n");
  }
  
       while ((c = getopt (argc, argv, "m:s:r:p:i:c:b:vlL")) != -1)
         switch (c)
           {
           case 'm':
             MULT_IP = optarg;
             break;
           case 's':
             send_ip = optarg;
             break;
           case 'r':
	     recv_ip = optarg;
             break;
           case 'p':
             packet_size = atoi(optarg);
             break;
           case 'i':
             interval = atoi(optarg);
             break;
           case 'c':
             end_count = atoi(optarg);
             break;
           case 'b':
             recv_buf = atoi(optarg);
             break;
           case 'v':
             echo_packet_count=1;
	     break;
           case 'L':
             loop=1;
             break;
           case 'l':
             loopmain=1;
             break;
           case '?':
	   default:
             if ((optopt == 'm')||(optopt == 's')||(optopt == 'r')||(optopt == 'p')||(optopt == 'i')||(optopt == 'c'))
               fprintf (stderr, "Option -%c requires an argument.\n", optopt);
             else if (isprint (optopt))
               fprintf (stderr, "Unknown option `-%c'.\n", optopt);
             else
               fprintf (stderr,
                        "Unknown option character `\\x%x'.\nU",
                        optopt);
            fprintf (stderr, "Use: multicast -m <mult IP> [-s <sender IP>]|[-r <receiver IP>] [-p <packet size=1400>] [-b <recv buff=128k>] [-i <interval=0>] [-c <count=0(infinity)>] [-v] [-lL]\n");
	    fprintf (stderr, "Use parameter -l or -L only with -s AND -r!\n");
    	    fprintf (stderr, "Examples: \nmulticast -m 224.0.0.180 -s 192.168.3.101 -p 16000 -i 1000\nmulticast -m 224.0.0.180 -r 192.168.3.101 -p 16000\n");

             return 1;;
           }

	if ((loop)||(loopmain))
	{
	  if ((recv_ip)&&(send_ip))
	  {
	    if (loopmain)
	    {
      	      iret2 = pthread_create( &threadSend, NULL, send_multicast_datagram, (void*) send_ip);
	      if (threadSend) 
		pthread_join( threadSend, NULL);
	    }
	    else
	    {
		iret1 = pthread_create( &threadRecv, NULL, loopback_multicast_datagram, NULL);
		if (threadRecv) 
		  pthread_join( threadRecv, NULL);
	    }
	  }
	  else 
	  {
            fprintf (stderr, "Use: multicast -m <mult IP> [-s <sender IP>]|[-r <receiver IP>] [-p <packet size=1400>] [-b <recv buff=128k>] [-i <interval=0>] [-c <count=0(infinity)>] [-v] [-lL]\n");
	    fprintf (stderr, "Use parameter -l or -L only with -s AND -r!\n");
    	    fprintf (stderr, "Examples: \nmulticast -m 224.0.0.180 -s 192.168.3.101 -p 16000 -i 1000\nmulticast -m 224.0.0.180 -r 192.168.3.101 -p 16000\n");

	  }
	}
	else
	{
	    sem_init(&mutex, 0, 0);
	    
	    if (recv_ip)
	    {
	      iret2 = pthread_create( &threadRecv, NULL, receive_multicast_datagram, (void*) recv_ip);
	      sem_wait (&mutex);
	    }

	    if (send_ip)
	    {
	      iret2 = pthread_create( &threadSend, NULL, send_multicast_datagram, (void*) send_ip);
	    }
	    
	    if ((!recv_ip)&&(!send_ip))
            {
                fprintf (stderr, "Use: multicast -m <mult IP> [-s <sender IP>]|[-r <receiver IP>] [-p <packet size=1400>] [-b <recv buff=128k>] [-i <interval=0>] [-c <count=0(infinity)>] [-v] [-lL]\n");
                fprintf (stderr, "Use parameter -l or -L only with -s AND -r!\n");
                fprintf (stderr, "Examples: \nmulticast -m 224.0.0.180 -s 192.168.3.101 -p 16000 -i 1000\nmulticast -m 224.0.0.180 -r 192.168.3.101 -p 16000\n");
            }
	    /* Wait till threads are complete before main continues. Unless we  */
	    /* wait we run the risk of executing an exit which will terminate   */
	    /* the process and all threads before the threads have completed.   */

	    if (threadRecv) 
	      pthread_join( threadRecv, NULL);

	    if (threadSend) 
	      pthread_join( threadSend, NULL);
	}
	
	
	//printf("Thread 1 returns: %d\n",iret1);
	//printf("Thread 2 returns: %d\n",iret2);
	printf ("\nTotal packets %d, Errors %d\n", count_send, err_count);
	exit(0);
	
}

void handle_signals(int signo) {
  if ((signo == SIGINT)) { //&&(threadRecv)) {
     //printf ("\nPackets recv %d, total packets %d\n", count_recv, end_count);
    printf ("\nTotal packets %d, Errors %d (%lf good), Average Bandwidth = %lf Mbit/s\n", count_send?count_send:count_recv, err_count, (double)100 - (double)100*err_count/((count_send?count_send:count_recv)+err_count), bandwidth_sum / bandwidth_count);
  }
  close((ssize_t)stdout);
    close(sd);
    close(sd2);
  exit(0);
}

void send_init (void)
{
    /*
   * Create a datagram socket on which to send.
   */
  sd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sd < 0) {
    perror("opening datagram socket");
    exit(1);
  }
 
  /*
   * Initialize the group sockaddr structure with a
   * group address of MULT_IP and port PORT.
   */
  memset((char *) &groupSock, 0, sizeof(groupSock));
  groupSock.sin_family = AF_INET;
  groupSock.sin_addr.s_addr = inet_addr(MULT_IP);
  groupSock.sin_port = htons(PORT);
 
  /*
   * Disable loopback so you do not receive your own datagrams.
   */
  {
    char loopch=0;
 
    if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP,
                   (char *)&loopch, sizeof(loopch)) < 0) {
      perror("setting IP_MULTICAST_LOOP:");
      close(sd);
      exit(1);
    }
  }
 
    //char * client_ip;
    //client_ip = (char *) ptr; //argv[1];
    
    if (inet_aton(send_ip, &localInterface)==0) {
       perror("inet_aton() failed\n");
       exit(1);
    }

 
  /*
   * Set local interface for outbound multicast datagrams.
   * The IP address specified must be associated with a local,
   * multicast-capable interface.
   */
  //localInterface.s_addr = inet_addr("192.168.3.101");
  if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_IF,
                 (char *)&localInterface,
                 sizeof(localInterface)) < 0) {
    perror("setting local interface");
    exit(1);
  }
 
  /*
   * Join the multicast group MULT_IP on the local 9.5.1.1
   * interface.  Note that this IP_ADD_MEMBERSHIP option must be
   * called for each local interface over which the multicast
   * datagrams are to be received.
   */
  /*group.imr_multiaddr.s_addr = inet_addr(MULT_IP);
  group.imr_interface.s_addr = inet_addr(client_ip);  //INADDR_ANY; //
  if (setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                 (char *)&group, sizeof(group)) < 0) {
    perror("adding multicast group (sender)");
    close(sd);
    exit(1);
  } */

}

void *send_multicast_datagram(void *ptr)
{
  /* ------------------------------------------------------------*/
  /*                                                             */
  /* Send Multicast Datagram code.                               */
  /*                                                             */
  /* ------------------------------------------------------------*/

    if (loopmain)
    {
      recv_init();
    }
      fd_set rfds;
      struct timeval tv;
      int retval;
      
    struct timespec time1, time2, time3;
    double delta1, delta2;
    double max_delta1=0;
    double max_delta2=0;

  
  send_init();
  /*
   * Send a message to the multicast group specified by the
   * groupSock sockaddr structure.
   */
  datalen = packet_size; //10;
  clock_gettime( CLOCK_REALTIME, &time1);
  
    while((end_count==0)||(count_send<end_count))
    {
        if (loopmain)
	{
	   FD_ZERO(&rfds);
           FD_SET(sd2, &rfds);
	   tv.tv_sec = 1;
           tv.tv_usec = 0; //Timeout = 5 s
           
	   datalen2 = packet_size;
	}
      
      
      ++count_send;
      (*((unsigned int *)(databuf+4)))= count_send;
      
	if (sendto(sd, databuf, datalen, 0,
		  (struct sockaddr*)&groupSock,
		  sizeof(groupSock)) < 0)
	{
	  perror("sending datagram message");
          printf("sd=%d, databuf=0x%p[%d], &groupSock=0x%p[%d]\n", sd, databuf, datalen, (struct sockaddr*)&groupSock, (int)sizeof(groupSock));
	}
	//clock_gettime( CLOCK_REALTIME, &time1);

        if (count_send%echo_packet_count==0)
        {
            clock_gettime( CLOCK_REALTIME, &time2);
                delta1 = (double)1000L * (time2.tv_sec - time1.tv_sec) + (double)(time2.tv_nsec - time1.tv_nsec) / (double)1000000L;
                printf ("Send %d x %d bytes. Bandwidth= %6.2lf Mbit/s Time=%5.3f ms\n",
                        count_send, packet_size, (double)echo_packet_count*packet_size*1000/delta1/1024/128, delta1 );

                bandwidth_sum+=(double)echo_packet_count*packet_size*1000/delta1/1024/128;
                ++bandwidth_count;
            clock_gettime( CLOCK_REALTIME, &time1);
        }

        if ((loopmain))
	{
	   retval = select(sd2+1, &rfds, NULL, NULL, &tv);
           /* Don't rely on the value of tv now! */
	   //clock_gettime( CLOCK_REALTIME, &time2);
	   
           if (retval == -1)
               perror("select()");
           else if ((retval == 1)&&(FD_ISSET(sd2, &rfds)))
	     {
	       retval = read(sd2, databuf2, datalen2);
               	if (retval < 0) {
		  perror("reading datagram message");
		  close(sd2);
		  exit(1);
		}
		delta1 = (double)1000L * (time2.tv_sec - time1.tv_sec) + (double)(time2.tv_nsec - time1.tv_nsec) / (double)1000000L;
		//delta1 = (double)((double)time2.tv_nsec - (double)time1.tv_nsec) / (double)1000000L;
		if (delta1 > max_delta1)
		    max_delta1 = delta1;
		if (count_send ==  (*((unsigned int *)(databuf2+4)))) //databuf2[4])
		{
		  if (count_send%echo_packet_count==0)
		    printf ("Read #%d %5dB OK! Bandwidth= %5.1f Mb/s Time=%5.3f ms MaxTime=%5.3f ms\n", count_send, retval, 1000/delta1*packet_size/1024/128, delta1, max_delta1);
		  //fflush(stdout);
		}
		else
		{
  		  if (count_send%echo_packet_count==0)
		    printf ("*Read #%d %5dB OK! Bandwidth = %5.1f Mbit/s Time=%5.3f ms MaxTime=%5.3f ms Lag=%d packet(s)!\n", count_send, retval, 1000/delta1*packet_size/1024/128, delta1, max_delta1, count_send-(*((unsigned int *)(databuf2+4))));
		  
		  
		  	FD_ZERO(&rfds);
			FD_SET(sd2, &rfds);
			tv.tv_sec = 1; //Timeout = 1 s
			tv.tv_usec = 0;

			datalen2 = packet_size;
			
			while((select(sd2+1, &rfds, NULL, NULL, &tv) == 1)&&(FD_ISSET(sd2, &rfds)))
			  {
			      if (read(sd2, databuf2, datalen2) < 0) {
				perror("reading datagram message");
				close(sd2);
				exit(1);
			      }
			  }

		  //fflush(stdout);
		}
	     }
           else
		{
		  ++err_count;
		  delta1 = (double)1000L * (time2.tv_sec - time1.tv_sec) + (double)(time2.tv_nsec - time1.tv_nsec) / (double)1000000L;
		  printf ("****************************** Read Error! Timeout (%5.3f ms)!\n", delta1);
		  //fflush(stdout);
		  
		} 
		

		
		/*
		fsync(sd2);
		   FD_ZERO(&rfds);
           FD_SET(sd2, &rfds);
	   tv.tv_sec = 0;
           tv.tv_usec = 30000; //Timeout = 5 s
           
	   datalen2 = packet_size;
	
			   retval = select(sd2+1, &rfds, NULL, NULL, &tv);

	   clock_gettime( CLOCK_REALTIME, &time2);
	   
           if (retval == -1)
               perror("select()");
           else if ((retval == 1)&&(FD_ISSET(sd2, &rfds)))
	     {
	       retval = read(sd2, databuf2, datalen2);
               	if (retval < 0) {
		  perror("reading datagram message");
		  close(sd2);
		  exit(1);
		}
		delta1 = (double)1000L * (time2.tv_sec - time1.tv_sec) + (double)(time2.tv_nsec - time1.tv_nsec) / (double)1000000L;
		//delta1 = (double)((double)time2.tv_nsec - (double)time1.tv_nsec) / (double)1000000L;
		if (delta1 > max_delta1)
		    max_delta1 = delta1;
		if (count_send ==  (*((unsigned int *)(databuf2+4)))) //databuf2[4])
		{
		  if (count_send%echo_packet_count==0)
		    printf ("Read #%d %5dB OK! Bandwidth= %5.1f Mb/s Time=%5.3f ms MaxTime=%5.3f ms\n", count_send, retval, 1000/delta1*packet_size/1024/128, delta1, max_delta1);
		  //fflush(stdout);
		}
		else
		{
  		  if (count_send%echo_packet_count==0)
		    printf ("*Read #%d %5dB OK! Bandwidth = %5.1f Mb/s Time=%5.3f ms MaxTime=%5.3f ms Lag=%d packet(s)!\n", count_send, retval, 1000/delta1*packet_size/1024/128, delta1, max_delta1, count_send-(*((unsigned int *)(databuf2+4))));
		  
		  
		  	FD_ZERO(&rfds);
			FD_SET(sd2, &rfds);
			tv.tv_sec = 1; //Timeout = 1 s
			tv.tv_usec = 0;

			datalen2 = packet_size;
			
			while((select(sd2+1, &rfds, NULL, NULL, &tv) == 1)&&(FD_ISSET(sd2, &rfds)))
			  {
			      if (read(sd2, databuf2, datalen2) < 0) {
				perror("reading datagram message");
				close(sd2);
				exit(1);
			      }
			  }

		  //fflush(stdout);
		}
	     }
           else
		{
		  ++err_count;
		  delta1 = (double)1000L * (time2.tv_sec - time1.tv_sec) + (double)(time2.tv_nsec - time1.tv_nsec) / (double)1000000L;
		  printf ("****************************** Read Error! Timeout (%5.3f ms)!\n", delta1);
		  //fflush(stdout);
		  
		} 
		*/

	}
	//if (interval > 0)
        {
            //usleep(interval);
            for(volatile int i=0; i<interval; ++i)
                for(volatile int sleep=0; sleep<100; ++sleep)
                {
                    ;
                }
        }
    }
  close(sd);
  if (loopmain)
  {  
    close(sd2);
  }
  //printf ("\nTotal packets %d, Errors %d\n", count_send, err_count);
}

void recv_init (void)
{
  
  /*
   * Create a datagram socket on which to receive.
   */
  sd2 = socket(AF_INET, SOCK_DGRAM, 0);
  if (sd2 < 0) {
    perror("opening datagram socket");
    exit(1);
  }
 
  /*
   * Enable SO_REUSEADDR to allow multiple instances of this
   * application to receive copies of the multicast datagrams.
   */
  {
    int reuse=1;
 
    if (setsockopt(sd2, SOL_SOCKET, SO_REUSEADDR,
                   (char *)&reuse, sizeof(reuse)) < 0) {
      perror("setting SO_REUSEADDR");
      close(sd2);
      exit(1); 
    }
  }
  
    //char * srv_ip;
    //srv_ip = (char *) ptr; //argv[2];
    
    if (inet_aton(recv_ip, &group2.imr_interface)==0) {
       perror("inet_aton() failed\n");
       exit(1);
    }
  
   /*
   * Join the multicast group2 MULT_IP on the local 9.5.1.1
   * interface.  Note that this IP_ADD_MEMBERSHIP option must be
   * called for each local interface over which the multicast
   * datagrams are to be received.
   */
  group2.imr_multiaddr.s_addr = inet_addr(MULT_IP);
  //group2.imr_interface.s_addr = INADDR_ANY;
  
  if (setsockopt(sd2, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                 (char *)&group2, sizeof(group2)) < 0) {
    perror("adding multicast group2 (recv)");
    close(sd2);
    exit(1);
 } 
 
    /* SO_RCVBUF options */

    int len;
    int rcvbuf;
    
    len = sizeof(rcvbuf);
    getsockopt(sd2, SOL_SOCKET, SO_RCVBUF, &rcvbuf, &len);
    printf("defaults: SO_RCVBUF = %d\n", rcvbuf);

    rcvbuf = recv_buf; 
    setsockopt(sd2, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));
    len = sizeof(rcvbuf);
    getsockopt(sd2, SOL_SOCKET, SO_RCVBUF, &rcvbuf, &len);
    printf("SO_RCVBUF = %d\n", rcvbuf); 

    /* SO_RCVBUF options */
 
 
  /*
   * Bind to the proper port number with the IP address
   * specified as INADDR_ANY.
   */
  memset((char *) &localSock, 0, sizeof(localSock));
  localSock.sin_family = AF_INET;
  localSock.sin_port = htons(PORT);;
  localSock.sin_addr.s_addr  = INADDR_ANY;
 
  if (bind(sd2, (struct sockaddr*)&localSock, sizeof(localSock))) {
    perror("binding datagram socket");
    close(sd2);
    exit(1);
  }
   
}
//-----------------------------------------------------------------------------------------

void *receive_multicast_datagram(void *ptr)
{
 
  /* ------------------------------------------------------------*/
  /*                                                             */
  /* Receive Multicast Datagram code.                            */
  /*                                                             */
  /* ------------------------------------------------------------*/

  recv_init();
  
  
   fd_set rfds;
   struct timeval tv;
   int retval;

   struct timespec time1, time2, time3;
   double delta1;

   time1.tv_sec = 0;

           /* Watch sd2 to see when it has input. */

 
  unsigned int offset = 0;

 sem_post (&mutex);
 //pthread_cond_signal( &condition_var );
 //pthread_mutex_unlock( &mutex1 );
 

        uint8_t* pStart = (uint8_t*)databuf2;
        uint8_t* pDest = pStart;
        uint8_t* pEnd = pStart + packet_size;


        clock_gettime( CLOCK_REALTIME, &time1);
  /*
   * Read from the socket.
   */
    while ((end_count==0)||((count<end_count)&&(offset<end_count)))
    {

           FD_ZERO(&rfds);
           FD_SET(sd2, &rfds);
	   tv.tv_sec = 1;
           tv.tv_usec = 0;

	   datalen2 = packet_size; //sizeof(databuf2);


           retval = select(sd2+1, &rfds, NULL, NULL, &tv);
           /* Don't rely on the value of tv now! */

           if (retval == -1)
               perror("select()");
           else if ((retval == 1)&&(FD_ISSET(sd2, &rfds)))
	     {
                 do
                 {
                     pDest = pStart;
                     while (pDest < pEnd)
                     {
                        retval = read(sd2, pDest, pEnd-pDest);
                        if (retval < 0) {
                        perror("reading datagram message");
                        close(sd2);
                        exit(1);
                        }
                        if (retval == 0)
                            break;
                        pDest += retval;
                     }

                     retval = (pDest - pStart);
                     if (retval == packet_size)
                     {

                        if (offset+1 ==  (*((unsigned int *)(databuf2+4)))) //databuf2[4])
                        {
                            ++count_recv;
                            //fflush(stdout);
                        }
                        else
                        {
                            ++count_recv;
                            if (offset > 0)
                                err_count+=(*((unsigned int *)(databuf2+4)))-(offset+1);
                            //printf ("Read Error! %d Packet(s) loss!\n", (*((unsigned int *)(databuf2+4)))-(offset+1));
                            //fflush(stdout);
                        }

                        if (count_recv%echo_packet_count==0)
                        {
                            clock_gettime( CLOCK_REALTIME, &time2);
                                delta1 = (double)1000L * (time2.tv_sec - time1.tv_sec) + (double)(time2.tv_nsec - time1.tv_nsec) / (double)1000000L;
                                printf ("Read %d x %d bytes. err_count=%d (Packet #%d) Bandwidth= %6.2lf Mbit/s Time=%5.3f ms\n",
                                        count_recv, packet_size, err_count, offset+1, (double)echo_packet_count*packet_size*1000/delta1/1024/128, delta1 );
                            bandwidth_sum+=(double)echo_packet_count*packet_size*1000/delta1/1024/128;
                            ++bandwidth_count;

                            clock_gettime( CLOCK_REALTIME, &time1);
                        }

                        offset = (*((unsigned int *)(databuf2+4)));
                     }
                 } while (retval>0);
	     }
           else
		{
		  //++err_count;
		  printf ("Read Error! Timeout!\n");
		  //fflush(stdout);
		} 
	++count;
	//usleep(interval);
    }
 printf ("Packets recv %d, total packets %d\n", count_recv, end_count);
 close(sd2);
}

//-----------------------------------------------------------------------------------------

void *loopback_multicast_datagram(void *ptr)
{
  recv_init();
  send_init();

    
   fd_set rfds;
   struct timeval tv;
   int retval;
 
  unsigned int offset = 0;
  /*
   * Read from the socket.
   */
    while ((end_count==0)||((count<end_count)&&(offset<end_count)))
    {

           FD_ZERO(&rfds);
           FD_SET(sd2, &rfds);
	   tv.tv_sec = 0;
           tv.tv_usec = interval*2+1000;

	   datalen2 = packet_size; //sizeof(databuf2);


           retval = select(sd2+1, &rfds, NULL, NULL, &tv);
           /* Don't rely on the value of tv now! */

           if (retval == -1)
               perror("select()");
           else if ((retval == 1)&&(FD_ISSET(sd2, &rfds)))
	     {
               	if (read(sd2, databuf2, datalen2) < 0) {
		  perror("reading datagram message");
		  close(sd2);
		  exit(1);
		}
		if (sendto(sd, databuf2, datalen2, 0,
		  (struct sockaddr*)&groupSock,
		  sizeof(groupSock)) < 0)
		{
		  perror("sending datagram message");
                  printf("sd=%d, databuf2=0x%p[%d], &groupSock=0x%p[%d]", sd, databuf2, datalen, (struct sockaddr*)&groupSock, (int)sizeof(groupSock));
		}
	     }
           else
		{
		  //++err_count;
		  printf ("Read Error! Timeout!\n");
		  //fflush(stdout);
		} 
	++count;
	//usleep(interval);
    }
 printf ("Packets %d, total packets %d\n", count, end_count);
 close(sd2);
 close(sd);
}




