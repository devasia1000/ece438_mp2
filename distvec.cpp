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

#include "lib/structs.h"
#include "lib/node.h"

#define BACKLOG 20   // how many pending connections queue will hold
#define MAXDATASIZE 2000
#define MANAGER_PORT "6000"
#define NODE_PORT "4950"



long timestamp = -1;
int top[MAX_NODE_COUNT][MAX_NODE_COUNT]; // stores topology information
int virtual_id = -1;
char manager_ip[80]; // ip address of the manager
vector<char*> ip_addresses; // hold ip addresses of neighbours
vector<int> sock_fd; // hold a sock fd corresponding to each neighbour
node n;

// START OF FUNCTION DEFINITIONS
void* contactManager(void *ptr);
void* convergence_checker(void *ptr);
void update_timestamp();
void *startServer(void *ptr);
void *get_in_addr(struct sockaddr *sa);
void handle_routing_table_update(int x[MAX_NODE_COUNT][MAX_NODE_COUNT]);
void clean();
char* convertToString(int number);
void sigchld_handler(int s);


int main(int argc, char **argv){
	if(argc != 2){
		cerr<<"Usage: ./distvec <manager_ip_address>\n";
		return 1;
	}
	strcpy(manager_ip, argv[1]);
	clean();
	
  	//cout<<__func__<<" : started manager thread\n";
	pthread_t manager_thread;
	pthread_create( &manager_thread, NULL, contactManager, NULL);

  	//cout<<__func__<<" : started convergence checker thread\n";
	pthread_t convergence;
	pthread_create(&convergence, NULL, convergence_checker, NULL);
	pthread_join(convergence, NULL);
}




// contacts the manager and gets virtual id
void *contactManager(void *ptr) {
	int sockfd, numbytes;
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
	string port = MANAGER_PORT;

	/* clear the hints structure */
	memset(&hints, 0, sizeof(hints));
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
	if (p == NULL) {exit(0);}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
  	//printf("client: connecting to %s\n", s);
	freeaddrinfo(servinfo);
	while(1) {
		/* read data from server */
		numbytes = recv(sockfd, buf, MAXDATASIZE, 0);
		if (numbytes == -1) {
  		/* error has occured with read */
			perror("recv");
			exit(0);
		}
		else if (numbytes > 0) {
			update_timestamp();
  			/* read is sucsessful */
			if(virtual_id == -1) {
        		// cout<<__func__<<" : going to assign virtual id\n";
				if(buf[1] == '\0') {
					virtual_id = buf[0];
				}
				else {
					virtual_id = 10*buf[0] + buf[1];
				}
    			virtual_id = virtual_id - 48; // account for conversion from ASCII to int
    			//cout<<__func__<<" assigned virtual id "<<virtual_id<<"\n";
    			n = node(virtual_id,0,DVECTOR);
    			// std::cout  << n.to_string();
    			pthread_t link_state_listener;
    			pthread_create(&link_state_listener, NULL, startServer, NULL);
    			//cout<<__func__<<" : started link state listener thread\n";
			}
			else {
				update info;
				memcpy(&info, buf, sizeof(update));
				for(int i=0 ; i<MAX_NODE_COUNT ; i++){
					if(ip_addresses[i][0] == '\0' && info.neighbours[i][0] != '\0' && i != virtual_id){
        				//cout<<"updated ip address for "<<i<<" : "<<ip_addresses[i]<<"\n";
						ip_addresses[i] = new char[20];
						strcpy(ip_addresses[i], info.neighbours[i]);
						// Open socket to neighbor
						//add_sockfd(i);
					}
				}
				// handle_routing_table_update(info.top);
    			//cout<<__func__<<" : recieved neighbour information from manager:\n";
    			// ENCODE INFORMATION INTO STRUCT
				char mess[MAXDATASIZE];
				data message;
				message.sender_id = virtual_id;
				message.ttl = 20;
				for(int i=0 ; i<MAX_NODE_COUNT ; i++){
					for(int j=0 ; j<MAX_NODE_COUNT ; j++){
						message.top[i][j] = top[i][j];
					}
				}
				sleep(2);
				memcpy(mess, &message, sizeof(data));
				for(int i=0 ; i<MAX_NODE_COUNT ; i++){
					if(top[virtual_id][i] > 0){
						if (send(sock_fd[i], mess, MAXDATASIZE, 0) == -1){
          					//perror("send");
						}
					}
				}
    			//cout<<__func__<<" : sent routing information to neighbour\n";
				update_timestamp();
			}
		}
		else if (numbytes == 0) {
      		/* socket has been closed */
			close(sockfd);
  			//printf("closing socket\n");
		}
	}
}//END contactManager

