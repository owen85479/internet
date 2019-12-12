all:
	gcc server.c -o server -lpthread
	gcc client.c -o client -lpthread
	gcc server.c client.c -c
clean:
	rm server client client.o server.o
