#include "graph.h"

graph::graph(int src){

    source = src;

    for(int i=0 ; i<MAX_NODE_COUNT ; i++){
        for(int j=0 ; j<MAX_NODE_COUNT ; j++){
            top[i][j] = 0;
            parent[i] = -1;
        }
    }
}

void graph::addLink(int node1, int node2, int cost){
    // IMPORTANT NOTE: TO BREAK LINES BETWEEN NODES, SEND A NEGATIVE EDGE WEIGHT!
    if(cost <= 0 || cost == INT_MAX){
        top[node1][node2] = 0;
        top[node2][node1] = 0;

    }

    else {
        top[node1][node2] = cost;
        top[node2][node1] = cost;
        //cout<<"link added between "<<node1<<" and "<<node2<<"\n";
    }
}

int graph::djikstra(){

    int dist[MAX_NODE_COUNT];
    bool sptSet[MAX_NODE_COUNT];

    for (int i = 0; i < MAX_NODE_COUNT; i++){ // clear all arrays

        if(i == source){
            dist[i] = 0;
        } else{
            dist[i] = INT_MAX;
        }

        parent[i] = -1;
        sptSet[i] = false;
    }

    for (int i = 0; i < MAX_NODE_COUNT-1; i++){

        int min = INT_MAX;
        int min_index;

        for (int i = 0; i < MAX_NODE_COUNT; i++){
            if (sptSet[i] == false && dist[i] <= min){
                min_index = i;
                min = dist[i];
            }
        }

        sptSet[min_index] = true;

        for (int j = 0; j < MAX_NODE_COUNT; j++){

            if (!sptSet[j] && top[min_index][j] && dist[min_index] != INT_MAX && dist[min_index]+top[min_index][j] < dist[j]){
                dist[j] = dist[min_index] + top[min_index][j];
                parent[j] = min_index;
                pathcost[j] = dist[j];
            }
        }
    }

    djikstra_helper();

    return 1;
}

int graph::djikstra_helper(){
    
    path_info.clear();

    for(int i=0 ; i<MAX_NODE_COUNT ; i++){

        if(parent[i] != -1){

            PathInfo path;
            path.path.clear();

            path.destination = i;
            path.cost = pathcost[i];
            path.source = source;

            int j = i;

            stack<int> temp;
            temp.push(j);

            while(parent[j] != source){
                temp.push(parent[j]);
                j = parent[j];
            }
            temp.push(source);

            while(!temp.empty()){
                int x = temp.top();
                path.path.push_back(x);
                temp.pop();
            }

            path_info.push_back(path);
        }

        else if(i == source){
            PathInfo p;
            p.path.clear();

            p.destination = source;
            p.cost = 0;
            p.source = source;

            p.path.push_back(source);

            path_info.push_back(p);
        }
    }

    return 0;
}

vector<PathInfo> graph::getPathInformation(){
    djikstra();

    return path_info;
}

