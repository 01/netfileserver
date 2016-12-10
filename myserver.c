#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

#include <sys/stat.h>

#include "libnetfiles.h"


/////////////////////////////////////////////////////////////
//
// Data type and configuration definitions
//
// ---------------------------------------------------------
//
// Max size of the file descriptor table.  

//#define FD_TABLE_SIZE   5  

static int  terminate = FALSE;
void *workerThread( void *newSocket_FD );

/********Data structure of a file descriptor************/

typedef struct {
    int  fd;                      // File descriptor (must be negative)
    FILE_CONNECTION_MODE fcMode;  // File connection mode
    int fileOpenFlags;            // Open file flags
    char pathname[256];           // file path name
} NET_FD_TYPE;

int main(int argc, char *argv[])
{
    int sockfd = 0;
    int newsockfd = 0;
    int n;

    struct sockaddr_in serv_addr, cli_addr;
    int clilen = sizeof(cli_addr);
    pthread_t    Worker_thread_ID = 0;
    char buffer[BUFFER_SIZE] = "";
 	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr,"netfileserver: socket() failed, errno= %d\n", errno);
        exit(EXIT_FAILURE);
    }

	bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
    {
        fprintf(stderr,"netfileserver: bind() failed, errno= %d\n", errno);
        exit(EXIT_FAILURE);
    }


    //
    // Listen on the socket.  Allow a connection 
    // backlog of size "50".
    //
    if (listen(sockfd, 50) < 0)
    {
        fprintf(stderr,"netfileserver: listen() failed, errno= %d\n", errno);
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("netfileserver: listener is li");
        NET_FUNCTION_TYPE netFunc;
        newsockfd=accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if(newsockfd<0){
            error("Error on accept");
        }

        bzero(buffer, BUFFER_SIZE);

        n=read(newsockfd, buffer, BUFFER_SIZE-1);

        if(n<0){
            error("Error reading from socket");
        }
        sscanf(buffer, "%u,", &netFunc);
        if(netFunc==NET_SERVERINIT) {
            sprintf(buffer, "%d,0,0,0", SUCCESS);
            //close(newsockfd);
            //close(sockfd);
        }
        printf("%d\n", netFunc);
        if(netFunc==NET_OPEN) sprintf(buffer, "%d,%d,%d,%d", SUCCESS, errno, h_errno, n);

    }
    //return 0; 

while(terminate == FALSE)
    {
        //printf("netfileserver: listener is waiting to accept incoming request\n");
        if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) < 0)
        {
            //
            // Socket accept function returns an error
            //
            if ( errno != EINTR )
            {
                fprintf(stderr,"netfileserver: accept() failed, errno= %d\n", errno);

            	close(newsockfd);
            	if ( sockfd != 0 ) close(sockfd);
                exit(EXIT_FAILURE);
            }
            else
            {
            	close(newsockfd);
            	if (sockfd != 0 )    close(sockfd);
                terminate = TRUE;  // Signal listener to terminate
            };
        }
        else
        {
            //printf("netfileserver: listener accepted a new request from socket\n");

            pthread_create(&Worker_thread_ID, NULL, &workerThread, &newsockfd );

            //printf("netfileserver: listener spawned a new worker thread with ID %d\n",Worker_thread_ID);
        };
    };


    if ( newsockfd != 0 ) close(newsockfd);
    if ( sockfd != 0 ) close(sockfd);

    printf("netfileserver: terminated\n");
return 0;

}

void *workerThread(void *newSocket_FD)
{
    int rc = 0;
    int fd = -1;
    int *sockfd = newSocket_FD;
    NET_FD_TYPE  *newFd = NULL;

    char buffer[BUFFER_SIZE] = "";
    char myThread[64] = "";
    NET_FUNCTION_TYPE netFunc = INVALID;

    sprintf(myThread, "netfileserver: workerThread %d,", pthread_self());


    rc = pthread_detach(pthread_self());


    rc = read(*sockfd, buffer, BUFFER_SIZE -1);
    if ( rc < 0 ) {
        fprintf(stderr,"%s fails to read from socket\n", myThread);
        if ( *sockfd != 0 ) close(*sockfd);
		pthread_exit(NULL);
    }
    else 
    {
        printf("%s received \"%s\"\n", myThread, buffer);
    }

/*************Decode the incoming message. Find Out What Operation to Do ******************/
    
    sscanf(buffer, "%u,", &netFunc);
 
    switch (netFunc)
    {
        case NET_SERVERINIT:
            
            sprintf(buffer, "%d,0,0,0", SUCCESS);
            //printf("%s responding with \"%s\"\n", myThread, buffer);
            break;

        case NET_OPEN:
            //printf("%s received \"netopen\"\n", myThread);
            // Incoming message format is:
            //     2,connectionMode,fileOpenFlags,pathname
            //
            newFd = malloc(sizeof(NET_FD_TYPE));

            sscanf(buffer, "%u,%d,%d,%s", &netFunc, (int *)&(newFd->fcMode), 
                      &(newFd->fileOpenFlags), newFd->pathname );


            //
            // The "Do_netopen" function returns a new file descriptor 
            // on success.  Otherwise, it will return a "-1".
            //
            rc = 0;   //Do_netopen( newFd );
            //printf("%s Do_netopen returns fd %d\n", myThread, rc);


            //
            // Compose a response message.  The format is:
            //
            //    result,errno,h_errno,netFd
            //
            if ( rc == FAIL) {
                sprintf(buffer, "%d,%d,%d,%d", FAIL,errno, h_errno, FAIL);
            } 
            else {
                sprintf(buffer, "%d,%d,%d,%d", SUCCESS, errno, h_errno, rc);
            }
            //printf("%s responding with \"%s\"\n", myThread, buffer);

            free( newFd );
            break;

        case NET_READ:
            printf("%s received \"netread\"\n", myThread);

            //printf("%s responding with \"%s\"\n", myThread, buffer);
            break;

        case NET_WRITE:
            printf("%s received \"netwrite\"\n", myThread);

            //printf("%s responding with \"%s\"\n", myThread, buffer);
            break;

        case NET_CLOSE:
            //printf("%s received \"netclose\"\n", myThread);

            //
            // Incoming message format is:
            //     5,fd,0,0
            //
            sscanf(buffer, "%u,%d", &netFunc, &fd); 


            //
            // On success, "deleteFD" returns the file descriptor 
            // that was closed.  Otherwise, it will return a "-1".
            //
            //printf("%s trying to delete fd %d\n", myThread, fd);
           // rc = deleteFD(fd);
            //printf("%s deleteFD returns %d\n", myThread, rc);


            //
            // Compose a response message.  The format is:
            //
            //    result,errno,h_errno,netFd
            //
            if ( rc == FAIL) {
                sprintf(buffer, "%d,%d,%d,%d", FAIL, errno, h_errno, rc);
            } 
            else {
                sprintf(buffer, "%d,%d,%d,%d", SUCCESS, errno, h_errno, rc);
            }
            //printf("%s responding with \"%s\"\n", myThread, buffer);

            break;

        case INVALID:
        default:
            printf("%s received invalid net function\n", myThread);
            break;
    }



    //
    // Send my server response back to the client
    //
    rc = write(*sockfd, buffer, strlen(buffer) );
    if ( rc < 0 ) {
        fprintf(stderr,"%s fails to write to socket\n", myThread);
    }
    
    if ( *sockfd != 0 ) close(*sockfd);
    pthread_exit( NULL );
}

