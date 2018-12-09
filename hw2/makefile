all:thread_server.o thread_client.o
	gcc thread_server.o -lpthread -o server
	gcc thread_client.o -lpthread -o client

thread_server.o:thread_server.c
	gcc -c thread_server.c -lpthread

thread_client.o:thread_client.c
	gcc -c thread_client.c -lpthread

clean:
	rm *.o

