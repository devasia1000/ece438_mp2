#ifndef NODE_MESSAGE
#define NODE_MESSAGE

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <regex>
#include <sstream> 
#include <vector>
#include <iterator>



class message {
private:
    int from_node;
    int to_node;
    std::string msg;
public:
    message();
    message(std::string input);
    int get_from_node();
    int get_to_node();
    std::string get_msg();
    std::string to_string();
};
message::message(){}
message::message(std::string input){
    std::smatch m;
    std::regex reg("[a-zA-z]");
    // Find position of first alphabetic letter with 
    // m.position()
    if(std::regex_search(input,m,reg)){
        // create sub string up to first alpabetic letter
        std::string nodes =  input.substr(0,m.position());
        // Split nodes into seperate strings and
        // store them in a vector
        // e.g. split '2  1' into '2' and '1'  
        std::istringstream buf(nodes);
        std::istream_iterator<std::string> beg(buf), end;
        std::vector<std::string> tokens(beg, end); 
        // Convert string to integer
        if (!(std::istringstream(tokens.at(0)) >> this->from_node)) this->from_node = -1;
        if (!(std::istringstream(tokens.at(1)) >> this->to_node)) this->to_node = -1;
        // Store 2nd half of string as the msg
        this->msg = input.substr(m.position());
    }
    
}
// Getters
int message::get_from_node(){
    return from_node;
}
int message::get_to_node(){
    return to_node;
}
std::string message::get_msg(){
    return msg;
}
std::string message::to_string(){
    return "[message]\n"
        "\tthis->from_node: " + std::to_string(this->from_node) + "\n" 
        "\tthis->to_node: " + std::to_string(this->to_node) + "\n" 
        "\tthis->msg: " + this->msg + "\n";
}
#endif