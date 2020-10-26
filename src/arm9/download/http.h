#ifndef _HTTP_H_
#define _HTTP_H_



#include <nds.h>
#include <dswifi9.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>

const int LLEN = 14; // strlen("Content-Length");



int http_send(char*);

char* http_recv(int, int*);
char* http_recv(int);






#endif /* _HTTP_H_ */