void *convergence_checker(void *ptr){
	while(1){
		sleep(5);
		long cur_time = time(0);
		if(cur_time - timestamp > 5 && timestamp != 0){
      //cout<<__func__<<" : Convergence Detected! Printing out routing table...\n";
			graph g(virtual_id);
			for(int i=0 ; i<MAX_NODE_COUNT ; i++){
				for(int j=0 ; j<MAX_NODE_COUNT ; j++){
					if(top[i][j] > 0 && top[i][j] != INT_MAX && top[i][j] != 6000){
						g.addLink(i, j, top[i][j]);
					}
				}
			}
			vector<PathInfo> info_list = g.getPathInformation();
			for(unsigned int i=0 ; i<info_list.size() ; i++){
				PathInfo path = info_list[i];
				cout<<path.destination<<" "<<path.cost<<": "<<path.source<<" ";
				for(unsigned int j=0 ; j<path.path.size() ; j++){
					cout<<path.path[j]<<" ";
				}
				cout<<path.destination<<"\n";
			}
			timestamp = 0;
		}
	}
	exit(0);
}// END convergence_checker

// listens for incoming connections from neighbours
void *startServer(void *ptr) {
	class node n(virtual_id,0,DVECTOR);
	
	int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXDATASIZE];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, NODE_PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
       exit(1);
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        
        if (::bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        exit(1);
    }

    freeaddrinfo(servinfo);

    printf("listener: waiting to recvfrom...\n");

    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd, buf, MAXDATASIZE-1 , 0,
        (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }

    printf("listener: got packet from %s\n",
        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s));
    printf("listener: packet is %d bytes long\n", numbytes);
    buf[numbytes] = '\0';
    printf("listener: packet contains \"%s\"\n", buf);

    close(sockfd);

    return NULL;

	// char server_port[80];
	// string po = PORT;
	// strcpy(server_port, po.c_str());
	// if(virtual_id < 10){
	// 	server_port[1] = '\0';
	// } else if (virtual_id > 10){
	// 	server_port[2] = '\0';
	// }

	// int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	// struct addrinfo hints, *servinfo, *p;
	// struct sockaddr_storage their_addr; // connector's address information
	// socklen_t sin_size;
	// struct sigaction sa;
	// int yes = 1;
	// char s[INET6_ADDRSTRLEN];
	// int rv;

	// memset(&hints, 0, sizeof hints);
	// hints.ai_family = AF_UNSPEC;
	// hints.ai_socktype = SOCK_STREAM;
	// hints.ai_flags = AI_PASSIVE; // use my IP
	// int port = atoi(PORT) + virtual_id;

	// if ((rv = getaddrinfo(NULL, convertToString(port), &hints, &servinfo)) != 0) {
	//   	// fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	// 	exit(1);
	// }
	// // loop through all the results and bind to the first we can
	// for(p = servinfo; p != NULL; p = p->ai_next) {
	// 	if ((sockfd = socket(p->ai_family, p->ai_socktype,
	// 		p->ai_protocol)) == -1) {
	// 		perror("server: socket");
	// 		continue;
	// 	}
	// 	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
	// 		sizeof(int)) == -1) {
	// 		perror("setsockopt");
	// 		exit(1);
	// 	}
	// 	// if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
	// 	//   close(sockfd);
	// 	//   perror("server: bind");
	// 	//   continue;
	// 	// }
	// 		break;
	// }

	// if (p == NULL)  {
	//  	// fprintf(stderr, "server: failed to bind\n");
	// 	exit(1);
	// }
	// freeaddrinfo(servinfo); // all done with this structure
	// if (listen(sockfd, BACKLOG) == -1) {
	// 	perror("listen");
	// 	exit(1);
	// }
	// sa.sa_handler = sigchld_handler; // reap all dead processes
	// sigemptyset(&sa.sa_mask);
	// sa.sa_flags = SA_RESTART;
	// if (sigaction(SIGCHLD, &sa, NULL) == -1) {
	// 	perror("sigaction");
	// 	exit(1);
	// }
	// // cout<<"Neighbour server: waiting for connections on port "<<convertToString(port)<<"\n";
	// while(1) {  // main accept() loop
	// 	sin_size = sizeof their_addr;
	// 	new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
	// 	if (new_fd == -1) {
	// 		perror("accept");
	// 		continue;
	// 	}
	// 	inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
	//   	// printf("Neighbour server: got connection from %s\n", s);
	//   	// create new thread to handle client
	// 	pthread_t client_manager;
	// 	// pthread_create( &client_manager, NULL, handle_client, &new_fd);
	// }
}

// handle updates to the routing table
void handle_routing_table_update(int x[MAX_NODE_COUNT][MAX_NODE_COUNT]){
	if(timestamp <= time(0)){
    	for(int i=1 ; i<17 ; i++){
		    if(x[virtual_id][i] != top[virtual_id][i] && x[virtual_id][i] != 0){
	        // we use the value INT_MAX to show links have been broken
		        if(x[virtual_id][i] == INT_MAX){
		          cout<<"no longer linked to node "<<i<<"\n";
		        } 
		        else {
		          cout<<"now linked to node "<<i<<" with cost "<<x[virtual_id][i]<<"\n";
		        }
      		}
    	}
	    for(int i=0 ; i<MAX_NODE_COUNT ; i++){
	      for(int j=0 ; j<MAX_NODE_COUNT ; j++){
	        if(x[i][j] > 0){
	          top[i][j] = x[i][j];
	        }
	      }
	    }
    	update_timestamp();
  	}
}

void update_timestamp(){
	timestamp = (long)time(0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
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

void sigchld_handler(int s) {
  while(waitpid(-1, NULL, WNOHANG) > 0);
}


void clean(){
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
}


