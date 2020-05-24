CC=gcc
LIBPATH=lib
SRCPATH=src
OBJPATH=obj
CFLAGS=-I ./include
CFLAGS+=-I ./lib
LIB=$(LIBPATH)/libsocket.o $(LIBPATH)/log.o $(LIBPATH)/str.o
CFLAGS+=-g
OBJSERV=$(OBJPATH)/ftp.o $(OBJPATH)/ftps.o 
OBJSCLI=$(OBJPATH)/ftp.o $(OBJPATH)/ftpc.o 

all: objects server client

server:
	$(CC) server.c -o server $(OBJSERV) $(LIB) $(CFLAGS)

client:
	$(CC) client.c -o client $(OBJSCLI) $(LIB) $(CFLAGS)

objects: ftp ftps ftpc

ftps:
	$(CC) -c -o $(OBJPATH)/ftps.o $(SRCPATH)/ftps.c $(CFLAGS)

ftpc:
	$(CC) -c -o $(OBJPATH)/ftpc.o $(SRCPATH)/ftpc.c $(CFLAGS)

ftp:
	$(CC) -c -o $(OBJPATH)/ftp.o $(SRCPATH)/ftp.c $(CFLAGS)

clean:
	rm $(OBJPATH)/*.o
	rm server
	rm client
