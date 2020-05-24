#ifndef __LIBSOCKET__H
#define __LIBSOCKET__H
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <time.h>
#include <ctype.h>

//#define struct sockaddr SA
#define LISTENQ		8
#define SERVER_IP 	"127.0.0.1"
#define SERVER_PORT 	3333

int Socket(int family, int proto, int flags);
char *Date(char *s);
int Bind(int sockfd,const struct sockaddr *addr , socklen_t addrlen);
int Connect(int sockfd , const struct sockaddr *addr , socklen_t addrlen);
int Listen(int sockfd , int backlog);
int Accept(int sockfd, struct sockaddr *addr , socklen_t *addrlen);
int Inet_pton(int af , const char *src , void  *dest);
const char *Inet_ntop(int af , const void *src , char *dst , socklen_t size);
void str_echo(int listenfd);
void str_cli(FILE *fp, int sockfd);
char *Socket_ntop(struct sockaddr_in *addr, socklen_t *addrlen);
int Tcp_Connect(const char *serverIp , unsigned short port);
int Tcp_Listen(const char *ip , unsigned short port);
void progress_bar(char *base , unsigned long total , unsigned long done , unsigned short size);

#endif
