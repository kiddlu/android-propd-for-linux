all: propd

propd: main.o PropertyServer.o
	g++ -o $@ $^

main.o: main.cpp
	g++ -c $^ -o $@ -I../lib

PropertyServer.o: PropertyServer.cpp
	g++ -c $^ -o $@ -I../lib

clean:
	rm propd *.o

