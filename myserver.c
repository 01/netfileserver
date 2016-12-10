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


#define FD_TABLE_SIZE   5  

static int  terminate = FALSE;
void *workerThread( void *newSocket_FD );

/********Data structure of a file descriptor************/

typedef struct {
    int  fd;                                                                                        // File descriptor int (must be negative)
    CONNECTION_MODE fMode;                                                                          // Connection Mode
    int fileFlags;                                                                                  // File Flags
    char filePath[256];                                                                             // File Path 
} fileDescriptor;


fileDescriptor FD_Table[FD_TABLE_SIZE];

void initFDTable();
int findFD( fileDescriptor *fdPtr );
int createFD( fileDescriptor *fdPtr );
int deleteFD( int fd );
int canOpen(fileDescriptor *fdPtr );
void *workerThread( void *newSocket_FD );
int ex_netopen( fileDescriptor *fdPtr );

int main(int argc, char *argv[])
{
    int sockfd = 0;
    int newsockfd = 0;
    int n;

    struct sockaddr_in serv_addr, cli_addr;
    int clilen = sizeof(cli_addr);
    pthread_t    Worker_thread_ID = 0;
    initFDTable();
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

    if (listen(sockfd, 50) < 0)
    {
        fprintf(stderr,"netfileserver: listen() failed, errno= %d\n", errno);
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    else{}


while(terminate == FALSE){
        printf("netfileserver: listener is waiting to accept incoming request\n");
        if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) < 0)
        {
            if ( errno != EINTR ){
                fprintf(stderr,"netfileserver: accept() failed, errno= %d\n", errno);
            	close(newsockfd);
            	if ( sockfd != 0 ) close(sockfd);
                exit(EXIT_FAILURE);
            }
            else{
            	close(newsockfd);
            	if (sockfd != 0 ) close(sockfd);
                terminate = TRUE;  
            }
        }
        else{
            printf("netfileserver: listener accepted a new request from socket\n");

            pthread_create(&Worker_thread_ID, NULL, &workerThread, &newsockfd );

            printf("netfileserver: listener spawned a new worker thread with ID Thread : %lu\n",Worker_thread_ID);
        }
    }


    if ( newsockfd != 0 ) close(newsockfd);
    if ( sockfd != 0 ) close(sockfd);

    printf("netfileserver: terminated\n");
    return 0;

}

void *workerThread(void *newSocket_FD)
{
    int n = 0;
    int fd = -1;
    int *sockfd = newSocket_FD;
    fileDescriptor  *newFd = NULL;

    char buffer[BUFFER_SIZE] = "";
    NET_FUNCTION_TYPE netFunc = INVALID;

    n = pthread_detach(pthread_self());


    n = read(*sockfd, buffer, BUFFER_SIZE -1);
    if ( n < 0 ) {
        fprintf(stderr,"Thread: %lu failed to read from socket\n", pthread_self());
        if ( *sockfd != 0 ) close(*sockfd);
		pthread_exit(NULL);
    }
    else 
    {
        printf("Thread: %lu received \"%s\"\n", pthread_self(), buffer);
    }

/*************Decode the incoming message. Find Out What Operation to Do ******************/
    
    sscanf(buffer, "%u,", &netFunc);
 
    switch (netFunc)
    {
        case NET_SERVERINIT:
            
            sprintf(buffer, "%d,0,0,0", SUCCESS);
            printf("Thread : %lu responding with \"%s\"\n", pthread_self(), buffer);
            break;

        case NET_OPEN:
           printf("Thread : %lu received \"netopen\"\n", pthread_self());
                                                                                            // Incoming message format is:  2,connectionMode,fileFlags,filePath
            newFd = malloc(sizeof(fileDescriptor));

            sscanf(buffer, "%u,%d,%d,%s", &netFunc, (int *)&(newFd->fMode), &(newFd->fileFlags), newFd->filePath);

            n = ex_netopen( newFd );
            printf("Thread : %lu ex_netopen returns fd %d\n", pthread_self(), n);
                                                                                            // Compose a response message.  The format is: result,errno,h_errno,fdPtr
            if ( n == FAIL) {
                sprintf(buffer, "%d,%d,%d,%d", FAIL, errno, h_errno, n);
            } 
            else {
                sprintf(buffer, "%d,%d,%d,%d", SUCCESS, errno, h_errno, n);
            }
            printf("Thread : %lu responding with \"%s\"\n", pthread_self(), buffer);
            free( newFd );
            break;

        case NET_READ:
            printf("Thread : %lu received \"netread\"\n", pthread_self());
            break;

        case NET_WRITE:
            n = findFD(fd);

            printf("Thread : %lu received \"netwrite\"\n", pthread_self());
            break;

        case NET_CLOSE:
                                                                                            // Incoming message format is: 5,fd,0,0
            sscanf(buffer, "%u,%d", &netFunc, &fd); 
            n = deleteFD(fd);                                                             // Server Response : result, errno, h_errno, fdPtr
            if ( n == FAIL) {
                sprintf(buffer, "%d,%d,%d,%d", FAIL, errno, h_errno, n);
            } 
            else {
                sprintf(buffer, "%d,%d,%d,%d", SUCCESS, errno, h_errno, n);
            }

            break;

        case INVALID:
        default:
            printf("Thread : %lu received invalid net function\n", pthread_self());
            break;
    }

    n = write(*sockfd, buffer, strlen(buffer) );                                                // Send Server response back to client
    if ( n < 0 ) {
        fprintf(stderr,"Thread : %lu fails to write to socket\n", pthread_self());
    }
    
    if ( *sockfd != 0 ) close(*sockfd);
    pthread_exit( NULL );
}

