all: propd

propd: main.o propd.o
	gcc -o $@ $^ -lpthread

main.o: main.c
	gcc -c $^ -o $@ -I../client

propd.o: propd.c
	gcc -c $^ -o $@ -I../client

clean:
	rm propd *.o

