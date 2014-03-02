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

#define BACKLOG 20   // how many pending connections queue will hold
#define PORT "6000"

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

// used to hold client information
vector<int> sockfd_array;
vector<char*> ip_address_array;
vector<char*> port_array;

// list of available nodes
set<int> nodes;

int x=1;

// used to hold messages to be sent between nodes
vector<char*> message;

int main(){

    sockfd_array.resize(MAX_NODE_COUNT);
    ip_address_array.resize(MAX_NODE_COUNT);
    port_array.resize(MAX_NODE_COUNT);
    message.resize(40);


    char top_file[80];
    cout<<"Enter path to topology file: ";
    cin>>top_file;

    FILE *file = fopen(top_file, "r");
    if(file == NULL){
        cerr<<strerror(errno)<<"\n";
        exit(0);
    }

    graph g(1); // source node of graph is meaningless here
    int num1, num2, num3;
    while(fscanf(file, "%d", &num1) != EOF && fscanf(file, "%d", &num2) != EOF && fscanf(file, "%d", &num3) != EOF){
        g.addLink(num1, num2, num3);
        nodes.insert(num1);
        nodes.insert(num2);
    }

    fclose(file);

    char message_file[80];
    cout<<"Enter path to message file: ";
    cin>>message_file;

    file = fopen(message_file, "r");
    if(file == NULL){
        cerr<<strerror(errno)<<"\n";
        exit(0);
    }

    int num5, num6;
    char mess[80];
    while(fscanf(file, "%d", &num5) != EOF && fscanf(file, "%d", &num6) != EOF && fscanf(file, "%s", mess) != EOF){
        message[num5] = mess;
    }

    //  GENERAL ALG:
    // START ACCEPTING CONNECTIONS FROM NODES
    // AFTER ACCEPTING CONNECTION, ASSIGN EACH NODE A VIRTUAL ID
    // SEND MESSAGE DATA TO NODES
    // SEND NEIGHBOUR DATA AND COST TO EACH NODE
    // WAIT FOR CONVERGENCE
    // ACCEPT INPUT FROM STDIN
    // REPEAT

    /****************************** START ACCEPTING CONNECTIONS FROM NODES **************************/

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
        return 1;
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
        return 2;
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

        inet_ntop(their_addr.ss_family,
                get_in_addr((struct sockaddr *)&their_addr),
                s, sizeof s);
        printf("server: got connection from %s\n", s);
        
        set<int>::iterator it = nodes.begin();
        int virtual_id = *it;
        nodes.erase(virtual_id);

        char *c = new char[50];
        strcpy(c, s);
        ip_address_array[virtual_id] = c;

        cout<<"Stored IP address: "<<ip_address_array[virtual_id]<<"\n";

        // TAKE NOTE OF CLIENT IP ADDRESS AND PORT NUMBER
        // ASSIGN VIRTUAL NODE ID TO CLIENT

        if (!fork()) { // this is the child process
            
            // SEND RELEVANT TOPOLOGY INFORMATION
                // above will be accomplished by sending adjaceny matrix and blocking out non-neighbour information
            // UPDATE CONVERGENCE INFORMATION

            cout<<"Sending virtual id: "<<virtual_id<<"\n";

            char* id = convertToString(virtual_id);
            int size;
            if(virtual_id < 10){
                id[1] = '\0';
                size = 2;
            } else{
                id[2] = '\0';
                size = 3;
            }

            if (send(new_fd, id, size, 0) == -1){
                perror("send");
            }

            vector<char> top_info;

            for(int i=0 ; i<MAX_NODE_COUNT ; i++){
                for(int j=0 ; j<MAX_NODE_COUNT ; j++){
                    if(i == virtual_id || j == virtual_id){
                        top_info.push_back(g.top[i][j]+48); // factor of 48 converts int to ASCII
                        cout<<*top_info.end()<<" ";
                    } else {
                        top_info.push_back(0+48); // factor of 48 converts int to ASCII
                        cout<<*top_info.end()<<" ";
                    }
                }
            }

            char final_top_info[MAX_NODE_COUNT*MAX_NODE_COUNT];
            copy(top_info.begin(), top_info.end(), final_top_info);

            if (send(new_fd, final_top_info, sizeof(final_top_info)/sizeof(*final_top_info), 0) == -1){
                perror("send");
            }

            exit(0);
        }

        //  this is the server process

        // WAIT FOR CONVERGENCE
    }

    return 0;
}

void testGraph(){
    graph g(2);

    g.addLink(1, 2, 8);
    g.addLink(2, 3, 3);
    g.addLink(2, 5, 4);
    g.addLink(5, 4, 1);
    g.addLink(4, 1, 1);

    vector<PathInfo> path_info = g.getShortestPathInformation();
    for(unsigned int i=0 ; i<path_info.size() ; i++){
        cout<<"Source: "<<path_info[i].source<<", ";
        cout<<"Dest: "<<path_info[i].destination<<", ";
        cout<<"Cost: "<<path_info[i].cost<<", ";

        cout<<"Path: ";
        for(unsigned int j=0 ; j<path_info[i].path.size() ; j++){
            cout<<path_info[i].path[j]<<" ";
        }
        cout<<"\n";
    }
}
