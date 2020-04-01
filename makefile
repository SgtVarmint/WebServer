server: server.c
	gcc -ggdb server.c -o server

clean:
	rm -f server
