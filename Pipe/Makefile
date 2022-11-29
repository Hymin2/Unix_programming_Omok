CC = gcc
TARGET = pipe_server pipe_client

normar: $(TARGET)

pipe_server: pipe_server.c
	$(CC) -o pipe_server pipe_server.c -lpthread

pipe_client: pipe_client.c
	$(CC) -o pipe_client pipe_client.c -lpthread -lncurses

clean:
	$(RM) $(TARGET)
