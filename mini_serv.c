#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>

int max_fd = 0, g_id = 0, id[65536], currmsg[65536], sock_fd = -1;
fd_set fdread, fdwrite, currentsock;
char buf[4096 * 42], msg[4096 * 42 + 42], str[4096 * 42];

void sendAll(int except) {
    for(int fd = 0; fd <= max_fd; fd++)
        if(FD_ISSET(fd, &fdwrite) && except != fd)
            send(fd, msg, strlen(msg), 0);
}

void fatal() {
    write(2, "Fatal error\n", strlen("Fatal error\n"));
    close(sock_fd);
    exit(1);
}

int main(int argc, char **argv) 
{
    if(argc != 2) {
        write(2, "Wrong number of arguments\n", strlen("Wrong number of arguments\n"));
        exit(1);
    }
    uint16_t port = atoi(argv[1]);
    struct sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(2130706433);
    serveraddr.sin_port = htons(port);
    if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        fatal();
    if(bind(sock_fd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1)
        fatal();
    if(listen(sock_fd, 128) == -1)
        fatal();
    bzero(id, sizeof(id));
    FD_ZERO(&currentsock);
    FD_SET(sock_fd, &currentsock);
    max_fd = sock_fd;

    while(1) {
        fdwrite = fdread = currentsock;
        if(select(max_fd + 1, &fdread, &fdwrite, 0, 0) <= 0)
            continue;
        for(int fd = 0; fd <= max_fd; fd++) {
            if(FD_ISSET(fd, &fdread)) {
                if(fd == sock_fd) {
                    struct sockaddr_in clientaddr;
                    socklen_t len = sizeof(clientaddr);
                    int clientfd = accept(fd, (struct sockaddr*)&clientaddr, &len);
                    if(clientfd == -1)
                        continue;
                    FD_SET(clientfd, &currentsock);
                    id[clientfd] = g_id++;
                    currmsg[clientfd] = 0;
                    if(max_fd < clientfd)
                        max_fd = clientfd;
                    sprintf(msg, "server: client %d just arrived\n", id[clientfd]);
                    sendAll(fd);
                    break;
                }
                else {
                    int ret = recv(fd, buf, 4096 * 42, 0);
                    if(ret <= 0) {
                        sprintf(msg, "server: client %d just left\n", id[fd]);
                        sendAll(fd);
                        FD_CLR(fd, &currentsock);
                        close(fd);
                        break;
                    }
                    else {
                        for(int i = 0, j = 0; i < ret; i++, j++) {
                            str[j] = buf[i];
                            if(str[j] == '\n') {
                                str[j+1] = '\0';
                                if(currmsg[fd])
                                    sprintf(msg, "%s", str);
                                else
                                    sprintf(msg, "client %d: %s", id[fd], str);
                                currmsg[fd] = 0;
                                sendAll(fd);
                                j = -1;
                            }
                            else if(i == (ret - 1)) {
                                str[j+1] = '\0';
                                if (currmsg[fd])
                                    sprintf(msg, "%s", str);
                                else
                                    sprintf(msg, "client %d: %s", id[fd], str);
                                currmsg[fd] = 1;
                                sendAll(fd);
                                break;
                            }}}}}}};
    return(0);
}