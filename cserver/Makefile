all: propd

propd: main.o propd.o list.o
	gcc -o $@ $^

main.o: main.c
	gcc -c $^ -o $@ -I../client

propd.o: propd.c
	gcc -c $^ -o $@ -I../client

list.o: list.c
	gcc -c $^ -o $@ -I../client
	
clean:
	rm propd *.o

