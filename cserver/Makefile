all: propd

propd: main.o propd.o slist.o
	gcc -o $@ $^

main.o: main.c
	gcc -c $^ -o $@ -I../client

propd.o: propd.c
	gcc -c $^ -o $@ -I../client

slist.o: slist.c
	gcc -c $^ -o $@ -I../client

clean:
	rm propd *.o

