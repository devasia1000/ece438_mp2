#include "graph.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <set>
#include <sstream>
#include <pthread.h>

#define BACKLOG 20   // how many pending connections queue will hold
#define MAXDATASIZE 500
#define PORT "6000"

// START OF FUNCTION DEFINITIONS
void *contactManager(void *ptr);
void handle_routing_table_update(char buf[MAXDATASIZE]); // handles all updates to routing tables
void *startServer(void *ptr); // listens to incoming connections from neighbours
void *handle_client(void *ptr); // handle a single client connection
void add_sockfd(int virtual_id); // opens a socket to nighbour using and stores sockfd in 'sock_fd' vector
// END OF FUNCTION DEFINITIONS

// START OF GLOBAL VARIABLES
int top[MAX_NODE_COUNT][MAX_NODE_COUNT]; // stores topology information
int virtual_id = -1;
string manager_ip = "localhost"; // ip address of the manager, TODO: PASS IN AS ARG LATER
string port = "6000"; // port number of the manager, TODO: PASS IN AS ARG LATER
vector<char*> ip_addresses; // hold ip addresses of neighbours
vector<int> sock_fd; // hold a sock fd corresponding to each neighbour
// END OF GLOBAL VARIABLES

void sigchld_handler(int s) {
  while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

char* convertToString(int number) {
  stringstream ss;
  //create a stringstream
  ss << number;
  //add number to the stream
  string result = ss.str();
  char *a=new char[result.size()+1];
  a[result.size()]=0;
  memcpy(a, result.c_str(), result.size());
  return a;
}

int main() {
  
  // clear neighbour ip address information
  ip_addresses.resize(20);
  for(unsigned int i=0 ; i<ip_addresses.size() ; i++){
    ip_addresses[i] = new char[80];
    ip_addresses[i][0] = '\0';
  }

  sock_fd.resize(20);
  for(unsigned int i=0 ; i<sock_fd.size() ; i++){
    sock_fd[i] = 0;
  }

  // clear topology data
  for (int i=0 ; i<MAX_NODE_COUNT ; i++) {
    for (int j=0 ; j<MAX_NODE_COUNT ; j++) {
      top[i][j] = 0;
    }
  }

  pthread_t manager_thread;
  pthread_create( &manager_thread, NULL, contactManager, NULL);
  pthread_join(manager_thread, NULL);
  
  // TODO: uncomment out later
  pthread_t link_state_listener;
  pthread_create(&link_state_listener, NULL, startServer, NULL);
  pthread_join(link_state_listener, NULL);

  // TODO: need another thread to check for convergence and print routing table
}

// contacts the manager and gets virtual id
void *contactManager(void *ptr) {

  int sockfd, numbytes;
  char buf[MAXDATASIZE];
  struct addrinfo hints, *servinfo, *p;
  int rv;
  char s[INET6_ADDRSTRLEN];

  /* clear the hints structure */
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(manager_ip.c_str(), port.c_str(), &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    exit(0);
  }

  /* loop through all the results and connect to the first we can */
  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("client: socket");
      continue;
    }

    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("client: connect");
      continue;
    }
    break;
  }

  if (p == NULL) {
    fprintf(stderr, "client: failed to connect\n");
    exit(0);
  }

  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
  printf("client: connecting to %s\n", s);
  freeaddrinfo(servinfo);

  while(1) {

    /* read data from server */
    numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0);
    if (numbytes == -1) {
      /* error has occured with read */
      perror("recv");
      exit(0);
    }

    else if (numbytes > 0) {

      cout<<"read is done\n";

      /* read is sucsessful */
      if(virtual_id == -1) {
        cout<<"going to assign virtual id\n";
        if(buf[1] == '\0') {
          virtual_id = buf[0];
        }

        else {
          virtual_id = 10*buf[0] + buf[1];
        }

        virtual_id = virtual_id - 48; // account for conversion from ASCII to int
        printf("assigned virtual_id: %d\n", virtual_id);
      }

      else {

        cout<<buf<<"\n";

        /*
          Format of neighbour information: 
          virtual_id:ip_address
          eg: 2:192.168.1.2 
        */

        char v_id[MAXDATASIZE], ip[MAXDATASIZE];
        int v=0, p=0, i=0;
        for(; i<MAXDATASIZE ; i++){
          if(buf[i] != ':'){
            v_id[v] = buf[i];
            v++;
          }

          else{
            v_id[v] = '\0';
            i++;
            break;
          }
        }

        for(; i<MAXDATASIZE ; i++){
          ip[p] = buf[i];
          p++;
        }

        int id;
        if(v_id[1] == '\0') {
          id = v_id[0];
        }

        else {
          id = 10*v_id[0] + v_id[1];
        }

        id = id - 48; // account for conversion from ASCII to int

        strcpy(ip_addresses[id], ip);

        cout<<"for virtual id "<<id<<", stored ip address "<<ip_addresses[id]<<"\n";

        add_sockfd(virtual_id);

        // TODO: set TTL to '20' and broadcast message to all neighbours
      }
    }

    else if (numbytes == 0) {
      /* socket has been closed */
      close(sockfd);
      printf("closing socket\n");
    }
  }
}

