
#define  BSD                // WIN for Winsock and BSD for BSD sockets

//----- Include files --------------------------------------------------------
#include <stdio.h>          // Needed for printf()
#include <string.h>         // Needed for memcpy() and strcpy()
#include <stdlib.h>         // Needed for exit()
#include <unistd.h>
#include <error.h>
#ifdef WIN
  #include <windows.h>      // Needed for all Winsock stuff
#endif
#ifdef BSD
  #include <sys/types.h>    // Needed for sockets stuff
  #include <netinet/in.h>   // Needed for sockets stuff
  #include <sys/socket.h>   // Needed for sockets stuff
  #include <arpa/inet.h>    // Needed for sockets stuff
  #include <fcntl.h>        // Needed for sockets stuff
  #include <netdb.h>        // Needed for sockets stuff
  #include <time.h>
#endif

//----- Defines --------------------------------------------------------------
#define  PORT_NUM   7000    // Arbitrary port number for the server

int hex_print(char *buffer,int n);
int relay_client(int fetcher_s,int server_s, struct sockaddr_in fetcher_addr,struct sockaddr_in peer_addr);
struct sockaddr_in get_fetcher_handshake(int fetcher_s);
int pingscan(int server_s,struct sockaddr_in server_addr);
  double seconds_since_ping=0;
  time_t starttime_ping=0;
