#ifndef 	_LIBNETFILES_H_
#define    	_LIBNETFILES_H_

//
// Hard-coded port number
//
#define PORT  6006  
#define TRUE    1
#define FALSE   0
#define SUCCESS 0
#define FAIL -1


//
// Size of each message between the
// server and the client.
//
#define BUFFER_SIZE  256 


//
// Network file connection mode
//
typedef enum {
    UNRESTRICTED = 1,
    EXCLUSIVE   = 2,
    TRANSACTION = 3,
    INVALID_FILE = 99
} FILE_CONNECTION_MODE;



//
// Net server function types
//
typedef enum {
    NET_SERVERINIT = 1,
    NET_OPEN  = 2,
    NET_READ  = 3,
    NET_WRITE = 4,
    NET_CLOSE = 5,
    INVALID   = 99
} NET_FUNCTION_TYPE;







/////////////////////////////////////////////////////////////
//
// Function declarations 
//
/////////////////////////////////////////////////////////////

extern int netserverinit(char *hostname, int filemode);
extern int netopen(const char *pathname, int flags);
extern ssize_t netread(int fildes, void *buf, size_t nbyte); 
extern ssize_t netwrite(int fildes, const void *buf, size_t nbyte); 
extern int netclose(int fd);
int getSock(const char * hostname);



#endif    // _LIBNETFILES_H_

