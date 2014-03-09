#include "../lib/node.h"
#include <string>
#include <iostream>

int main(){
        class node n(1,0,DVECTOR);
        class node n1(2,8,DVECTOR);
        n.add_neighbor(n1);
        n.remove_neighbor(2);
        std::cout << n.to_string();
}
