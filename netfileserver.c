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
#define QUEUE_TABLE_SIZE 100

/*

#define QUEUE_SIZE 5


char queue[QUEUE_SIZE][BUFFER_SIZE];
int front = 0;
int rear = -1;
int numElements = 0;

int isEmpty(){

	if(numElements == 0){
		return 1;
	}
	else{
		return 0;
	}
}

int isFull(){

	if(numElements == QUEUE_SIZE){
		return 1;
	}
	else{
		return 0;
	}
}

void enqueue(char* element){

	if(isFull()){
		printf("Queue is full.\n");
	}
	else{
		if(rear == QUEUE_SIZE-1){
			rear = -1;
		}
		
		strcpy(queue[++rear], element);
		numElements++;
	}

}

char* dequeue(){

	if(isEmpty()){
		printf("Queue is empty.\n");
	}
	else{
		char data[BUFFER_SIZE];
		strcpy(arr, queue[front++];
	
		if(front == QUEUE_SIZE){
			front = 0;
		}
	
		numElements--;
		return data;
	}
}

*/


typedef struct{
	char filePath[256];
	int ticketNumber;
	int front;
} queue;

queue queue_Table[QUEUE_TABLE_SIZE];
int numQueues = 0;


static int  terminate = FALSE;
void *workerThread( void *newSocket_FD );

/********Data structure of a file descriptor************/

typedef struct {
    int  netFD;
    int localFD;                                                                                        // File descriptor int (must be negative)
    CONNECTION_MODE fMode;                                                                          // Connection Mode
    int fileFlags;                                                                                  // File Flags
    char filePath[256];                                                                             // File Path 
} fileDescriptor;


fileDescriptor FD_Table[FD_TABLE_SIZE];

void initFDTable();
//int findFD( fileDescriptor *fdPtr );
int createFD( fileDescriptor *fdPtr );
int deleteFD( int fd );
int canOpen(fileDescriptor *fdPtr );
void *workerThread( void *newSocket_FD );
int ex_netopen( fileDescriptor *fdPtr );
int ex_netread(int fd, ssize_t nbyte, char * readBuffer);
int ex_netwrite(int fd, char * readBuffer, ssize_t nbyte);

int main(int argc, char *argv[])
{
    int sockfd = 0;
    int newsockfd = 0;
    int n;

    struct sockaddr_in serv_addr, cli_addr;
    int clilen = sizeof(cli_addr);
    pthread_t    Worker_thread_ID = 0;
    initFDTable();
    char buffer[5000] = "";
 	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr,"netfileserver: socket() failed, errno= %d\n", errno);
        exit(EXIT_FAILURE);
    }

	bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(atoi(argv[1]));
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

