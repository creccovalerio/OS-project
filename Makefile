all:
	gcc -o server server.c helper.c -lpthread
	gcc -o client tcpClient.c helper.c -lpthread

clean:
	rm -f *.o
	rm -f $(CLIENT)
	rm -f $(SERVER)