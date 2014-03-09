#ifndef NODE
#define NODE
#define LINK 1001102302
#define DVECTOR 1002104003
#define PORT 4950

#include <string>
#include <iostream>
#include <vector>
#include <iterator>
#include <sstream>


class node{
private:
	int id;
	int port;
	int type;
	int cost;
	std::vector<node> neighbors;
	char* int_to_char(int i);
	void update_forwarding_table();
public:
	node();
	node(int id, int cost, int type);
	int get_id();
	int get_cost();
	std::string get_type_s();
	int get_type_i();
	int get_port_i();
	char* get_port_c();
	void add_neighbor(node n);
	void remove_neighbor(int node_id);
	std::vector<node> get_neighbors_v();
	std::string get_neighbors_s();
	std::string to_string();
};
node::node(){}
node::node(int id, int cost, int type){
	this->id = id;
	this->port = PORT + id;
	this->cost = cost;
	this->type = type;
}
int node::get_id(){
	return id;
}
int node::get_cost(){
	return cost;
}
std::string node::get_type_s(){
	if(type == LINK) return "LINK";
	if(type == DVECTOR) return "DVECTOR";
	return "?";
}
int node::get_type_i(){
	return type;
}
int node::get_port_i(){
	return port;
}
char* node::get_port_c(){
	return int_to_char(get_port_i());
}
void node::add_neighbor(node n){
	neighbors.push_back(n);
	std::cout << "now linked to node " << n.get_id() << " with cost " << n.get_cost() << "\n";
}
void node::remove_neighbor(int node_id){
	int index = -1;
	for(std::vector<node>::size_type i = 0; i != neighbors.size(); i++) {
    	if(node_id == neighbors[i].get_id())
    	{
    		index = i;
    	}
    	// std::cout << neighbors[i].to_string();
	}
	if(index >= 0){
		neighbors.erase(neighbors.begin() + index -1);
		std::cout << "no longer linked to node " << node_id << "\n";
	} 
}
std::vector<node> node::get_neighbors_v(){ 
	return neighbors;
}
std::string node::get_neighbors_s(){
	std::string res;
	for(std::vector<node>::iterator it = neighbors.begin(); it != neighbors.end(); ++it) {
    	node n = (node)*it;
    	// std::cout << n.to_string();
    	res.append(n.to_string());
	}
	return res;
}
void node::update_forwarding_table(){
	if(type == DVECTOR){

	}else if(type == LINK ){

	}
}
std::string node::to_string(){
	return "[node]\n"
    "\tthis->id: " + std::to_string(id) + "\n" 
    "\tthis->port: " + std::to_string(port) + "\n"
    "\tthis->cost: " + std::to_string(cost) + "\n" 
    "\tthis->type: " + get_type_s() + "\n" +
    "\tneighbors.size(): " + std::to_string(neighbors.size()) + "\n"
    "\tthis->neighbors: \n" + get_neighbors_s() + "\n";
}
char* node::int_to_char(int i) {
  std::stringstream ss;
  ss << i;
  std::string result = ss.str();
  char* buff = new char[result.size()+1];
  buff[result.size()] = 0;
  memcpy(buff, result.c_str(), result.size());
  return buff;
}
#endif