void add_sockfd(int virtual_id){
  int port = atoi(PORT) + virtual_id;

  int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(ip_addresses[virtual_id], convertToString(port), &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "failed to bind socket\n");
        exit(1);
    }

    sock_fd[virtual_id] = sockfd;
}

// listens for incoming connections from neighbours
void *startServer(void *ptr) {

  sleep(3); // wait to get virtual id and neighbour information from manager

  char server_port[80];
  strcpy(server_port, port.c_str());
  if(virtual_id < 10){
    server_port[1] = '\0';
  } else if (virtual_id > 10){
    server_port[2] = '\0';
  }

  int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr; // connector's address information
  socklen_t sin_size;
  struct sigaction sa;
  int yes=1;
  char s[INET6_ADDRSTRLEN];
  int rv;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // use my IP

  if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    exit(1);
  }

  // loop through all the results and bind to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
                      p->ai_protocol)) == -1) {
          perror("server: socket");
          continue;
      }

      if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                  sizeof(int)) == -1) {
          perror("setsockopt");
          exit(1);
      }

      if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
          close(sockfd);
          perror("server: bind");
          continue;
      }

      break;
  }

  if (p == NULL)  {
      fprintf(stderr, "server: failed to bind\n");
      exit(1);
  }

  freeaddrinfo(servinfo); // all done with this structure

  if (listen(sockfd, BACKLOG) == -1) {
      perror("listen");
      exit(1);
  }

  sa.sa_handler = sigchld_handler; // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
      perror("sigaction");
      exit(1);
  }

  printf("server: waiting for connections...\n");

  while(1) {  // main accept() loop

      sin_size = sizeof their_addr;
      new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
      if (new_fd == -1) {
          perror("accept");
          continue;
      }

      inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
      printf("server: got connection from %s\n", s);

      // create new thread to handle client
      pthread_t client_manager;
      pthread_create( &client_manager, NULL, handle_client, &new_fd);
  }
}

// handle a single client connection
void *handle_client(void *ptr){
  int *p = (int*) ptr;
  int sockfd = *p;

  char buf[MAXDATASIZE];

  while(1){

    /* read data from server */
    int numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0);
    if (numbytes == -1) {
      /* error has occured with read */
      perror("recv");
      exit(0);
    }

    else if (numbytes > 0) {
      // buf now contains routing table information
      handle_routing_table_update(buf);

      // TODO: decrement TTL and send message to all neighbours

    }

    else if (numbytes == 0) {
      /* socket has been closed */
      close(sockfd);
      printf("closing socket\n");
    }
  }
}

// handle updates to the routing table
void handle_routing_table_update(char buf[MAXDATASIZE]){
  int k=0;
  for(int i=0 ; i<MAX_NODE_COUNT ; i++){
    for(int j=0 ; j<MAX_NODE_COUNT ; j++){
      top[i][j] = buf[k];
      k++;
    }
  }
}