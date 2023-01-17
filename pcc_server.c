// code based on "tcp_client.c" and "tcp_server.c" files from recitation 10
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>


unsigned int pcc_total[95];

int sigint_flag = 0;
int connfd = -1;

void print_pcc_total(){
    int i;
    for (i = 0; i < 95; ++i){
        printf("char '%c' : %u times\n", i+32, pcc_total[i]);
    }
}

// a function that prints valid char statistics if sigint happens
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
    int connection_lost_flag;
    int not_written, total_sent, nsent;
    int not_read, total_read, nread;

    int buff_size;

    unsigned int current_pcc_total[95];
    char data_buff[1024];
    int listenfd  = -1;
    uint32_t C, N;

    if (argc != 2){
        perror("Invalid input");
        exit(1);
    }
    int server_port = atoi(argv[1]);

    sig_handler();
    memset( &pcc_total, 0, sizeof(pcc_total));



    struct sockaddr_in serv_addr; // where to listen
    struct sockaddr_in peer_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in );

    listenfd = socket( AF_INET, SOCK_STREAM, 0 );

    // Set socket options
//  written as explained in : https://linux.die.net/man/3/setsockopt
    int val = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
        perror("Error setting socket options");
        exit(1);
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

    while(! sigint_flag )
    {
        connection_lost_flag = 0;
        memset(current_pcc_total, 0, sizeof(current_pcc_total));
        // Accept a connection.
        connfd = accept( listenfd,
                         (struct sockaddr*) &peer_addr,
                         &addrsize);
        if( connfd < 0 )
        {
            if (errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE) {
                printf("\n Error : Accept Failed. %s \n", strerror(errno));
                close(connfd);
                connfd =-1;
                continue;
            }else if (connfd == 0 && connfd != errno){
                perror("connection lost");
                close(connfd);
                connfd =-1;
            }
            else{
                printf("\n Error : Accept Failed. %s \n", strerror(errno));
                exit(1);
            }
        }
        //    client writes file size to server
        not_read = 4; // how much we have left to read
        total_read = 0; // how much we read so far
        while (not_read > 0){
            nread = read(connfd, data_buff + total_read, not_read);
            if (nread <= 0){
                if (errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE){
                    perror("Failed reading N bytes from client to server");
                    close(connfd);
                    connfd =-1;
                    connection_lost_flag = 1;
                    break;
                }else if (nread == 0 && nread != errno){
                    perror("connection lost");
                    close(connfd);
                    connfd =-1;
                }
                else{
                    perror("Failed reading file size from client");
                    exit(1);
                }
            }
            total_read += nread;
            not_read -= nread;
        }
        if (connection_lost_flag){
            continue;
        }

        //    client writes N bytes from file to server

        memcpy(&N, data_buff, 4);
        N = ntohl(N);
        C=0;
        not_read = N; // how much we have left to read
        total_read = 0; // how much we read so far
        while (not_read > 0){
            if (sizeof (data_buff) <= not_read){
                buff_size = sizeof (data_buff);
            } else{
                buff_size = not_read;
            }
            nread = read(connfd, data_buff, buff_size);
            if (nread <= 0){
                if (errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE){
                    perror("Failed sending N bytes to server");
                    close(connfd);
                    connfd =-1;
                    connection_lost_flag = 1;
                    break;
                }else if (nread == 0 && nread != errno) {
                    perror("connection lost");
                    close(connfd);
                }
                else{
                    perror("Failed sending N bytes to server");
                    exit(1);
                }
            }
            //       update current_pcc_total of current client
            for (int i = 0; i < nread; ++i) {
                if ( 32 <= data_buff[i] && data_buff[i] <= 126){
                    current_pcc_total[data_buff[i] - 32] ++;
                    C ++;
                }
            }
            total_read += nread;
            not_read -= nread;
        }
        C = htonl(C);
        memcpy(data_buff, &C, 4);
//        write C to client
        not_written = 4; // how much we have left to write
        total_sent = 0; // how much we've written so far
        while (not_written > 0){
            nsent = write(connfd, data_buff, not_written);
            if (nsent < 0){
                if (errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE){
                    perror("Failed sending C to client");
                    close(connfd);
                    connfd =-1;
                    connection_lost_flag = 1;
                    break;
                }else if (nsent == 0 && nsent != errno) {
                    perror("connection lost");
                    close(connfd);
                }
                else{
                    perror("Failed sending C to client");
                    exit(1);
                }
            }
            total_sent += nsent;
            not_written -= nsent;
        }
        if (connection_lost_flag){
            continue;
        }

//        update pcc_total before closing connection
        for (int i = 0; i < 95; ++i) {
            pcc_total[i] += current_pcc_total[i];
        }
        // close socket
        close(connfd);
        connfd=-1;
    }
    if (sigint_flag){
        print_pcc_total();
        exit(0);
    }
}
