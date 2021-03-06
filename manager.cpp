    #include "graph.h"
    #include "message.h"

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
    #include <fstream>

    #define BACKLOG 20   // how many pending connections queue will hold
    #define MAXDATASIZE 3000
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

    // GLOBAL VARIBALES 
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

    vector<int> sockfd_array; // holds socket file descriptors to each node
    vector<char*> ip_address_array; // holds ip addresses of nodes
    set<int> nodes; // used to store and assign virtual id's to nodes
    int top[MAX_NODE_COUNT][MAX_NODE_COUNT]; // used to hold topology information
    vector<message> message_list; // used to hold messages to be sent between nodes
    vector<bool> connected_nodes; // keep track of nodes that are already connected
    // END OF GLOBAL VARIABLES

    // START FUNCTION DECLARATIONS
    void *update_client(void *ptr);
    update prepare_send(int virtual_id);
    void *stdin_reader(void *ptr);
    // END FUNCTION DECLARATIONS

    int main(int argc, char **argv){

        if(argc != 3){
            cerr<<"Usage: ./manager <topology_file> <message_file>\n";
            return 1;
        }

        sockfd_array.resize(MAX_NODE_COUNT);
        ip_address_array.resize(MAX_NODE_COUNT);
        message_list.resize(MAX_NODE_COUNT);
        connected_nodes.resize(MAX_NODE_COUNT);

    // clear all data
        for(int i=0 ; i<MAX_NODE_COUNT ; i++){

            ip_address_array[i] = NULL;
            sockfd_array[i] = 0;
            connected_nodes[i] = 0;

            for(int j=0 ; j<MAX_NODE_COUNT ; j++){
                top[i][j] = 0;
            }
        }

        FILE *file = fopen(argv[1], "r");
        if(file == NULL){
            cerr<<strerror(errno)<<"\n";
            exit(0);
        }

        int num1, num2, num3;
        while(fscanf(file, "%d", &num1) != EOF && fscanf(file, "%d", &num2) != EOF && fscanf(file, "%d", &num3) != EOF){
            top[num1][num2] = num3;
            top[num2][num1] = num3;
            nodes.insert(num1);
            nodes.insert(num2);
        }

        fclose(file);
        
        // START Read message file
        ifstream input(argv[2]);
        if(input.fail()){
          cerr<<strerror(errno)<<"\n";
          exit(0);
      }

      string line;
      while( std::getline( input, line ) ) {
        class message mess(line);
        message_list.push_back(mess);
    }

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

    pthread_t stdin_listen;
    pthread_create( &stdin_listen, NULL, stdin_reader, NULL);

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
        connected_nodes[virtual_id] = 1;

        char *c = new char[50];
        strcpy(c, s);
        ip_address_array[virtual_id] = c;

        //cout<<"Stored IP address: "<<ip_address_array[virtual_id]<<"\n";

        sockfd_array[virtual_id] = new_fd;

        //cout<<"Sending virtual id: "<<virtual_id<<"\n";
        char *id = convertToString(virtual_id);
        if(virtual_id < 10){
            id[1] = '\0';
        } else{
            id[2] = '\0';
        }

        // assign virtual id to client
        if (send(sockfd_array[virtual_id], id, MAXDATASIZE, 0) == -1){
            perror("send");
        }

        // TODO: send message information to client

        for(unsigned int i=0 ; i<message_list.size() ; i++){
            message mess = message_list[i];

            if(mess.get_from_node() == virtual_id){
                update info;
                info.neighbour_update = false;
                info.message_update = true;
                info.hops_pos = 0;

                info.source = mess.get_from_node();
                info.dest = mess.get_to_node();

                strcpy(info.mess, mess.get_msg().c_str());

                char buf[MAXDATASIZE];
                memcpy(buf, &info, sizeof(update));

                cout<<"message to "<<virtual_id<<"\n";
                cout<<"\tsource: "<<info.source<<"\n";
                cout<<"\tdest: "<<info.dest<<"\n";
                cout<<"\tmessage: "<<info.mess<<"\n";

                if (send(sockfd_array[virtual_id], buf, MAXDATASIZE, 0) == -1){
                    perror("send");
                }
            }
        }

        // send topology and neighbour information to all clients
    for(int i=0 ; i<MAX_NODE_COUNT ; i++){
        if(sockfd_array[i] != 0){
            int *temp = new int;
                *temp = i; // IMPORTANT: used to store i and protect against change while for loop is running

                pthread_t update_thread;
                pthread_create( &update_thread, NULL, update_client, temp);
            }
        }
    }

    return 0;
}

// read from stdin and update appropiate client
void *stdin_reader(void *ptr){
    sleep(2);

    while(true){

        cout<<"Enter a topology update:\n";

        int node_id, dest_node, link_cost;
        cin>>node_id>>dest_node>>link_cost;

        top[node_id][dest_node] = link_cost;
        top[dest_node][node_id] = link_cost;

        // send topology and neighbour information to all clients
        for(int i=0 ; i<MAX_NODE_COUNT ; i++){
            if(sockfd_array[i] != 0){
                int *temp = new int;
                *temp = i; // IMPORTANT: used to store i and protect against change while for loop is running

                pthread_t update_thread;
                pthread_create( &update_thread, NULL, update_client, temp);
            }
        }
    }

    return NULL;
}

void *update_client(void *ptr){

    int *temp = (int*) ptr;
    int virtual_id = *temp;

    // send neighbour update
    update info;
    info.neighbour_update = true;
    info.message_update = false;

    for(int i=0 ; i<MAX_NODE_COUNT ; i++){
        info.neighbours[i][0] = '\0';
    }

    for(int i=0 ; i<MAX_NODE_COUNT ; i++){
        for(int j=0 ; j<MAX_NODE_COUNT ; j++){
            info.top[i][j] = 0;
        }
    }

    for(int i=0 ; i<MAX_NODE_COUNT ; i++){

        for(int j=0 ; j<MAX_NODE_COUNT ; j++){

            if(connected_nodes[i] > 0 && connected_nodes[j] > 0){
                info.top[i][j] = top[i][j];
                info.top[j][i] = top[j][i];
            }
        }

        if(ip_address_array[i] != NULL){
            strcpy(info.neighbours[i], ip_address_array[i]);
            //cout<<"stored "<<info.neighbours[i]<<" for "<<i<<"when sending to "<<virtual_id<<"\n";
        }
    }

    char buf[MAXDATASIZE];
    //memcpy(info.top, top, sizeof (int) * MAX_NODE_COUNT * MAX_NODE_COUNT);

    memcpy(buf, &info, sizeof(update));

    if (send(sockfd_array[virtual_id], buf, MAXDATASIZE, 0) == -1){
        perror("send");
    }

    return NULL;
}
