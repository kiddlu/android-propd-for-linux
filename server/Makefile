all: propd

propd: main.o libpropd.a
	gcc -o $@ $^ -L. -lpropd -lstdc++

main.o: main.c
	gcc -c $^ -o $@ -I../client

libpropd.a: propd.o PropertyServer.o
	ar -r $@ $^

propd.o: propd.cpp
	g++ -c $^ -o $@ -I../client

PropertyServer.o: PropertyServer.cpp
	g++ -c $^ -o $@ -I../client

clean:
	rm propd *.o *.a

