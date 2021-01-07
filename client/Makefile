all: setprop getprop service

setprop: setprop.o properties.o
	gcc -o $@ $^ -lpthread

getprop: getprop.o properties.o
	gcc -o $@ $^ -lpthread

service: service.o properties.o
	gcc -o $@ $^ -lpthread

properties.o: properties.c
	gcc -c $^ -o $@

getprop.o: getprop.c
	gcc -c $^ -o $@

setprop.o: setprop.c
	gcc -c $^ -o $@
	
service.o: service.c
	gcc -c $^ -o $@

clean:
	rm setprop getprop service *.o

