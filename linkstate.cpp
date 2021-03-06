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
#define MAXDATASIZE 3000
#define VIRTUAL_MAXDATASIZE 10
#define PORT "6000"

// START OF GLOBAL VARIABLES
long timestamp = -1;
int top[MAX_NODE_COUNT][MAX_NODE_COUNT]; // stores topology information
int virtual_id = -1;
char manager_ip[80]; // ip address of the manager
vector<char*> ip_addresses; // hold ip addresses of neighbours
vector<int> sock_fd; // hold a sock fd corresponding to each neighbour
bool table_changed;

struct update{
    bool neighbour_update; // set this to true if this is a neighbour update
    int sender_id;
    int ttl;
    char neighbours[MAX_NODE_COUNT][20];
    int top[MAX_NODE_COUNT][MAX_NODE_COUNT];

    bool message_update; // set this to true if this is a message update
    int source;
    int dest;
    char mess[200];
    int hops[MAX_NODE_COUNT];
    int hops_pos;
  };

  vector<update> message_list;
  vector<PathInfo> info_list;
// END OF GLOBAL VARIABLES


// START OF FUNCTION DEFINITIONS
  void *contactManager(void *ptr);
void handle_routing_table_update(int x[MAX_NODE_COUNT][MAX_NODE_COUNT]); // handles all updates to routing tables
void update_timestamp(); // update the convergence timestamp
void *startServer(void *ptr); // listens to incoming connections from neighbours
void *handle_client(void *ptr); // handle a single client connection
void add_sockfd(int virtual_id); // opens a socket to nighbour using and stores sockfd in 'sock_fd' vector
void *convergence_checker(void *ptr); // checks for convergence
void dijkstra(int graph[MAX_NODE_COUNT][MAX_NODE_COUNT], int src);
int get_next_hop(update mess); // get next hop for a message
// END OF FUNCTION DEFINITIONS


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

int main(int argc, char **argv) {

  if(argc != 2){
    cerr<<"Usage: ./link_state <manager_ip_address>\n";
    return 1;
  }

  strcpy(manager_ip, argv[1]);

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

  //cout<<__func__<<" : started manager thread\n";
  pthread_t manager_thread;
  pthread_create( &manager_thread, NULL, contactManager, NULL);

  //cout<<__func__<<" : started convergence checker thread\n";
  pthread_t convergence;
  pthread_create(&convergence, NULL, convergence_checker, NULL);
  pthread_join(convergence, NULL);
}

void *convergence_checker(void *ptr){
  while(1){

    sleep(5);

    long cur_time = time(0);
    if(cur_time - timestamp > 5 && timestamp != 0){
      //cout<<"\n";
      //cout<<__func__<<" : Convergence Detected! Printing out routing table...\n";

      graph g(virtual_id);
      for(int i=0 ; i<MAX_NODE_COUNT ; i++){
        for(int j=0 ; j<MAX_NODE_COUNT ; j++){
          if(top[i][j] > 0 && top[i][j] != INT_MAX && top[i][j] != 6000){
            g.addLink(i, j, top[i][j]);
          }
        }
      }

      info_list = g.getPathInformation();

      for(unsigned int i=0 ; i<info_list.size() ; i++){

        if(table_changed == true){

          PathInfo path = info_list[i];
          cout<<path.destination<<" "<<path.cost<<": ";

          for(unsigned int j=0 ; j<path.path.size() ; j++){
            cout<<path.path[j]<<" ";
          }
          cout<<"\n";
        }
      }

      table_changed = false;

      sleep(10);

      // traverse message list backwards to get earliest message
      for(int i=message_list.size()-1 ; i>=0 ; i--){

        update up = message_list[i];

        up.neighbour_update = false;
        up.message_update = true;
        up.hops[up.hops_pos] = virtual_id;
        up.hops_pos++;

        int next_hop = get_next_hop(up);
        //cout<<"got next hop as "<<next_hop<<" for message to "<<up.dest<<"\n";

        if(next_hop > 0){ // NOTE: next_hop will be -1 if node hasn't connected yet

          cout<<"from "<<up.source<<" to "<<up.dest<<" hops ";
        for(int i=0 ; i<up.hops_pos ; i++){
          cout<<up.hops[i]<<" ";
        }
        cout<<"message "<<up.mess<<"\n";

        char buf[MAXDATASIZE];
        memcpy(buf, &up, sizeof(update));

        if (send(sock_fd[next_hop], buf, MAXDATASIZE, 0) == -1){
          //perror("send");
        }
      }
    }

    timestamp = 0;
  }
}

exit(0);
}

void update_timestamp(){
  timestamp = (long) time(0);
}

