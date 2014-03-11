#include "graph.h"

// constructor sets adjagency matrix to 0
graph::graph(){
    for(int i=0 ; i<MAX_NODE_COUNT ; i++){
        for(int j=0 ; j<MAX_NODE_COUNT ; j++){
            top[i][j] = 0;
        }
    }
}

void graph::addLink(int node1, int node2, int cost){
    if(cost <= 0){
        top[node1][node2] = 0;
        top[node2][node1] = 0;
        return;
    }

    top[node1][node2] = cost;
    top[node2][node1] = cost;
}

int graph::getShortestPath(int node1, int node2){
    if(top[node1][node2] == 0){
        return -1;
    }

    return getShortestPathHelper(node1, node2, 0);
}

int getShortestPathHelper(int node1, int node2, int sum){
    
    return 1;
}
