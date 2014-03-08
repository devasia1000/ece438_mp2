all: manager linkstate distvec

manager: manager.cpp graph.cpp graph.h
	g++ -pthread -std=c++0x -g -Wall  manager.cpp graph.cpp graph.h -o manager

linkstate: linkstate.cpp graph.h graph.cpp
	g++ -pthread -std=c++0x -g -Wall linkstate.cpp graph.cpp graph.h -o linkstate

distvec: distvec.cpp graph.h graph.cpp
	g++ -pthread -std=c++0x -g -Wall distvec.cpp graph.cpp graph.h -o distvec

clean:
	rm -rf manager linkstate distvec