// contacts the manager and gets virtual id
void *contactManager(void *ptr) {

  int sockfd, numbytes;
  char buf[MAXDATASIZE];
  struct addrinfo hints, *servinfo, *p;
  int rv;
  char s[INET6_ADDRSTRLEN];

  string port = PORT;

/* clear the hints structure */
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(manager_ip, port.c_str(), &hints, &servinfo)) != 0) {
    //fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
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
    //fprintf(stderr, "client: failed to connect\n");
    exit(0);
  }

  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
  //printf("client: connecting to %s\n", s);
  freeaddrinfo(servinfo);

  while(1) {

/* read data from server */
    numbytes = recv(sockfd, buf, MAXDATASIZE, MSG_WAITALL);
    if (numbytes == -1) {
  /* error has occured with read */
      perror("recv");
      exit(0);
    }

    else if (numbytes > 0) {

  /* read is sucsessful */
      if(virtual_id == -1) {
        //cout<<__func__<<" : going to assign virtual id\n";
        if(buf[1] == '\0') {
          virtual_id = buf[0];
        }

        else {
          virtual_id = 10*buf[0] + buf[1];
        }

    virtual_id = virtual_id - 48; // account for conversion from ASCII to int
    //cout<<__func__<<" assigned virtual id "<<virtual_id<<"\n";

    pthread_t link_state_listener;
    pthread_create(&link_state_listener, NULL, startServer, NULL);
    //cout<<__func__<<" : started link state listener thread\n";
  }

  else {

    update info;
    memcpy(&info, buf, sizeof(update));

    if(info.neighbour_update == true){

      //cout<<"recived neighbour update from manager\n";

      update_timestamp();

      handle_routing_table_update(info.top);

      for(int i=0 ; i<MAX_NODE_COUNT ; i++){
        if(ip_addresses[i][0] == '\0' && info.neighbours[i][0] != '\0' && i != virtual_id && top[virtual_id][i] > 0){
        //cout<<"updated ip address for "<<i<<" : "<<ip_addresses[i]<<"\n";
          ip_addresses[i] = new char[20];
          strcpy(ip_addresses[i], info.neighbours[i]);
        //cout<<"connected to "<<i<<"\n";
          add_sockfd(i);
        }
      }

    //cout<<__func__<<" : recieved neighbour information from manager:\n";

    // ENCODE INFORMATION INTO STRUCT
      char mess[MAXDATASIZE];
      update message;

      message.neighbour_update = true;
      message.message_update = false;

      message.sender_id = virtual_id;
      message.ttl = 5;
      for(int i=0 ; i<MAX_NODE_COUNT ; i++){
        for(int j=0 ; j<MAX_NODE_COUNT ; j++){
          message.top[i][j] = top[i][j];
        }
      }

      sleep(2);

      memcpy(mess, &message, sizeof(update));
      for(int i=0 ; i<MAX_NODE_COUNT ; i++){
        if(top[virtual_id][i] > 0){
          if (send(sock_fd[i], mess, MAXDATASIZE, 0) == -1){
            perror("send");
          }
        }
      }

    //cout<<__func__<<" : sent routing information to neighbour\n";
      //update_timestamp();
    }

    else if (info.message_update == true){

      //cout<<"recived message update from manager\n";
      message_list.push_back(info); // store message to be sent after convergence
    }
  }

}

else if (numbytes == 0) {
      /* socket has been closed */
  close(sockfd);
  //printf("closing socket\n");
}
}
}

void add_sockfd(int v_id){

  usleep(100);

  int port = atoi(PORT) + v_id;
  ostringstream oss;
  oss<<port;
  string po = oss.str();

  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  int rv;
  char s[INET6_ADDRSTRLEN];

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(ip_addresses[v_id], po.c_str(), &hints, &servinfo)) != 0) {
    //fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    exit(1);
  }

// loop through all the results and connect to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
      p->ai_protocol)) == -1) {
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
  //fprintf(stderr, "client: failed to connect\n");
  exit(1);
}

inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
  s, sizeof s);
//printf("client: connecting to %s\n", s);

freeaddrinfo(servinfo); // all done with this structure

sock_fd[v_id] = sockfd;

//cout<<"stored socket fd for "<<v_id<<"\n";

}