void *workerThread(void *newSocket_FD){
    int n = 0;
    int fd = -1;
    int *sockfd = newSocket_FD;
    fileDescriptor  *newFd = NULL;

    char buffer[BUFFER_SIZE] = "";
    NET_FUNCTION_TYPE netFunc = INVALID;

    n = pthread_detach(pthread_self());
    n = read(*sockfd, buffer, 500 -1);

    if ( n < 0 ) {
        fprintf(stderr,"Thread: %lu failed to read from socket\n", pthread_self());
        if ( *sockfd != 0 ) close(*sockfd);
		pthread_exit(NULL);
    }
    else {
        printf("Thread: %lu received \"%s\"\n", pthread_self(), buffer);
    }

/*************Decode the incoming message. Find Out What Operation to Do ******************/
    int * nbyte = malloc(sizeof(int));
    char readBuffer [500];
    sscanf(buffer, "%u,", &netFunc);
   
    switch (netFunc){

/**********************************************************************************************************************************************************
**                                                              NET_SERVERINIT                                                                           **
** netserverinit command from client client to make connection to this server. The Server will respond with SUCCESS (1) No Errors From Server Side       **
**********************************************************************************************************************************************************/        

        case NET_SERVERINIT:
            sprintf(buffer, "%d,0,0,0", SUCCESS);
            printf("Thread : %lu responding with \"%s\"\n", pthread_self(), buffer);
            break;


/**********************************************************************************************************************************************************
**                                                              NET_OPEN                                                                                 **
** Attempts to Open a File on the Server- Returns to Client a file descriptor or -1 if an error occurred (in which case errno is set and given to client)**
** REQUIRED ERRORS : EACCES(PERMISSION DENIED), EINTR(INTERUPTED FUNCTION CALL), EISDIR(IS A DIRECTORY), ENOENT(NO SUCH FILE), EROFS(READ ONLY FILE)     **
** OPTIONAL ERRORS : ENFILE (To Many FIles Open), EWOULDBLOCK (Operation Would Block), EPERM (Operation Not Permited)                                    **
**********************************************************************************************************************************************************/
        case NET_OPEN:
            printf("Thread : %lu received \"netopen\"\n", pthread_self());
                                                                                            // Incoming message format is:  2,connectionMode,fileFlags,filePath
            newFd = malloc(sizeof(fileDescriptor));

            sscanf(buffer, "%u,%d,%d,%s", &netFunc, (int *)&(newFd->fMode), &(newFd->fileFlags), newFd->filePath);

	    

	    // operation cannot currently be serviced, so it enters a queue
	    if (canOpen(newFd) == FALSE ){

		int queuePosition;
		int queueExists = FALSE;
		int i;
		for(i=0; i<numQueues; i++){

			if( strcmp(newFd->filePath, queue_Table[i].filePath) == 0 ){
				queuePosition = queue_Table[i].ticketNumber;
				queue_Table[i].ticketNumber++;
				queueExists = TRUE;
				break;
			}
		}
		// operation enters existing queue for file
		if(queueExists){
			
			while(TRUE){
				
				sleep(2);

				if( queuePosition == queue_Table[i].front && canOpen(newFd) ){
					queue_Table[i].front++;
					break;
				}
			}
		}
		// operation enters new queue for file
		else{

			strcpy(queue_Table[numQueues].filePath, newFd->filePath);
			queue_Table[numQueues].ticketNumber = 2;
			queue_Table[numQueues].front = 1;
			int temp = numQueues;
			numQueues++;

			while(TRUE){
				
				sleep(2);

				if( canOpen(newFd) ){
					queue_Table[temp].front++;
					break;
				}
			}
		}

	    }

            n = ex_netopen(newFd);
            printf("Thread : %lu ex_netopen returns fd %d\n", pthread_self(), n);
                                                                                            // Compose a response message.  The format is: result,errno,h_errno,fdPtr
            if (n == FAIL) {
                sprintf(buffer, "%d,%d,%d,%d", FAIL, n, errno, h_errno);
            } 
            else {
                sprintf(buffer, "%d,%d,%d,%d", SUCCESS, n, errno, h_errno);
            }
            printf("Thread : %lu responding with \"%s\"\n", pthread_self(), buffer);
            free(newFd);
            break;

/**********************************************************************************************************************************************************
**                                                              NET_READ                                                                                 **
** Upon recieving command from client. Server attempts to read the number of bytes passed by the clients given a file descriptor resuts the number of    **
** bytes actually read, or returns -1 if failed and sets errno to indicate Errors                                                                        **
** ERRORS : ETIMEDOUT, EBADF, ECONNRESET                                                                                                                 **
**********************************************************************************************************************************************************/
        case NET_READ:
            printf("Thread : %lu received \"netread\"\n", pthread_self());

            sscanf(buffer, "%u,%d, %d", &netFunc, &fd, nbyte);
            printf("%s\n", buffer);
            
            n = ex_netread(fd, *nbyte, readBuffer);
            printf("Makes it here\n");
            if(n==FAIL){
                sprintf(buffer, "%d,%d,%d,%d", FAIL, errno, h_errno, n);
            }
            else{
                readBuffer[n]='\0';
                 sprintf(buffer, "%d,%d,%d,%s", SUCCESS, n, errno, readBuffer);
            }
            break;

/**********************************************************************************************************************************************************
**                                                              NET_WRITE                                                                                **
** Upon recieving command from client. Server attempts to write the number of bytes passed by the clients given a file descriptor resuts the number of   **
** bytes actually read, or returns -1 if failed and sets errno to indicate Errors                                                                        **
** ERRORS : ETIMEDOUT, EBADF, ECONNRESET                                                                                                                 **
**********************************************************************************************************************************************************/
        case NET_WRITE: 
            printf("Thread : %lu received \"netwrite\"\n", pthread_self());
            sscanf(buffer, "%u,%d, %d", &netFunc, &fd, nbyte);
            n = ex_netwrite(fd, readBuffer, *nbyte);
            if(n==FAIL){
                sprintf(buffer, "%d,%d,%d", FAIL, errno, h_errno);
                printf("ex_netwrite Failed with %d\n",n);
            }
            else{
                 sprintf(buffer, "%d,%d,%d", SUCCESS, n, errno);
                  printf("ex_netwrite succeeded with %d\n",n);
            }
            break;
/**********************************************************************************************************************************************************
**                                                              NET_CLOSE                                                                                **
** Upon recieving command from client. Server attempts to close the File of the file descriptor given by client. Sends SUCCESS if succeed else Sends     **
** failure along with errno and h_errno                                                                                                                  **
** ERRORS : ERRORS, EBADF                                                                                                               **
**********************************************************************************************************************************************************/
        case NET_CLOSE:
                                                                                            // Incoming message format is: 5,fd,0,0
            sscanf(buffer, "%u,%d", &netFunc, &fd);
            printf("FD FOR CLOSE : %d\n", fd); 
            n = deleteFD(fd);                                                             // Server Response : result, errno, h_errno, fdPtr
            if (n == FAIL) {
                sprintf(buffer, "%d,%d,%d,%d", FAIL, errno, h_errno, n);
            } 
            else {
                sprintf(buffer, "%d,%d,%d,%d", SUCCESS, errno, h_errno, n);
            }

            break;

        case INVALID:
            break;

        default:
            printf("Thread : %lu received invalid net function\n", pthread_self());
            break;

}    n = write(*sockfd, buffer, (strlen(buffer)+1) );                                                // Send Server response back to client
    if ( n < 0 ) {
        fprintf(stderr,"Thread : %lu fails to write to socket\n", pthread_self());
    }
    
    //if ( *sockfd != 0) close(*sockfd);
    pthread_exit(NULL);

}
int ex_netopen(fileDescriptor *newFd ){
    int n = -1;
    if (canOpen(newFd) == FALSE ){
        return FAIL;
    }
    newFd->localFD = open(newFd->filePath, newFd->fileFlags);
    if(newFd->localFD <0) return FAIL;
    n = createFD(newFd);
    if (n == FAIL) {
        errno = ENFILE;
        return FAIL;
    } 

    return n;  
}


