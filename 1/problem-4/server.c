/*
Name :Ziteng Yang
ID: 517021910683
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void *serve(void*newsockfd);
void *server_close_func(void *close);
int client_counter=0;   //count the client being serving 
int server_close=0;     //check if server should close
int total_client=0;     //count the total client that entered the server

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_total_client = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[])
{   

    /////////////////////////////////////////////////////////////////////
    int sockfd, newsockfd1,newsockfd2,portno,clilen,n;
    char buffer[256];
    struct sockaddr_in serv_addr,cli_addr;
    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd<0){
        printf("ERROR opening socket\n");
        exit(1);
    }
    
    bzero((char*)&serv_addr,sizeof(serv_addr));     //set zero

    portno = 2050;  //the port number of server is randomly assigned

    serv_addr.sin_family=AF_INET;       //It should always be set to the symbolic constant AF_INET
    serv_addr.sin_port=htons(portno);   //sin_port field contains port number, converted to network byte order
    serv_addr.sin_addr.s_addr=INADDR_ANY;   //ip address of server

    //bind(): bind a socket to an address
    if(bind(sockfd,(struct sockaddr*)&serv_addr, sizeof(serv_addr))<0){
        printf("Error on binding\n");
        exit(0);
    }


    /*
        The listen system call allows the process to listen on the socket for connections. 
        The first argument is the socket file descriptor, and the second is the size of the backlog queue, 
        i.e., the number of connections that can be waiting 
        while the process is handling a particular connection. 
        This should be set to 5, the maximum size permitted by most systems
    */
    listen(sockfd,5);   //listen on the socket for connection   //5:
    clilen = sizeof(cli_addr);
    printf("Server initialing...\n");
    /////////////////////////////////////////////////////////////////////
    
    pthread_t thread1, thread_get_input;
    int  iret1,ire_get_input;
    
    //a method to close server with input
    //not that perfect, but can avoid using ctrl+z to terminate server and thus avoid kill command
    ire_get_input = pthread_create(&thread_get_input,NULL,server_close_func,(void*)&server_close);
    if(ire_get_input)
    {
        fprintf(stderr,"Error - pthread_create() return code: %d\n",iret1);
        exit(EXIT_FAILURE);
    }

    //server is working
    while(1&&server_close!=1)
    {
	//create thread for input of server's master
        newsockfd1 = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if(newsockfd1<0) {
            printf("Error when receiving one client\n");
            continue;
        }
	
	//create thread for clients
        iret1 = pthread_create( &thread1, NULL, serve, (void*) &newsockfd1);
        if(iret1)
        {
            fprintf(stderr,"Error - pthread_create() return code: %d\n",iret1);
            exit(EXIT_FAILURE);
        }

    }

    
    close(sockfd);
    printf("server end!\n");
    return 0;
}

void *serve(void *sockfd)
{
    int newsockfd = (int)(*((int*)sockfd));
    char buffer[256];
    int n,number;

    pthread_mutex_lock(&mutex_total_client);
        total_client++;
    pthread_mutex_unlock(&mutex_total_client);
    number=total_client;


    n = read(newsockfd,buffer,255);
    if (n < 0) {
        printf("ERROR reading from socket");
        exit(0);
    }
    //full, client must wait
     while(client_counter>=2){

        if(strcmp(buffer,":q\n")==0)        //client terminated
        {
            printf("client-%d terminated!\n",number);
            close(newsockfd);
            return NULL;
        }
	//no encode but send wait message
         n = write(newsockfd,"Please wait!",12);
         if (n < 0) {
             printf("ERROR writing to socket");
             exit(0);
         }
	//read again
        n = read(newsockfd,buffer,255);
        if (n < 0) {
            printf("ERROR reading from socket");
            exit(0);
        }
	//if can encode, loop will be broke
     } 
    
    //client being served
    pthread_mutex_lock(&mutex1);
        client_counter++;
    pthread_mutex_unlock(&mutex1);


    //encode client's message
    while(1){

        printf("Here is the message from client-%d: %s\n",number,buffer);
        if(strcmp(buffer,":q\n")==0)
        {
            printf("client-%d terminated!\n",number);
            break;
        }
        //encode the message
        int i=0;
        while(i<255) 
        {   
            if((buffer[i]>='a'&&buffer[i]<='z'))
                buffer[i]='a'+(buffer[i]-'a'+3)%26;
            if((buffer[i]>='A'&&buffer[i]<='Z'))
                buffer[i]='A'+(buffer[i]-'A'+3)%26;
            
            i++;
        }
        
        //write back the encoded message
        n = write(newsockfd,buffer,255);
        if (n < 0) {
            printf("ERROR writing to socket");
            exit(0);
        }

        //read the message
        bzero(buffer,256);
        n = read(newsockfd,buffer,255);
        if (n < 0) {
            printf("ERROR reading from socket");
            exit(0);
        }
    }

    pthread_mutex_lock(&mutex1);
        client_counter--;
    pthread_mutex_unlock(&mutex1);


    close(newsockfd);
}

//for server's master
void *server_close_func(void *close)
{
    char input[32];
    printf("input \"close\" to close the server\n");
    while(1)
    {
        fgets(input,32,stdin);
        if(strcmp(input,"close\n")==0) {
            *((int*)close)=1;
            break;
        }
    }
}
