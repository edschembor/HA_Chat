CC=gcc
LD=gcc
CFLAGS=-g -Wall -std=c99
CPPFLAGS=-I. -I/home/eschemb1/distributed/final/include
SP_LIBRARY_DIR=/home/eschemb1/distributed/final

all: chat_server client

client: $(SP_LIBRARY_DIR)/libspread-core.a client.o
	$(LD) -o $@ client.o $(SP_LIBRARY_DIR)/libspread-core.a -ldl -lm -lrt -lnsl $(SP_LIBRARY_DIR)/libspread-util.a

chat_server:  $(SP_LIBRARY_DIR)/libspread-core.a chat_server.o
	$(LD) -o $@ chat_server.o $(SP_LIBRARY_DIR)/libspread-core.a -ldl -lm -lrt -lnsl $(SP_LIBRARY_DIR)/libspread-util.a

clean:
	rm -f *.o chat_server client

