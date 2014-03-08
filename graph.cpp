// IMPORTANT REFERENCES: http://www.geeksforgeeks.org/greedy-algorithms-set-6-dijkstras-shortest-path-algorithm/
//                       http://www.geeksforgeeks.org/greedy-algorithms-set-5-prims-minimum-spanning-tree-mst-2/

#include "graph.h"

// constructor sets adjagency matrix to 0
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
    if(cost <= 0){
        top[node1][node2] = 0;
        top[node2][node1] = 0;
        return;
    }

    top[node1][node2] = cost;
    top[node2][node1] = cost;

}

int graph::minDistance(int dist[], bool sptSet[]){

    // Initialize min value
    int min = INT_MAX;
    int min_index;

    for (int v = 0; v < MAX_NODE_COUNT; v++){
        if (sptSet[v] == false && dist[v] <= min){
            min = dist[v];
            min_index = v;
        }
    }

    return min_index;
}

// THIS FUNCTION IS FOR TESTING PURPOSES ONLY...
// I WILL IMPLEMENT DJIKSTRAS BY MYSELF AFTER I COMFIRM LINK STATE IS WORKING
int graph::djikstra(){

    int dist[MAX_NODE_COUNT];     // The output array.  dist[i] will hold the shortest
    // distance from src to i

    bool sptSet[MAX_NODE_COUNT]; // sptSet[i] will true if vertex i is included in shortest
    // path tree or shortest distance from src to i is finalized

    // Initialize all distances as INFINITE and stpSet[] as false
    for (int i = 0; i < MAX_NODE_COUNT; i++){
        dist[i] = INT_MAX;
        sptSet[i] = false;
    }

    // Distance of source vertex from itself is always 0
    dist[source] = 0;

    // Find shortest path for all vertices
    for (int count = 0; count < MAX_NODE_COUNT-1; count++){

        // Pick the minimum distance vertex from the set of vertices not
        // yet processed. u is always equal to src in first iteration.
        int u = minDistance(dist, sptSet);
        // Mark the picked vertex as processed
        sptSet[u] = true;

        // Update dist value of the adjacent vertices of the picked vertex.
        for (int v = 0; v < MAX_NODE_COUNT; v++){

            // Update dist[v] only if is not in sptSet, there is an edge from 
            // u to v, and total weight of path from src to  v through u is 
            // smaller than current value of dist[v]
            if (!sptSet[v] && top[u][v] && dist[u] != INT_MAX && dist[u]+top[u][v] < dist[v]){
                dist[v] = dist[u] + top[u][v];
                pathcost[v] = dist[v];
                parent[v] = u;
            }
        }
    }

    // print the constructed distance array
    djikstra_helper();

    return 1;
}

int graph::djikstra_helper(){
    
    path_info.clear();

    for(int i=0 ; i<MAX_NODE_COUNT ; i++){
        stack<int> temp;
        if(parent[i] != -1){
            
            PathInfo path;

            path.destination = i;
            path.cost = pathcost[i];
            path.source = source;

            int j = i;
            while(parent[j] != source){
                temp.push(parent[j]);
                j = parent[j];
            }

            while(!temp.empty()){
                path.path.push_back(temp.top());
                temp.pop();
            }

            path_info.push_back(path);
        }
    }

    return 0;
}

vector<PathInfo> graph::getShortestPathInformation(){
    djikstra();
    return path_info;
}