int ex_netopen(fileDescriptor *newFd ){
    int n = -1;

    n = open(newFd->filePath, newFd->fileFlags);
    if ( n < 0 ) {
        // File open failed
        printf("Open File Failed\n");
        return FAIL;
    }

    close(n);
    printf("opened and closed \"%s\"\n", newFd->filePath);

    if (canOpen(newFd) == FALSE ) return FAIL;
    
    n = createFD(newFd);
    if (n == FAIL) {
        errno = ENFILE;
        return FAIL;
    } 

    return n;  
}

int canOpen(fileDescriptor *newFd ){
    int i = 0;

    switch (newFd->fMode) {
        case TRANSACTION:                                                       // Can only be opened if no fd is already assigned to this file for any reason
            for (i=0; i < FD_TABLE_SIZE; i++) {
                
                if (strcmp(FD_Table[i].filePath, newFd->filePath) == 0) return FALSE;
            }
            return TRUE;
            break;
 
        case EXCLUSIVE:                                                         // Can be opened as long as no file descriptor has been assigned O_WRONLY or O_RDWR
            for (i=0; i < FD_TABLE_SIZE; i++) {
                if (strcmp(FD_Table[i].filePath, newFd->filePath) == 0){
                    
                    if((FD_Table[i].fileFlags == O_WRONLY) ||(FD_Table[i].fileFlags ==O_RDWR)) return FALSE;
                }
            }
            return TRUE;
            break;
 
        case UNRESTRICTED:                                                      // Can be opened regardless
            return TRUE;
            break;

        default:
            errno = EINVAL;
            break;
    }

    return FALSE;
}

void initFDTable(){
    int i = 0;
    for (i=0; i < FD_TABLE_SIZE; i++) {
        FD_Table[i].fd = 0; 
        FD_Table[i].fMode = INVALID_FILE;        
        FD_Table[i].fileFlags = O_RDONLY;        
        FD_Table[i].filePath[0] = '\0';        
    }
}

int findFD( fileDescriptor *newFd ){
    int i = 0;
    for (i=0; i < FD_TABLE_SIZE; i++){
        if ((strcmp(FD_Table[i].filePath, newFd->filePath) == 0) && (FD_Table[i].fMode == newFd->fMode) && (FD_Table[i].fileFlags == newFd->fileFlags)){
            return i;
        }
    }
    return FAIL;
}

int createFD(fileDescriptor *newFd ){
    int i = 0;
    int n = -1;

    n = findFD(newFd);
    if ( n >= 0 ) {                                                             // findFD returns positive integer if file descriptor exists in table
        return FD_Table[n].fd;
    }

    for (i=0; i < FD_TABLE_SIZE; i++) {
        if ( FD_Table[i].fd == 0 ){
            FD_Table[i].fd = (-1 * (i));
            FD_Table[i].fMode = newFd->fMode;        
            FD_Table[i].fileFlags = newFd->fileFlags;        
            strcpy( FD_Table[i].filePath, newFd->filePath);
            return FD_Table[i].fd;  
         }
    }

    return FAIL;                                                                            // File descriptor table is full
}

int deleteFD(int fd){
    int i = 0;
    for (i=0; i < FD_TABLE_SIZE; i++) {
        if (FD_Table[i].fd == fd) {
            FD_Table[i].fd = 0;  
            FD_Table[i].fMode = INVALID_FILE;        
            FD_Table[i].fileFlags = O_RDONLY;        
            FD_Table[i].filePath[0] = '\0';     
            return fd;
         }
    }
    errno = EBADF;
    return FAIL;
}