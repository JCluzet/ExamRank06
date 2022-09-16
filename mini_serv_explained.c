#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int maxFd = 0;
int numberofClients = 0;
int id_by_sock[65536], currentmessage[65536];

fd_set ready_to_read, ready_to_write, active_sockets;
char buf_to_read[4096 * 42], buf_to_write[4096 * 42 + 42], buf_str[4096 * 42];

// ==> Send the message stock on buf_to_write by sprintf, to all clients except one
void sendAll(int except)
{
    for(int sock = 0; sock <= maxFd; sock++)
    {
        if(FD_ISSET(sock, &ready_to_write) && except != sock)
            send(sock, buf_to_write, strlen(buf_to_write), 0);
    }
}

// ==> Classic fatalError
void fatalError(void)
{
    write(2, "Fatal error\n", strlen("Fatal error\n"));
    exit(1);
}

int main(int argc, char **argv)
{
    // ========== ESTABLISHING CONNEXION 📡 =========== //

    // ==> Wrong number arguments check
    if(argc != 2)
    {
        write(2, "Wrong number of arguments\n", strlen("Wrong number of arguments\n"));
        exit(1);
    }

    // ==> Create all server details (port, IP)
    uint16_t port = atoi(argv[1]);
    struct sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = (1 << 24) | 127;
    serveraddr.sin_port = (port >> 8) | (port << 8);

    int server_sock = -1;

    // find the socket server
    if((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        fatalError();

    // bind the socket to the good port with sockaddr struct
    if(bind(server_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1)
    {
        close(server_sock);
        fatalError();
    }

    // listen server socket
    if(listen(server_sock, 128) == -1)
    {
        close(server_sock);
        fatalError();
    }

    // set to zero all we need
    bzero(id_by_sock, sizeof(id_by_sock));
    FD_ZERO(&active_sockets);
    FD_SET(server_sock, &active_sockets);
    maxFd = server_sock;


    // ========== CONNEXION SET ✅ =========== //


    // ========== TREAT ENTRANCE MESSAGE =========== //
    while(1)
    {
        ready_to_write = ready_to_read = active_sockets;
        if(select(maxFd + 1, &ready_to_read, &ready_to_write, 0, 0) <= 0)
            continue;

        // for all clients 
        for(int sock = 0; sock <= maxFd; sock++)
        {
            // if the client is ready to be read
            if(FD_ISSET(sock, &ready_to_read))
            {
                // if the actuel client (ready to be read) is the server one (means a new connexion)
                if(sock == server_sock)
                {
                    // create a new client struct for stock new client
                    struct sockaddr_in clientaddr;
                    socklen_t len = sizeof(clientaddr);
                    // accept the client
                    int client_sock = accept(sock, (struct sockaddr*)&client_sock, &len);
                    if(client_sock == -1)
                        continue;
                    
                    // put the client into active_sockets
                    FD_SET(client_sock, &active_sockets);
                    id_by_sock[client_sock] = numberofClients++;
                    currentmessage[client_sock] = 0;
                    if(maxFd < client_sock)
                        maxFd = client_sock;

                    // sending message to all clients
                    sprintf(buf_to_write, "server: client %d just arrived\n", id_by_sock[client_sock]);
                    sendAll(sock);
                    break;
                }
                // else the sock is ready to be read but it's not the server one (means the client is left or send a message to treat)
                else
                {
                    // catch the message of the socket
                    int ret = recv(sock, buf_to_read, 4096 * 42, 0);
                    // if the lenght of message is <= than 0, it's mean the client left. 
                    if(ret <= 0)
                    {
                        sprintf(buf_to_write, "server: client %d just left\n", id_by_sock[sock]);
                        sendAll(sock);
                        // clear this client from active_socket & close the client socket.
                        FD_CLR(sock, &active_sockets);
                        close(sock);
                        break;
                    }
                    // else there is a message to treat
                    else 
                    {
                        // for every character of the message
                        for(int i = 0, j = 0; i < ret; i++, j++)
                        {
                            // copy it into buf_str
                            buf_str[j] = buf_to_read[i];
                            if(buf_str[j] == '\n')
                            {
                                // if there is a \n, we send a line with all text to \n
                                buf_str[j+1] = '\0';
                                if(currentmessage[sock])
                                    sprintf(buf_to_write, "%s", buf_str);
                                else
                                    sprintf(buf_to_write, "client %d: %s", id_by_sock[sock], buf_str);
                                currentmessage[sock] = 0;
                                sendAll(sock);
                                j = -1;
                            }
                            else if(i == (ret - 1)) // if this is the last character of the message and it's not ending by a \n (means we need to send it without \n)
                            {
                                buf_str[j+1] = '\0';
                                if (currentmessage[sock])
                                    sprintf(buf_to_write, "%s", buf_str);
                                else
                                    sprintf(buf_to_write, "client %d: %s", id_by_sock[sock], buf_str);
                                currentmessage[sock] = 1;
                                sendAll(sock);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    return(0);
}
