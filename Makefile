all: manager linkstate distvec

manager: manager.cpp graph.cpp graph.h
	g++ -pthread -Wall -o manager manager.cpp graph.cpp graph.h

linkstate: linkstate.cpp graph.h
	g++ -pthread -Wall -o linkstate linkstate.cpp graph.cpp graph.h

distvec: distvec.cpp graph.h
	g++ -pthread -Wall -o distvec distvec.cpp graph.cpp graph.h  

clean:
	rm -rf manager linkstate distvec
