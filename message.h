#ifndef NODE_MESSAGE
#define NODE_MESSAGE

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <sstream> 
#include <vector>
#include <iterator>
#include <regex.h> 


class message {
private:
    int from_node;
    int to_node;
    std::string msg;
    int find_first_alpha_index(std::string str);
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
    // Find position of first alphabetic letter 
    int index = find_first_alpha_index(input);
    if(index != -1){
        // create sub string up to first alpabetic letter
        std::string nodes =  input.substr(0,index);
        // Split nodes into seperate strings and
        // store them in a vector
        // e.g. split '2  1' into '2' and '1'  
        std::istringstream buf(nodes);
        std::istream_iterator<std::string> beg(buf), end;
        std::vector<std::string> tokens(beg, end); 
        // Convert string to integer
        if (!(std::istringstream(tokens.at(0)) >> this->from_node)){
          this->from_node = -1;  
      } 
      if (!(std::istringstream(tokens.at(1)) >> this->to_node)){
          this->to_node = -1;  
      } 
        // Store 2nd half of string as the msg
      this->msg = input.substr(index);
  }
  
}
int message::find_first_alpha_index(std::string str){
    const char* input = str.c_str();
    regex_t regex;
    regmatch_t pmatch[10];
    if(regcomp(&regex, "[a-zA-Z]", REG_EXTENDED)){
        std::cout << "regex error";
        exit(0);
    }
    if(regexec(&regex, input, 10, pmatch, REG_NOTBOL) != 0){
        std::cout << "NO MATCH\n";
    }
    char result[100];
    for(int i = 0; pmatch[i].rm_so != -1; i++){
        int length = pmatch[i].rm_eo - pmatch[i].rm_so;
        memcpy(result, input + pmatch[i].rm_so, length);
        result[length] = 0;
        return str.find(result);
    }
    return -1;
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
