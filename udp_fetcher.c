#pragma comment(lib, "ws2_32.lib")
#define WIN                 // WIN for Winsock and BSD for BSD sockets
#define WIN32                 // WIN for Winsock and BSD for BSD sockets
#define ENABLE_ZENTIMER
//----- Include files ---------------------------------------------------------
#include <stdio.h>          // Needed for printf()
#include <string.h>         // Needed for memcpy() and strcpy()
#ifdef WIN
  #include <windows.h>      // Needed for all Winsock stuff
  #include <time.h>
#endif
#ifdef BSD
  #include <stdlib.h>
  #include <unistd.h>
  #include <sys/types.h>    // Needed for sockets stuff
  #include <netinet/in.h>   // Needed for sockets stuff
  #include <sys/socket.h>   // Needed for sockets stuff
  #include <arpa/inet.h>    // Needed for sockets stuff
  #include <fcntl.h>        // Needed for sockets stuff
  #include <netdb.h>        // Needed for sockets stuff
#endif

#include "zentimer.h"

//----- Defines ---------------------------------------------------------------
#define  PORT_NUM           9600  // Port number used
#define  IP_ADDR      "x.x.x.x"   // IP address of Public server (*** HARDWIRED ***)
#define MPX_PORT 8545
#define MPX_HOST "x.x.x.x"        // MPX_HOST on lan

//===== Main program ==========================================================


void usleep(__int64 usec);
void request_handshake(int client_s,struct sockaddr_in server_addr);
void ping(int client_s,struct sockaddr_in server_addr);



ztimer_t PingTimer;
ztimer_t ConnectTimer;
int ping_enabled = 0;

