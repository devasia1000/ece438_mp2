all: manager linkstate distvec

manager: manager.cpp graph.cpp graph.h
	g++ -pthread -std=c++0x -Wall  manager.cpp graph.cpp -o manager

linkstate: linkstate.cpp graph.h graph.cpp
	g++ -pthread -std=c++0x -Wall linkstate.cpp graph.cpp -o linkstate

distvec: distvec.cpp graph.h graph.cpp
	g++ -pthread -std=c++0x -Wall distvec.cpp graph.cpp -o distvec

clean:
	rm -rf manager linkstate distvec
