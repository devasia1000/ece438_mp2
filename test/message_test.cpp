#include "../lib/message.h"
#include <string>

int main(){
        class message msg("2 1 here is a message from 2 to 1");
            std::cout << msg.to_string();
}