// listens for incoming connections from neighbours
void *startServer(void *ptr) {

  char server_port[80];
  string po = PORT;
  strcpy(server_port, po.c_str());
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

int port = atoi(PORT) + virtual_id;

if ((rv = getaddrinfo(NULL, convertToString(port), &hints, &servinfo)) != 0) {
  //fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
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
  //fprintf(stderr, "server: failed to bind\n");
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

//cout<<"Neighbour server: waiting for connections on port "<<convertToString(port)<<"\n";

while(1) {  // main accept() loop

  sin_size = sizeof their_addr;
  new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
  if (new_fd == -1) {
    perror("accept");
    continue;
  }

  inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
  //printf("Neighbour server: got connection from %s\n", s);

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

    usleep(50);

    // read data from connected client 
    int numbytes = recv(sockfd, buf, MAXDATASIZE, MSG_WAITALL);
    //cout<<"recvieved "<<numbytes<<" bytes from client\n";

    if (numbytes == -1) {
      perror("recv");
      exit(0);
    }

    else if (numbytes > 0) {

      //update_timestamp();

      // decode information from char array into struct
      update info;
      memcpy(&info, buf, sizeof(update));

      if(info.neighbour_update == true){

        //unsigned int prev_virtual_id = info.sender_id;
        info.neighbour_update = true;
        info.message_update = false;

        unsigned int prev_virtual_id = info.sender_id;
        info.sender_id = virtual_id;
        info.ttl--;

      //cout<<"recieved info from "<<prev_virtual_id<<" with ttl "<<info.ttl<<"\n";

        // ENCODE STRUCT INTO CHAR ARRAY AND SENd
        char buf2[MAXDATASIZE];
      //cout<<"bytes read"<<numbytes<<"\n";
        memcpy(buf2, &info, sizeof(update));

        if(info.ttl > 0){

          // send to all neighbours
          for(unsigned int i=0 ; i<sock_fd.size() ; i++){
            if(sock_fd[i] != 0  && i != prev_virtual_id){

          //cout<<"\tresending to "<<i<<"\n";
              if (send(sock_fd[i], buf2, MAXDATASIZE, 0) == -1){
            //perror("send");
              }

              update_timestamp();
            }
          }

        }
      }

      else if (info.message_update == true){
      // TODO: HANDLE BUG WHERE MESSAGE LIST SKIPS A 3
        //cout<<"recieved message update from "<<info.source<<" to "<<info.dest<<"\n";

        info.message_update = true;
        info.neighbour_update = false;

        info.hops[info.hops_pos] = virtual_id;
        info.hops_pos = info.hops_pos + 1;

        if(info.dest == virtual_id){

          cout<<"from "<<info.source<<" to "<<info.dest<<" hops ";
          for(int i=0 ; i<info.hops_pos ; i++){
            cout<<info.hops[i]<<" ";
          }
          cout<<"message "<<info.mess<<"\n";

        }

        else{

          int next_hop = get_next_hop(info);

        if(next_hop > 0){ // NOTE: next_hop will be -1 if node hasn't connected yet

          cout<<"from "<<info.source<<" to "<<info.dest<<" hops ";
        for(int i=0 ; i<info.hops_pos ; i++){
          cout<<info.hops[i]<<" ";
        }
        cout<<"message "<<info.mess<<"\n";

        char buf3[MAXDATASIZE];
        memcpy(buf3, &info, sizeof(update));

        if (send(sock_fd[next_hop], buf3, MAXDATASIZE, 0) == -1){
          perror("send");
        }
      }
    }
  }
}

else if (numbytes == 0) {
    /* socket has been closed */
  close(sockfd);
    //printf("closing socket\n");
}
}
}

int get_next_hop(update mess){
  int dest = mess.dest;
  
  for(unsigned int i=0 ; i<info_list.size() ; i++){
    PathInfo p = info_list[i];
    if(p.destination == dest){
      return p.path[1];
    }
  }

  return -1;
}

// handle updates to the routing table
void handle_routing_table_update(int x[MAX_NODE_COUNT][MAX_NODE_COUNT]){

  for(int i=0 ; i<MAX_NODE_COUNT ; i++){

    if(x[i][virtual_id] != top[i][virtual_id]){

        // we use the value INT_MAX to show links have been broken
      if(x[i][virtual_id] == -1){
        cout<<"no longer linked to node "<<i<<"\n";
        x[i][virtual_id] = 0;
      } 

      else {
        cout<<"now linked to node "<<i<<" with cost "<<x[virtual_id][i]<<"\n";
      }
    }

    for(int j=0 ; j<MAX_NODE_COUNT ; j++){
      top[i][j] = x[i][j];
    }
  }

  graph g(virtual_id);
  for(int i=0 ; i<MAX_NODE_COUNT ; i++){
    for(int j=0 ; j<MAX_NODE_COUNT ; j++){
      if(top[i][j] > 0 && top[i][j] != INT_MAX && top[i][j] != 6000){
        g.addLink(i, j, top[i][j]);
      }
    }
  }

  vector<PathInfo> temp = g.getPathInformation();

  if(info_list.size() != temp.size()){
    table_changed = true;
    return;
  }

  for(unsigned int i=0 ; i<info_list.size() ; i++){

    PathInfo path1 = info_list[i];
    PathInfo path2 = temp[i];

    if(path1.source != path2.source){
      table_changed = true;
      return;
    }

    if(path1.destination != path2.destination){
      table_changed = true;
      return;
    }

    if(path1.cost != path2.cost){
      table_changed = true;
      return;
    }

    if(path1.path.size() != path2.path.size()){
      table_changed = true;
      return;
    }

    for(unsigned int i=0 ; i<path1.path.size() ; i++){
      if(path1.path[i] != path2.path[i]){
        table_changed = true;
        return;
      }
    }

    //cout<<"set table_changed to "<<table_changed<<"\n";
  }
}