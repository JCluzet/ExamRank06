#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int maxFd = 0, numberofClients = 0, id_by_sock[65536], currentmessage[65536];
fd_set ready_to_read, ready_to_write, active_sockets;
char buf_to_read[4096 * 42], buf_to_write[4096 * 42 + 42], buf_str[4096 * 42];
void sendAll(int except)
{
    for(int sock = 0; sock <= maxFd; sock++)
    {
        if(FD_ISSET(sock, &ready_to_write) && except != sock)
            send(sock, buf_to_write, strlen(buf_to_write), 0);
    }
}
void fatalError(void)
{
    write(2, "Fatal error\n", strlen("Fatal error\n"));
    exit(1);
}
int main(int argc, char **argv)
{
    if(argc != 2)
    {
        write(2, "Wrong number of arguments\n", strlen("Wrong number of arguments\n"));
        exit(1);
    }

    uint16_t port = atoi(argv[1]);
    struct sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = (1 << 24) | 127;
    serveraddr.sin_port = (port >> 8) | (port << 8);
    int server_sock = -1;

    if((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        fatalError();

    if(bind(server_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1)
    {
        close(server_sock);
        fatalError();
    }

    if(listen(server_sock, 128) == -1)
    {
        close(server_sock);
        fatalError();
    }

    bzero(id_by_sock, sizeof(id_by_sock));
    FD_ZERO(&active_sockets);
    FD_SET(server_sock, &active_sockets);
    maxFd = server_sock;

    while(1)
    {
        ready_to_write = ready_to_read = active_sockets;
        if(select(maxFd + 1, &ready_to_read, &ready_to_write, 0, 0) <= 0)
            continue;
        for(int sock = 0; sock <= maxFd; sock++)
        {
            if(FD_ISSET(sock, &ready_to_read))
            {
                if(sock == server_sock)
                {
                    struct sockaddr_in clientaddr;
                    socklen_t len = sizeof(clientaddr);
                    int client_sock = accept(sock, (struct sockaddr*)&client_sock, &len);
                    if(client_sock == -1)
                        continue;
                    FD_SET(client_sock, &active_sockets);
                    id_by_sock[client_sock] = numberofClients++;
                    currentmessage[client_sock] = 0;
                    if(maxFd < client_sock)
                        maxFd = client_sock;
                    sprintf(buf_to_write, "server: client %d just arrived\n", id_by_sock[client_sock]);
                    sendAll(sock);
                    break;
                }
                else
                {
                    int ret = recv(sock, buf_to_read, 4096 * 42, 0);
                    if(ret <= 0)
                    {
                        sprintf(buf_to_write, "server: client %d just left\n", id_by_sock[sock]);
                        sendAll(sock);
                        FD_CLR(sock, &active_sockets);
                        close(sock);
                        break;
                    }
                    else 
                    {
                        for(int i = 0, j = 0; i < ret; i++, j++)
                        {
                            buf_str[j] = buf_to_read[i];
                            if(buf_str[j] == '\n')
                            {
                                buf_str[j+1] = '\0';
                                if(currentmessage[sock])
                                    sprintf(buf_to_write, "%s", buf_str);
                                else
                                    sprintf(buf_to_write, "client %d: %s", id_by_sock[sock], buf_str);
                                currentmessage[sock] = 0;
                                sendAll(sock);
                                j = -1;
                            }
                            else if(i == (ret - 1))
                            {
                                buf_str[j+1] = '\0';
                                if (currentmessage[sock])
                                    sprintf(buf_to_write, "%s", buf_str);
                                else
                                    sprintf(buf_to_write, "client %d: %s", id_by_sock[sock], buf_str);
                                currentmessage[sock] = 1;
                                sendAll(sock);
                                break;
                            }}}}}}}
    return(0);
}
