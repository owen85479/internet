all:
	gcc server_v1.c -o server_v1 -lpthread
	gcc client_v1.c -o client_v1 -lpthread
	gcc server_v1.c client_v1.c -c
clean:
	rm server_v1 client_v1 client_v1.o server_v1.o