int pingscan_enabled=0;
//===== Main program =========================================================
int main()
{
#ifdef WIN
  WORD wVersionRequested = MAKEWORD(1,1);       // Stuff for WSA functions
  WSADATA wsaData;                              // Stuff for WSA functions
#endif
  int                  server_s;        // Server socket descriptor
  int                  fetcher_s;        // Server socket descriptor

  struct sockaddr_in   fetcher_addr;     // Server Internet address
  struct sockaddr_in   server_addr;     // Server Internet address

  struct sockaddr_in   client_addr;     // Client Internet address
  //struct sockaddr_in   fetcher_addr; // Fetcher Internet address

  struct in_addr       client_ip_addr;  // Client IP address
  struct in_addr       fetcher_ip_addr;  // Client IP address

  int                  addr_len;        // Internet address length
 // char                 out_buf[4096];   // Output buffer for data
 // char                 in_buf[4096];    // Input buffer for data
  int                  retcode;         // Return code

#ifdef WIN
  // This stuff initializes winsock
  WSAStartup(wVersionRequested, &wsaData);
#endif
  char buffer[256];
  setvbuf(stdout, buffer, _IOFBF, sizeof(buffer));

  // >>> Step #1 <<<
  // Create a socket
  //   - AF_INET is Address Family Internet and SOCK_DGRAM is datagram
  server_s = socket(AF_INET, SOCK_DGRAM, 0);
  if (server_s < 0)
  {
    printf("*** ERROR - socket() failed \n");
    exit(-1);
  }

  fetcher_s = socket(AF_INET, SOCK_DGRAM, 0);
  if (fetcher_s < 0)
  {
    printf("*** ERROR - socket() failed \n");
    exit(-1);
  }

const int enable = 1;
if (setsockopt(server_s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    printf("setsockopt(SO_REUSEADDR) failed");
if (setsockopt(fetcher_s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    printf("setsockopt(SO_REUSEADDR) failed");

#ifdef BSD
int optval = 5; // valid values are in the range [1,7] 1- low priority, 7 - high priority
int optlen = sizeof(optval);
int buffer_size = 1200;

int flags = fcntl(fetcher_s, F_GETFL, 0);
fcntl(fetcher_s, F_SETFL, flags | O_NONBLOCK);

setsockopt(server_s, SOL_SOCKET, SO_PRIORITY, &optval, optlen);
setsockopt(fetcher_s, SOL_SOCKET, SO_PRIORITY, &optval, optlen);

setsockopt(fetcher_s, SOL_SOCKET, SO_RCVBUF, (char*) &buffer_size, sizeof(buffer_size));
setsockopt(fetcher_s, SOL_SOCKET, SO_SNDBUF, (char*) &buffer_size, sizeof(buffer_size));

setsockopt(server_s, SOL_SOCKET, SO_RCVBUF, (char*) &buffer_size, sizeof(buffer_size));
setsockopt(server_s, SOL_SOCKET, SO_SNDBUF, (char*) &buffer_size, sizeof(buffer_size));

//  int flags2 = fcntl(server_s, F_GETFL, 0);
//  fcntl(server_s, F_SETFL, flags2 | O_NONBLOCK);

#endif

  fetcher_addr.sin_family = AF_INET;                 // Address family to use
  fetcher_addr.sin_port = htons(7111);           // Port num to use
  fetcher_addr.sin_addr.s_addr  = htonl(INADDR_ANY); // IP address to use

  // >>> Step #2 <<<
  // Fill-in my socket's address information
  server_addr.sin_family = AF_INET;                 // Address family to use
  server_addr.sin_port = htons(PORT_NUM);           // Port number to use
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // Listen on any IP address
  retcode = bind(server_s, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (retcode < 0)
  {
    printf("*** ERROR - bind() failed \n");
    exit(-1);
  }
  retcode = bind(fetcher_s, (struct sockaddr *)&fetcher_addr, sizeof(fetcher_addr));
  if (retcode < 0)
  {
    printf("*** ERROR - bind() failed \n");
    exit(-1);
  }

  // >>> Step #3 <<<
  // Wait to receive a message from client

  printf("Waiting for recvfrom() to complete... \n");
  //addr_len = sizeof(client_addr);
  starttime_ping = time(0);
  fetcher_addr = get_fetcher_handshake(fetcher_s);

while (1) {

  int ret = relay_client(fetcher_s, server_s, fetcher_addr, client_addr);

  if (ret == -1) {
    printf("restarting process...\n");
    memset(&fetcher_addr, 0, sizeof(fetcher_addr));
    fetcher_addr = get_fetcher_handshake(fetcher_s);
    usleep(1000000);
  }
sleep(5);
}

#ifdef WIN
  retcode = closesocket(server_s);
  if (retcode < 0)
  {
    printf("*** ERROR - closesocket() failed \n");
    exit(-1);
  }
#endif
#ifdef BSD
  retcode = close(server_s);
  if (retcode < 0)
  {
    printf("*** ERROR - close() failed \n");
    exit(-1);
  }
#endif

#ifdef WIN
  // This stuff cleans-up winsock
  WSACleanup();
#endif

  // Return zero and terminate
  return(0);
}

int relay_client(int fetcher_s,int server_s, struct sockaddr_in fetcher_addr,struct sockaddr_in peer_addr) {
    char in_buf[1200];    // Input buffer for data
    char *peer_ip = NULL;
    struct in_addr       peer_ip_addr;  // Client IP address

    //char out_buf[4096];
    int retcode = 0;
    int get_ip_flag = 0;
    int peer_port = 0;
    int addr_len = sizeof(peer_addr);

    printf("relay started\n");
    pingscan_enabled = 1;
    starttime_ping = time(0);

while (1) {


  retcode = recvfrom(server_s, in_buf, sizeof(in_buf), MSG_WAITALL,(struct sockaddr *)&peer_addr, &addr_len);
  if (retcode < 0)
  {
    printf("*** ERROR - recvfrom() failed \n");
  }
  if (retcode > 0) {
    if (get_ip_flag == 0) {
    memcpy(&peer_ip_addr, &peer_addr.sin_addr.s_addr, 4);
    get_ip_flag = 1;
    peer_port = ntohs(peer_addr.sin_port);
    peer_ip = inet_ntoa(peer_ip_addr);
    }
//  hex_print(in_buf,retcode);
    int txbytes = sendto(fetcher_s, in_buf, retcode,  MSG_CONFIRM,(struct sockaddr *)&fetcher_addr, sizeof(fetcher_addr));

    if (pingscan_enabled == 1) {
    int ping_status = pingscan(fetcher_s, fetcher_addr);
    if (ping_status == -1) {
    return -1;
    }

    }
    memset(&in_buf, 0, sizeof(in_buf));
    printf("[recv] [client = %s]  [port = %d]\n", peer_ip, peer_port);

  }


usleep(1);
}
}

int hex_print(char *buffer,int n) {
printf("packets: %d bytes\n",n);
for (int i = 0; i < n; i++)
{
    printf("{%x}", buffer[i]);
}
printf("\n");
}


int hostalive(int fetcher_s,struct sockaddr_in fetcher_addr) {

}

int pingscan(int fetcher_s,struct sockaddr_in fetcher_addr) {
  char in_buff[16];
  int fetcher_addrlen = sizeof(fetcher_addr);

  int ret = recvfrom(fetcher_s, in_buff, sizeof(in_buff),  MSG_WAITALL, (struct sockaddr *)&fetcher_addr, &fetcher_addrlen);
  if (ret < 0)
  {
    seconds_since_ping = difftime( time(0), starttime_ping);
    if (seconds_since_ping > 6) {
    printf("Client LOST!\n");
    return -1;
    }
    return 1;
  } else {
    if (in_buff[0]=='P') {
    starttime_ping = time(0);
    printf("ping received!!! %s\n",in_buff);
    return 2;
    }
  return 1;
  }

}

struct sockaddr_in get_fetcher_handshake(int fetcher_s) {

  struct in_addr       m_fetcher_ip_addr;  // Client IP address
  struct sockaddr_in   m_fetcher_addr;
  char in_buff[16];
  int ret;
  int m_fetcher_addrlen=sizeof(m_fetcher_addr);
  pingscan_enabled=0;
while (1) {
  ret = recvfrom(fetcher_s, in_buff, sizeof(in_buff),  MSG_WAITALL, (struct sockaddr *)&m_fetcher_addr, &m_fetcher_addrlen);
  if (ret < 0)
  {
    printf("*** ERROR NO HANDSHAKE DATA AVAILABLE - recvfrom() failed \n");
    //exit(-1);
    //return;
    usleep(1000000);
  } else {
    printf("Fetcher Logged in ok\n");
  // Copy the four-byte client IP address into an IP address structure
     memcpy(&m_fetcher_ip_addr, &m_fetcher_addr.sin_addr.s_addr, 4);

  // Print an informational message of IP address and port of the client
     printf("IP address of UDP_FETCHER = %s  port = %d) \n", inet_ntoa(m_fetcher_ip_addr),ntohs(m_fetcher_addr.sin_port));
    return m_fetcher_addr;
  }
}
}



