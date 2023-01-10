// taken from "tcp_client.c" file from recitation 10

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>


int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Invalid input");
        return 1;
    }
    char *server_ip_add = argv[1];
    uint16_t server_port = (uint16_t) atoi(argv[2]);
    char *file_path = argv[3];
    int sockfd = -1;
    char *recv_buff;
    uint32_t C, N;
    struct sockaddr_in serv_addr;

//  Open the specified file for reading
    FILE *fp = fopen(file_path, "r");
    if (fp == NULL) {
        perror("Error opening file");
        exit(1);
    }

    //   get file size taken from: https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
    fseek(fp, 0, SEEK_END);
    N = ftell(fp);
    rewind(fp);

    recv_buff = (char*)malloc(N * sizeof(char));


//    read N bytes to buffer
    if (fread(recv_buff, sizeof(char), N, fp) != N) {
        perror("Failed reading N bytes from file");
        exit(1);
    }

//  Create a TCP connection to the specified server port on the specified server IP

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("\n Error : Could not create socket \n");
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port); // Note: htons for endiannes
    inet_pton(AF_INET, server_ip_add, &serv_addr.sin_addr);

    if (connect(sockfd,
                (struct sockaddr *) &serv_addr,
                sizeof(serv_addr)) < 0) {
        printf("\n Error : Connect Failed. %s \n", strerror(errno));
        exit(1);
    }


//    code taken from: https://stackoverflow.com/questions/9140409/transfer-integer-over-a-socket-in-c
    uint32_t conv = htonl(N);
    char *data = (char *) &conv;
    int bytes_left_to_write = 0;
    int bytes_read = 0;
//    client writes file size to server
    while (bytes_left_to_write < 4){
        bytes_read = write(sockfd, data + bytes_left_to_write, 4 - bytes_left_to_write);
        if (bytes_read < 0) {
            perror("Failed sending file size to server");
            exit(1);
        }
        else if (bytes_read == 0 && bytes_left_to_write < 4){
            perror("Failed sending file size to server");
            exit(1);
        }
        bytes_left_to_write += bytes_read;
    }

    //    client writes N bytes from file to server
    bytes_left_to_write = 0;
    bytes_read = 0;
    while (bytes_left_to_write < N){
        bytes_read = write(sockfd, recv_buff + bytes_left_to_write, N - bytes_left_to_write);
        if (bytes_read < 0) {
            perror("Failed sending N bytes from file to server");
            exit(1);
        }
        else if (bytes_read == 0 && bytes_left_to_write < N){
            perror("Failed sending N bytes from file to server");
            exit(1);
        }
        bytes_left_to_write += bytes_read;
    }
    free(recv_buff);


//    get C from server
    int bytes_read_from_server = 0;
    while (bytes_read_from_server < 4){
        bytes_read = read(sockfd, data, 4 - bytes_read_from_server);
        if (bytes_read < 0) {
            perror("Failed reading C from server");
            exit(1);
        }
        else if (bytes_read == 0 && bytes_read_from_server < 4){
            perror("Failed reading C from server");
            exit(1);
        }
        data += bytes_read;
        bytes_read_from_server += bytes_read;
    }
    close(sockfd);
    C = ntohl(conv);
    printf("# of printable characters: %u\n", C);
    exit(0);
}
