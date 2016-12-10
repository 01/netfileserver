#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "libnetfiles.h"

typedef struct {
    char hostname[64];
    FILE_CONNECTION_MODE fMode;
} NET_SERVER;

NET_SERVER testServer;


int getSock( const char * hostname ){
    int sockfd = 0;

    struct sockaddr_in serv_addr;
    struct hostent *server = NULL;

/**********************************Create New Socket**********************************/   
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr,"libnetfiles: socket() failed, errno= %d\n", errno);
		return FAIL;
    }
  
   
    server = gethostbyname(hostname);													//Get address of server given inputed name
    if (server == NULL) {
       	errno = 0;
        h_errno = HOST_NOT_FOUND;
        printf("stderr,libnetfiles: host not found, h_errno");
		return FAIL;
    }

/**********************************Initialize Address**********************************/  
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
    (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(PORT);
 	if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
    {
        errno = 0;
        h_errno = HOST_NOT_FOUND;
        fprintf(stderr,"libnetfiles: cannot connect to %s, h_errno= %d\n", 
                hostname, h_errno);
		return FAIL;
    }

    return sockfd;
}

int netserverinit(char *hostname, int filemode){
    int rc = 0;
    int sockfd = -1;
    char buffer[BUFFER_SIZE] = "";

    errno = 0;																			// Reset errno and h_errno						
    h_errno = 0;


    strcpy(testServer.hostname, buffer);													// Reset Server Name
	testServer.fMode = INVALID_FILE;

    //
    // Verify the given file connection mode is valid
    //
    switch (filemode) {
        case UNRESTRICTED:
        case EXCLUSIVE:   
        case TRANSACTION:   
            break;

        default:
            h_errno = INVALID_FILE;
            fprintf(stderr, "netserverinit: invalid file connection mode\n");
            return FAIL;
            break;
    }


    
    if (hostname == NULL || strlen(hostname) == 0) {
        errno = EINVAL;  // 22 = Invalid argument
        return FAIL;
    };

    
/**********************************Get a socket to connect to file server************************************/
    
    sockfd = getSock(hostname);
    if (sockfd < 0 ) {
        errno = 0;
       	h_errno = HOST_NOT_FOUND;
        //fprintf(stderr, "netserverinit: host not found, %s\n", hostname);
        //fprintf(stderr, "netserverinit: errno= %d, %s\n", errno, strerror(errno));
        //fprintf(stderr, "netserverinit: h_errno= %d, %s\n", h_errno, strerror(h_errno));
        return -1;
    }
    
	// printf("netserverinit: sockfd= %d\n", sockfd);


   
/***********Compose my net command to send to the server.  The format is:   netCmd,0,0,0 **/
   
    bzero(buffer, BUFFER_SIZE);
    sprintf(buffer, "%d,0,0,0", 1);

    //printf("netserverinit: send to server - \"%s\"\n", msg);
    rc = write(sockfd, buffer, strlen(buffer));
    if ( rc < 0 ) {
        // Failed to write command to server
       	h_errno = ECOMM;  // 70 = Communication error on send
        //fprintf(stderr, "netserverinit: failed to write cmd to server.  rc= %d\n", rc);
        return FAIL;
    }


    // 
    // Read the net response coming back from the server. 
    // The response message format is:
    //
    //   result,0,0,0
    //
    bzero(buffer, BUFFER_SIZE);
    rc = read(sockfd, buffer, BUFFER_SIZE-1);
	//printf("Ends up here%d\n", rc);
    if ( rc < 0 ) {
        h_errno = ECOMM;  // 70 = Communication error on send
        //fprintf(stderr,"netserverinit: fails to read from socket\n");
        if ( sockfd != 0 ) close(sockfd);
        return FAIL;
    }

    close(sockfd);  																			// Don't need this socket anymore

    //
    // Received a response back from the server
    //
    //printf("netserverinit: received from server - \"%s\"\n", msg);


    // Decode the response from the server
    sscanf(buffer, "%d,", &rc);
    if ( rc == 0 ) {
        //
        // Save the hostname of the net server.  All subsequent
        // network function calls will go this this net server.
        //
        strcpy(testServer.hostname, hostname);

        testServer.fMode = (FILE_CONNECTION_MODE)filemode;

        //printf("netserverinit: netServerName= %s, connection mode= %d\n", 
        //         gNetServer.hostname, gNetServer.fcMode);
        //printf("netserverinit: server responded with SUCCESS\n");
    }

    return rc;
}


