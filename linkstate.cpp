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

  void *contactManager(void *ptr);

    void sigchld_handler(int s){

      while(waitpid(-1, NULL, WNOHANG) > 0);
    }

        // get sockaddr, IPv4 or IPv6:
    void *get_in_addr(struct sockaddr *sa){

      if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
      }

      return &(((struct sockaddr_in6*)sa)->sin6_addr);
    }

    char* convertToString(int number){

           stringstream ss; //create a stringstream
           ss << number; //add number to the stream
           string result = ss.str();

           char *a=new char[result.size()+1];
           a[result.size()]=0;
           memcpy(a, result.c_str(), result.size());

           return a;
         }

        int top[MAX_NODE_COUNT][MAX_NODE_COUNT]; // stores topology information
        int virtual_id = -1;

        string manager_ip = "localhost"; // ip address of the manager...WILL NEED TO PASS IN AS ARG LATER
        string manager_port = "6000"; // port number of the manager...WILL NEED TO PASS IN AS ARG LATER

        int main(){
            pthread_t thread1;
            pthread_create( &thread1, NULL, contactManager, NULL);
            pthread_join( thread1, NULL);
        }

        // contacts the manager and gets virtual id
        void *contactManager(void *ptr){
        	
          int sockfd, numbytes;
          char buf[MAXDATASIZE];
          struct addrinfo hints, *servinfo, *p;
          int rv;
          char s[INET6_ADDRSTRLEN];

          //char input[MAXDATASIZE];

          /* clear the hints structure */
          memset(&hints, 0, sizeof hints);
          hints.ai_family = AF_UNSPEC;
          hints.ai_socktype = SOCK_STREAM;

          if ((rv = getaddrinfo(manager_ip.c_str(), manager_port.c_str(), &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            exit(0);
          }

            /* loop through all the results and connect to the first we can */
          for(p = servinfo; p != NULL; p = p->ai_next) {
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

          while(1){

                /* read data from server */
            numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0);

            if (numbytes == -1) { /* error has occured with read */
            perror("recv");
            exit(0);
          }

            else if (numbytes > 0){ /* read is sucsessful */
          if(virtual_id == -1){
            if(buf[1] == '\0'){
              virtual_id = buf[0];
            } else {
              virtual_id = 10*buf[0] + buf[1];
            }
          }

          printf("virtual_id: %d", virtual_id);
        }

            else if (numbytes == 0){ /* socket has been closed */
        close(sockfd);
        printf("closing socket\n");
      }
    }
    }

        // listens for incoming connections from neighbours
    void startServer(void *ptr){

    }