#ifdef WIN
void main(void)
#endif
{
#ifdef WIN
  WORD wVersionRequested = MAKEWORD(2,2);       // Stuff for WSA functions
  WSADATA wsaData;                              // Stuff for WSA functions
#endif
  int                  client_s;      // Client socket descriptor
  int                  decoder_s;      // Client socket descriptor
  unsigned long int    noBlock;       // Non-blocking flag
  struct sockaddr_in   server_addr;   // Server Internet address
  struct sockaddr_in   mpxdecoder_addr;   // Server Internet address
  int                  addr_len;      // Internet address length
  //char                 out_buf[256]; // Output buffer for data
  char                 in_buf[1200];  // Input buffer for data
  int                  retcode;       // Return code
  int rxbytes=0;
  int txbytes=0;
#ifdef WIN
  // This stuff initializes winsock
  WSAStartup(wVersionRequested, &wsaData);
#endif

  // Create a socket
  client_s = socket(AF_INET, SOCK_DGRAM, 0);
  if (client_s < 0)
  {
    printf("*** ERROR - socket() failed \n");
    exit(-1);
  }
  decoder_s = socket(AF_INET, SOCK_DGRAM, 0);
  if (decoder_s < 0)
  {
    printf("*** ERROR - socket() failed \n");
    exit(-1);
  }

#ifdef WIN
  // Make client_s non-blocking
  noBlock = 1;
  ioctlsocket(client_s, FIONBIO, &noBlock);
#endif

  // Fill-in server socket's address information
  server_addr.sin_family = AF_INET;                 // Address family to use
  server_addr.sin_port = htons(PORT_NUM);           // Port num to use
  server_addr.sin_addr.s_addr = inet_addr(IP_ADDR); // IP address to use

  mpxdecoder_addr.sin_family = AF_INET;                 // Address family to use
  mpxdecoder_addr.sin_port = htons(MPX_PORT);           // Port num to use
  mpxdecoder_addr.sin_addr.s_addr = inet_addr(MPX_HOST); // IP address to use

  
//request_handshake(client_s,server_addr);
  // Wait to receive a message (need to spin loop on the receive)
  addr_len = sizeof(server_addr);
  retcode = 0;
	//ZenTimerStart(&PingTimer);
  ZenTimerStart(&ConnectTimer);
  while (1)
  {

	
	uint64_t seconds_since_connect=0;
	ZenTimerElapsed(&ConnectTimer, &seconds_since_connect); 	
	seconds_since_connect=seconds_since_connect/1000000;
	//printf("seconds start_connect: %lld\n",seconds_since_connect);
	if (seconds_since_connect > 3) {
	  printf("seconds start_connect: %lld\n",seconds_since_connect);
	  ZenTimerStop2(&ConnectTimer);
	  request_handshake(client_s, server_addr);
	  continue;
	}else{
	if (ping_enabled == 0) { ping_enabled = 1; }
    if (PingTimer.state == ZTIMER_INACTIVE) { ZenTimerStart(&PingTimer); }
	}

	
	
	rxbytes = recvfrom(client_s, in_buf, sizeof(in_buf), 0, (struct sockaddr *)&server_addr, &addr_len);
	if (rxbytes < 0) {
	//printf("Not recieving mpx\n");
	continue;
	}
	
	if (rxbytes>0) {
	  
	  
	  //printf("Received from server: %d time_since: %lf pingtimer: %lf\n", rxbytes,seconds_since_start,seconds_since_start2);
      

	  txbytes = sendto(decoder_s, in_buf, rxbytes, 0, (struct sockaddr *)&mpxdecoder_addr, sizeof(mpxdecoder_addr));
	 
		if (txbytes < 0)
		{
			printf("*** ERROR - sendto() failed \n");
			exit(-1);
		} else {
		uint64_t time_since_ping=0;
		ZenTimerElapsed(&PingTimer, &time_since_ping); 
		time_since_ping=time_since_ping/1000000;
			if ((ping_enabled == 1) && (time_since_ping > 4) && (seconds_since_connect<1)) {
			//printf("Timer Triggered %llu\n",time_since_ping);
			ping_enabled = 0;
			ping(client_s, server_addr);
			ZenTimerStart(&PingTimer);
			}		
		memset(&in_buf, 0, sizeof(in_buf));
		}

	}
    
  }

  
  // Close all open sockets
#ifdef WIN
  retcode = closesocket(client_s);
  if (retcode < 0)
  {
    printf("*** ERROR - closesocket() failed \n");
    exit(-1);
  }
#endif

#ifdef WIN
  // This stuff cleans-up winsock
  WSACleanup();
#endif
}



void usleep(__int64 usec) 
{ 
    HANDLE timer; 
    LARGE_INTEGER ft; 

    ft.QuadPart = -(10*usec); // Convert to 100 nanosecond interval, negative value indicates relative time

    timer = CreateWaitableTimer(NULL, TRUE, NULL); 
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0); 
    WaitForSingleObject(timer, INFINITE); 
    CloseHandle(timer); 
}


void request_handshake(int client_s, struct sockaddr_in server_addr) {

	char handshake_buffer[64];
	int tx = 0;
	ping_enabled = 0;

// Assign a message to buffer out_buf
	printf("Sending... Handshake timeout \n");
	strcpy(handshake_buffer, "handshake");

  // Now send the message to server.
	tx = sendto(client_s, handshake_buffer, (strlen(handshake_buffer) + 1), 0,(struct sockaddr *)&server_addr, sizeof(server_addr));
	if (tx < 0)
	{
    printf("*** ERROR HAND SHAKE - sendto() failed \n");
    exit(-1);
	}
  
return;
}


void ping(int client_s,struct sockaddr_in server_addr) {
char handshake_buffer[25]="P";
int tx;
// Assign a message to buffer out_buf
  
  printf("Sending... ping\n");
  //strcpy(handshake_buffer, "P");

  // Now send the message to server.
  tx = sendto(client_s, handshake_buffer, (strlen(handshake_buffer) + 1), 0,(struct sockaddr *)&server_addr, sizeof(server_addr));
  if (tx < 0)
  {
    printf("*** ERROR HAND SHAKE - sendto() failed \n");
    exit(-1);
  }
  
return;
}
