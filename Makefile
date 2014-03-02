all: manager linkstate distvec

manager: manager.c graph.c graph.h
	g++ -Wall -o manager manager.c graph.c graph.h

linkstate: linkstate.c graph.c graph.h
	g++ -Wall -o linkstate linkstate.c graph.c graph.h

distvec: distvec.c graph.c graph.h
	g++ -Wall -o distvec distvec.c graph.c graph.h  

clean:
	rm -rf manager linkstate distvec
