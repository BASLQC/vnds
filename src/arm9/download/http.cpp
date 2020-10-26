#include "http.h"

int http_send(char* file)
{
    char request[256];
    sprintf(request, "GET %s HTTP/1.1\r\n"
                     "Host: www.digital-haze.net\r\n"
                     "User-Agent: Nintendo DS\r\n\r\n", file);

    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in sain;
    sain.sin_family = AF_INET;
    sain.sin_port = htons(80);
    inet_aton("205.196.221.150", &sain.sin_addr);
    connect(sockfd,(struct sockaddr*)&sain, sizeof(sain));

    send(sockfd, request, strlen(request), 0);
    
    return sockfd;
}

char* http_recv(int sockfd, int* somelen)
{
    int tlen = 0;
    int rlen = 0;
    int clen = 0;
    char buf[512];
    
    while(true) {
        if (tlen != 0)
            break;
        rlen = recv(sockfd, buf, 511, 0);
        if(rlen > 0)
            buf[rlen] = 0;

        for(int c = 0; c < rlen; ++c) {
            if (buf[c] == '\n') {
                if (!strncmp("Content-Length", buf+c+1, LLEN)) {
                    char tmp[16];
                    sscanf(buf+c,"%s %d\r\n", tmp, &tlen);
                    break;
                }
            }
        }
        if (rlen == 0)
            break;
	}
    
    char* data = new char[tlen];
    
    for (int i = 0; i < rlen-4; ++i) {
        if (!strncmp(buf+i, "\r\n\r\n", 4)) {
            strcpy(data, buf+i+4);
            break;
        }
    }
    rlen = clen = 0;
    while (true) {
        rlen = recv(sockfd, data+clen, tlen-clen, 0);
        if (rlen == 0)
            break;
        clen += rlen;
    }
    
    *somelen = tlen;
    
    return data;
}

char* http_recv(int sockfd)
{
    int l;
    http_recv(sockfd, &l);
}
