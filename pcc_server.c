// code based on "tcp_server.c" file from recitation 10

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

static unsigned int pcc_total[95] = {0};
int sigint_flag = 0;
int connfd = -1;

void print_pcc_total(){
    int i;
    for (i = 0; i < 95; ++i){
        printf("char '%c' : %u times\n", i+32, pcc_total[i]);
    }
}

// a function that prints valid char if Cntrl+C is pressed
void sigint_handler(){
    sigint_flag = 1;
    if (connfd <= 0){
        print_pcc_total();
        exit(0);
    }
}

int sig_handler(){
    //    If the server receives a SIGINT signal,
//    taken from my solution to HW2:
    struct sigaction newAction;
    memset(&newAction, 0, sizeof(newAction));
    newAction.sa_handler = sigint_handler;
    newAction.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &newAction, NULL) == -1) {
        fprintf(stderr, "Signal handle registration failed on error: %s\n", strerror(errno));
        return 1;
    }
    return 0;
}


int main(int argc, char *argv[])
{
    unsigned int current_pcc_total[95] = {0};
    char data_buff[1024];
    int totalsent = -1;
    int Nsent     = -1;
    int listenfd  = -1;
    int printable_counter = 0;

    if (argc != 2){
        perror("Invalid input");
        exit(1);
    }
    uint16_t server_port = (uint16_t) atoi(argv[1]);
    sig_handler();


    struct sockaddr_in serv_addr; // where to listen
    struct sockaddr_in my_addr;
    struct sockaddr_in peer_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in );
    listenfd = socket( AF_INET, SOCK_STREAM, 0 );

    // Set socket options
//  written as explained in : https://linux.die.net/man/3/setsockopt
    int val = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
        perror("Error setting socket options");
        close(listenfd);
        return 1;
    }

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

    while( !sigint_flag )
    {
        // Accept a connection.
        connfd = accept( listenfd,
                         (struct sockaddr*) &peer_addr,
                         &addrsize);

        if( connfd < 0)
        {
            if (errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE){
                printf("\n Error : Accept Failed. %s \n", strerror(errno));
                close(connfd);
                connfd=-1;
                continue;

            }
            else{
                printf("\n Error : Accept Failed. %s \n", strerror(errno));
                exit(1);
            }
        }
        getsockname(connfd, (struct sockaddr*) &my_addr,   &addrsize);
        getpeername(connfd, (struct sockaddr*) &peer_addr, &addrsize);

        totalsent = 0;
        int notwritten = strlen(data_buff);

        // keep looping until nothing left to write
        while( notwritten > 0 )
        {
            // notwritten = how much we have left to write
            // totalsent  = how much we've written so far
            // Nsent = how much we've written in last write() call */
            Nsent = read(connfd,
                          data_buff + totalsent,
                         notwritten);
            // check if error occured (client closed connection?)
            if (Nsent < 0){
                if (errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE){
                    printf("\n Error : Accept Failed. %s \n", strerror(errno));
                    close(connfd);
                    connfd=-1;
                    continue;
                }
                else{
                    printf("\n Error : Accept Failed. %s \n", strerror(errno));
                    exit(1);
                }
            }
            printf("Server: wrote %d bytes\n", Nsent);
//            update current_pcc_total of current client
            for (int i = 0; i < notwritten; ++i) {
                if ( 32 <= data_buff[i] && data_buff[i] <= 126){
                    current_pcc_total[data_buff[i] - 32] ++;
                    printable_counter ++;
                }

            }
            totalsent  += Nsent;
            notwritten -= Nsent;
        }

        // Send character count to client
        memcpy(data_buff, &printable_counter, 4 );
        if (write(connfd, data_buff, 4) != 4){
            if (errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE){
                printf("\n Error : Accept Failed. %s \n", strerror(errno));
                close(connfd);
                connfd=-1;
                continue;
            }
            else{
                printf("\n Error : Accept Failed. %s \n", strerror(errno));
                exit(1);
            }
        }

//        update pcc_total before closing connection
        for (int i = 0; i < 1024; ++i) {
            pcc_total[i] += current_pcc_total[i];
        }
        // close socket
        close(connfd);
        connfd=-1;
    }
    print_pcc_total();
    exit(0);
}
