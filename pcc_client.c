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


int main(int argc, char *argv[])
{
    if (argc != 4){
        printf("Invalid input");
        return 1;
    }
    char *server_ip_add = argv[1];
    int server_port = atoi(argv[2]);
    char *file_path = argv[3];

    uint32_t file_size;

    int  sockfd  = -1;
    size_t  bytes_read =  0;
    size_t  bytes_written =  0;

    unsigned int  printable_char_count =  0;

    char recv_buff[1024];

    struct sockaddr_in serv_addr; // where we Want to get to


//  Open the specified file for reading
    FILE* fp = fopen(file_path, O_RDONLY);
    if (fp == NULL) {
        perror("Error opening file");
        exit(1);
    }
    //   get file size taken from: https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
    fseek(fp, 0L, SEEK_END);
    file_size = (uint32_t) ftell(fp);
    rewind(fp);

//  Create a TCP connection to the specified server port on the specified server IP
    memset(recv_buff, 0,sizeof(recv_buff));

    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("\n Error : Could not create socket \n");
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port); // Note: htons for endiannes
    inet_pton(AF_INET, server_ip_add,&serv_addr.sin_addr);

    printf("Client: connecting...\n");
    if( connect(sockfd,
                (struct sockaddr*) &serv_addr,
                sizeof(serv_addr)) < 0)
    {
        printf("\n Error : Connect Failed. %s \n", strerror(errno));
        close(sockfd);
        fclose(fp);
        exit(1);
    }



//  client sends the server N, the number of bytes that will be transferred
//  The value N is a 32-bit unsigned integer in network byte order
    memcpy( recv_buff, &file_size, 4);
    if (write(sockfd, recv_buff, 4) != 4) {
        perror("Error sending file size");
        close(sockfd);
        fclose(fp);
        exit(1);
    }
//  The client sends the server N bytes
    while((bytes_read = read(sockfd,
                            recv_buff,
                            sizeof(recv_buff))))
    {
        bytes_written = write(sockfd,
                             recv_buff,
                              bytes_read);
        if( bytes_written <= 0 ){
            perror("Error sending file contents");
            close(sockfd);
            fclose(fp);
            exit(1);
        }
    }

//  The server sends the client C, the number of printable characters
    if(read(sockfd, recv_buff, 4) != 4) {
        perror("Error reading from server");
        close(sockfd);
        fclose(fp);
        exit(1);
    }

    memcpy(&printable_char_count, recv_buff, 4);
    printf("# of printable characters: %u\n", printable_char_count);
    close(sockfd);
    fclose(fp);
    exit(0);
}
