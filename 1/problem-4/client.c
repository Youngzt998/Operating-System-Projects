/*
Name :Ziteng Yang
ID: 517021910683
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

int main(int argc, char *argv[])
{
    /////////////////////////////////////////////////////////////////////
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    portno = 2050;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){ 
        printf("ERROR opening socket");
        exit(1);
    }

    server = gethostbyname("127.0.0.1");
    if (server == NULL) {
        printf("ERROR, no such host\n");
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        printf("ERROR connecting\n");
        exit(0);
    }

    printf("Please enter the message: \n");
    /////////////////////////////////////////////////////////////////////
    while(1){
        
        bzero(buffer,256);
        fgets(buffer,255,stdin); //read the message from stdin
        //printf("sending to server...\n");

        n = write(sockfd,buffer,255);    //write the message to the socket
        if (n < 0) {
            printf("ERROR writing to socket\n");
            exit(0);
        }

        if(strcmp(buffer,":q\n")==0) 
        {
            printf("you terminated client!\n");
            break;
        }   


        //printf("reading server's message...\n");
        bzero(buffer,256);
        n = read(sockfd,buffer,255);    //read reply from the socket
        if (n < 0){ 
            printf("ERROR reading from socket\n");
            exit(0);
        }
        
        printf("From server: %s\n",buffer);  //display this reply
    }

    close(sockfd);
    printf("client closing...\n");
    return 0;
}