int netopen(const char *pathname, int flags)
{
    int netFd  = -1;
    int sockfd = -1;
    int rc     = 0;
    char buffer[BUFFER_SIZE] = "";

    errno = 0;
    h_errno = 0;

    if (pathname == NULL || strlen(pathname)<1) {
        //fprintf(stderr,"netopen: pathname is NULL\n");
        errno = EINVAL;  // 22 = Invalid argument
        return FAIL;
    } 
    printf("netopen: pathname= %s, flags= %d\n", pathname, flags);

  /* if ( isNetServerInitialized( NET_OPEN ) != TRUE ) {
        errno = EPERM;  // 1 = Operation not permitted
        return FAIL;
    }*/


    //
    // Get a socket to talk to my net file server
    //
    sockfd = getSock(testServer.hostname);
    if ( sockfd < 0 ) {
        // this error should not happen
        errno = 0;
        h_errno = HOST_NOT_FOUND;
        fprintf(stderr, "netopen: host not found, %s\n", testServer.hostname);
        return FAIL;
    }


    // 
    // Compose my net command to send to the server.  The format is:
    //
    //     netCmd,connectionMode,fileOpenFlags,pathname
    //
    bzero(buffer, BUFFER_SIZE);
    sprintf(buffer, "%d,%d,%d,%s", NET_OPEN, testServer.fMode, flags, pathname);


    //printf("netopen: send to server - \"%s\"\n", msg);
    rc = write(sockfd, buffer, strlen(buffer));
    if ( rc < 0 ) {
        // Failed to write command to server
        fprintf(stderr, "netopen: failed to write cmd to server.  rc= %d\n", rc);
        return FAIL;
    }


    // 
    // Read the net response coming back from the server.
    // The response msg format is:
    //
    //    result,errno,h_errno,netFd
    //
    bzero(buffer, BUFFER_SIZE);
    rc = read(sockfd, buffer, BUFFER_SIZE-1);
    if ( rc < 0 ) {
        if ( sockfd != 0 ) close(sockfd);
        return FAIL;
    }

    close(sockfd);  // Don't need this socket anymore

    //
    // Received a response back from the server
    //
    //printf("netopen: received from server - \"%s\"\n", msg);


    // Decode the response from the server
    sscanf(buffer, "%d,%d,%d,%d", &rc, &errno, &h_errno, &netFd);
    if ( rc == FAIL ) {
        //printf("netopen: server returns FAILURE, errno= %d (%s), h_errno=%d\n",
        //          errno, strerror(errno), h_errno);
        return FAIL;
    }

    return netFd;
}

int main(int argc, char *argv[])
{
    char *hostname = NULL;
    int  rc = -1;
    int  fd = -1;


    if (argc < 2) {
        fprintf(stderr, "Usage: %s hostname\n", argv[0]);
        exit(-1);
    }

    hostname = argv[1];
if(gethostbyname(hostname)==NULL) printf("What the fuck");
else printf("seems fine \n");

rc = netserverinit(hostname, EXCLUSIVE);
    if (rc == SUCCESS ) 
    {
        printf("test 01: PASSED: netserverinit \"%s\", exclusive, errno= %d, h_errno= %d\n",
                         hostname,errno,h_errno);
    } else {
        printf("test 01: FAILED: netserverinit \"%s\", exclusive, errno= %d (%s), h_errno= %d\n",
                         hostname,errno, strerror(errno), h_errno);
        exit(EXIT_FAILURE);
    };
    printf("---------------------------------------------------------------------------\n");
rc = netopen("./junk.txt", O_RDONLY);
    if ( rc == SUCCESS ) 
    {
        printf("test 02: PASSED:");
                         
    } else {
        printf("test 02: FAILED: netserverinit \"%s\"net, exclusive, errno= %d (%s), h_errno= %d\n",
                         hostname,errno, strerror(errno), h_errno);
        exit(EXIT_FAILURE);
    };
    printf("---------------------------------------------------------------------------\n");



return 0;
}
