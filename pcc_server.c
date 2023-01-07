// taken from "tcp_server.c" file from recitation 10

#define _GNU_SOURCE

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <assert.h>
#include <signal.h>

uint32_t pcc_total[95];

int main(int argc, char *argv[])
{
    if (argc != 2){
        perror("Invalid input");
    }
    uint16_t server_port;
    server_port = atoi(argv[1]);

//    If the server receives a SIGINT signal,
//    taken from my solution to HW2:
    struct sigaction newAction;
    memset(&newAction, 0, sizeof(newAction));
    newAction.sa_handler = SIG_IGN;
    newAction.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &newAction, NULL) == -1) {
        fprintf(stderr, "Signal handle registration failed on error: %s\n", strerror(errno));
        return 1;
    }

    int pcc_total = -1;
    int nsent     = -1;
    int len       = -1;
    int n         =  0;
    int listenfd  = -1;
    int connfd    = -1;


    struct sockaddr_in serv_addr; // where to listen
    struct sockaddr_in my_addr;
    struct sockaddr_in peer_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in );

    char data_buff[1024];
    time_t ticks;
    listenfd = socket( AF_INET, SOCK_STREAM, 0 );
    memset( &serv_addr, 0, addrsize );

    serv_addr.sin_family = AF_INET;
    // INADDR_ANY = any local machine address
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(server_port);

    if( 0 != bind( listenfd,
                   (struct sockaddr*) &serv_addr,
                   addrsize ) )
    {
        printf("\n Error : Bind Failed. %s \n", strerror(errno));
        return 1;
    }

    if( 0 != listen( listenfd, 10 ) )
    {
        printf("\n Error : Listen Failed. %s \n", strerror(errno));
        return 1;
    }

    while( 1 )
    {
        // Accept a connection.
        // Can use NULL in 2nd and 3rd arguments
        // but we want to print the client socket details
        connfd = accept( listenfd,
                         (struct sockaddr*) &peer_addr,
                         &addrsize);

        if( connfd < 0 )
        {
            printf("\n Error : Accept Failed. %s \n", strerror(errno));
            return 1;
        }

        getsockname(connfd, (struct sockaddr*) &my_addr,   &addrsize);
        getpeername(connfd, (struct sockaddr*) &peer_addr, &addrsize);
        printf( "Server: Client connected.\n"
                "\t\tClient IP: %s Client Port: %d\n"
                "\t\tServer IP: %s Server Port: %d\n",
                inet_ntoa( peer_addr.sin_addr ),
                ntohs(     peer_addr.sin_port ),
                inet_ntoa( my_addr.sin_addr   ),
                ntohs(     my_addr.sin_port   ) );

        // write time
        ticks = time(NULL);
        snprintf( data_buff, sizeof(data_buff),
                  "%.24s\r\n", ctime(&ticks));

        pcc_total = 0;
        int notwritten = strlen(data_buff);

        // keep looping until nothing left to write
        while( notwritten > 0 )
        {
            // notwritten = how much we have left to write
            // totalsent  = how much we've written so far
            // nsent = how much we've written in last write() call */
            nsent = write(connfd,
                          data_buff + pcc_total,
                          notwritten);
            // check if error occured (client closed connection?)
            assert( nsent >= 0);
            printf("Server: wrote %d bytes\n", nsent);

            pcc_total  += nsent;
            notwritten -= nsent;
        }

        // close socket
        close(connfd);
    }
}
