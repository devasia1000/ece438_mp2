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

    int dist[MAX_NODE_COUNT];

    bool sptSet[MAX_NODE_COUNT];

    for (int i = 0; i < MAX_NODE_COUNT; i++){
        dist[i] = INT_MAX;
        parent[i] = -1;
        sptSet[i] = false;
    }

    dist[source] = 0;

    for (int count = 0; count < MAX_NODE_COUNT-1; count++){

        int u = minDistance(dist, sptSet);
        sptSet[u] = true;

        for (int v = 0; v < MAX_NODE_COUNT; v++){

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

int graph::printSolution(int dist[]){

   printf("Vertex   Distance from Source\n");
   for (int i = 0; i < MAX_NODE_COUNT; i++)
      printf("%d \t\t %d\n", i, dist[i]);

  return 0;
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

