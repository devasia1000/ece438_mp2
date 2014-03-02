#define MAX_NODE_COUNT 20

#include <vector>
#include <stdio.h>
#include <limits.h>
#include <iostream>
#include <stack>
using namespace std;

// struct to store path information
struct PathInfo{
    int source;
    int destination;
    int cost;
    vector<int> path;
};


class graph{

    private:

        // stores collection of PathInfo for each node in graph
        vector<PathInfo> path_info;

        // variables for djikstra's alg
        int parent[MAX_NODE_COUNT];
        int pathcost[MAX_NODE_COUNT];
        int source;


        int minDistance(int dist[], bool sptSet[]);
        int djikstra();
        int djikstra_helper();

    public:
        
        // stores topology information
        int top[MAX_NODE_COUNT][MAX_NODE_COUNT];

        graph(int src);
        void addLink(int node1, int node2, int cost);
        vector<PathInfo> getShortestPathInformation();


        /* 
           Calculates the:

           i) shortest path between node 1 and all nodes which
           which is stored in 'pathcost'
           ii) a list of nodes through which shortest path 
           exists which is stored in 'path'
         */

};
