// code based on "tcp_client.c" and "tcp_server.c" files from recitation 10

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <fcntl.h>


int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Invalid input");
        return 1;
    }
    int not_read, total_read, nread, bytes_read;
    int not_written, total_sent, nsent;

    char *server_ip_add = argv[1];
    uint16_t server_port = (uint16_t) atoi(argv[2]);
    char *file_path = argv[3];
    int sockfd = -1;
    char recv_buff[1024];
    uint32_t C, N;
    struct sockaddr_in serv_addr;

//  Open the specified file for reading
    FILE *fp = fopen(file_path, "r");
    if (fp == NULL) {
        perror("Error opening file");
        exit(1);
    }
    memset(recv_buff, 0, sizeof(recv_buff));

    //   get file size taken from: https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
    fseek(fp, 0, SEEK_END);
    N = ftell(fp);
    rewind(fp);
    N =  htonl(N);


//    read N bytes to buffer
    memcpy( recv_buff, &N, 4 );


//  Create a TCP connection to the specified server port on the specified server IP

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("\n Error : Could not create socket \n");
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip_add, &serv_addr.sin_addr);

    if (connect(sockfd,
                (struct sockaddr *) &serv_addr,
                sizeof(serv_addr)) < 0) {
        printf("\n Error : Connect Failed. %s \n", strerror(errno));
        exit(1);
    }

//  sending file size to server
    not_written = 4; // how much we have left to write
    total_sent = 0; // how much we've written so far
    while (not_written > 0){
        nsent = write(sockfd, recv_buff + total_sent, not_written);
        if (nsent < 0){
            perror("Failed sending file size to server");
            exit(1);
        }
        total_sent += nsent;
        not_written -= nsent;
    }

    //  sending N bytes to server
    int fd = open(file_path,  O_RDONLY );
    while ((bytes_read = read(fd, recv_buff, sizeof (recv_buff)))){
        not_written = bytes_read; // how much we have left to write
        total_sent = 0; // how much we've written so far
        while (not_written > 0){
            nsent = write(sockfd, recv_buff + total_sent, not_written);
            if (nsent < 0){
                perror("Failed sending N bytes to server");
                exit(1);
            }
            total_sent += nsent;
            not_written -= nsent;
        }
    }

    //    get C from server
    not_read = 4; // how much we have left to write
    total_read = 0; // how much we've written so far
    while (not_read > 0){
        nread = read(sockfd, recv_buff + total_read, not_read);
        if (nread <= 0){
            perror("Failed reading C from server");
            exit(1);
        }
        total_sent += nread;
        not_read -= nread;
    }


    close(sockfd);
    memcpy( &C , recv_buff, 4 );
    C = ntohl(C);
    printf("# of printable characters: %u\n", C);
    exit(0);
}