int canOpen(fileDescriptor *newFd ){
    int i = 0;
    for (i=0; i < FD_TABLE_SIZE; i++) {
        if (strcmp(FD_Table[i].filePath, newFd->filePath) == 0){
            if(FD_Table[i].fMode == TRANSACTION) return FALSE;
            switch (newFd->fileFlags) {
 
                case O_RDONLY:                                                      // If client wants to read, as long as file isnt opened in transcation mode, it can open                                             
                break;

                case O_WRONLY:                                                         // Can be opened as long as no file descriptor has been assigned O_WRONLY or O_RDWR
                if(FD_Table[i].fMode==EXCLUSIVE && (FD_Table[i].fileFlags == O_WRONLY || FD_Table[i].fileFlags == O_RDWR)) return FALSE;
                break;

                case O_RDWR:                                                         // Can be opened as long as no file descriptor has been assigned O_WRONLY or O_RDWR
                if(FD_Table[i].fMode==EXCLUSIVE && (FD_Table[i].fileFlags == O_WRONLY || FD_Table[i].fileFlags == O_RDWR)) return FALSE;
                break;

                default:
                errno = EINVAL;
                break;
            }
        }
    }
    return TRUE;
}


void initFDTable(){
    int i = 0;
    for (i=0; i < FD_TABLE_SIZE; i++) {
        FD_Table[i].localFD = 0;
        FD_Table[i].netFD = 0; 
        FD_Table[i].fMode = INVALID_FILE;        
        FD_Table[i].fileFlags = O_RDONLY;        
        FD_Table[i].filePath[0] = '\0';        
    }
}


int createFD(fileDescriptor *newFd ){
    int i = 0;
    int n = -1;

    for (i=0; i < FD_TABLE_SIZE; i++) {
        if (FD_Table[i].localFD == 0 ){
            FD_Table[i].localFD = newFd->localFD;
            FD_Table[i].netFD = (-5 * (i+1));
            FD_Table[i].fMode = newFd->fMode;        
            FD_Table[i].fileFlags = newFd->fileFlags;        
            strcpy(FD_Table[i].filePath, newFd->filePath);
            return FD_Table[i].netFD;  
         }
    }
    return FAIL;                                                                            // File descriptor table is full
}

int deleteFD(int netFD){
      int i=(netFD/-5)-1;
    if(i<FD_TABLE_SIZE && i>=0){
        FD_Table[i].localFD = 0;
        FD_Table[i].netFD = 0;  
        FD_Table[i].fMode = INVALID_FILE;        
        FD_Table[i].fileFlags = O_RDONLY;        
        FD_Table[i].filePath[0] = '\0';     
        return SUCCESS;
    }
    
    errno = EBADF;
    return FAIL;
}

int ex_netread(int fd, ssize_t nbyte, char *readBuffer){
    int n = -1;
   int i=(fd/-5)-1;
   printf("Makes it here netread\n");
    printf("LocalFD: %d\n", FD_Table[i].localFD);
    if(i<FD_TABLE_SIZE && i>=0){
        
        n = read(FD_Table[i].localFD, readBuffer, (ssize_t)nbyte);
        printf("return of ex_netread %d\n", n);
        if(n>=0) {
         return n;
         printf("Makes it past read\n");
        }
    }
    
    errno = EBADF;
    return FAIL;

}

int ex_netwrite(int fd, char * readBuffer, ssize_t nbyte){
    int n = -1;
   int i=(fd/-5)-1;
   printf("Makes it here netwrite\n");
    printf("LocalFD: %d\n", FD_Table[i].localFD);
    if(i<FD_TABLE_SIZE && i>=0){
        if(FD_Table[i].fileFlags==O_RDONLY){
            errno = EBADF;
            return FAIL;
        }
        n = write(FD_Table[i].localFD, readBuffer,(ssize_t)nbyte);
        printf("return of ex_netwrite%d\n", n);
        if(n>=0) {
         return n;
         printf("Makes it past read\n");
        }
    }
    
    errno = EBADF;
    return FAIL;

}

