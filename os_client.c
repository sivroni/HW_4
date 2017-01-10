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
#include <pthread.h>

#define MAX 1024
// To do list:
// 1. init all variables at start
// 2.  what does Client - instruction 5 means? 
// 3. read comments carefully
void main(int argc, char *argv[]){

	if (argc != 5){
		printf("Invalid number of arguments\n");
		exit(-1);
	}

	// init variables
	char * ptr; // for strtol function
	int returnVal; // return value to be check from various functions

	char * IP = argv[1];

	errno = 0;
	short PORT = strtol(argv[2], &ptr, 10);
	if (errno != 0){
		printf("Error converting PORT from string to short: %s\n", strerror(errno));
		exit(errno);
	}

	char * IN = argv[3];

	// check if IN file exists 
   	int IN_fd = open(IN, O_RDWR); // check flag - maybe change to read only?
	
   	if( IN_fd < 0 ){
        	printf( "Error opening IN file : %s\n", strerror(errno) );
        	exit(errno); 
	} 

	char * OUT = argv[4];

	// create OUT file: 
	int OUT_fd = open(OUT, O_RDWR | O_CREAT | O_TRUNC,0777 ); // opens/creates an output file - maybe change to write only?
	if (fd_output <0){ // check for error 
			printf("Error opening output file: %s\n", strerror(errno));
			exit(errno); 
	}

	int sockfd = 0, bytes_read_from_server = 0, bytes_written_to_out = 0, bytes_read_from_in = 0, bytes_written_to_server = 0;
    char clientBuffer[MAX];
    struct sockaddr_in serv_addr;  // contain the address of the server to which we want to connect
    struct sockaddr_in my_addr, peer_addr;  // client struct
    socklen_t addrsize = sizeof(struct sockaddr_in );

    memset(clientBuffer, '0',sizeof(clientBuffer)); // clear buffer
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    // AF_INET means ipv4, SOCK_STREAM means reliable, 
    // 0 means that the operating system chooses TCP for sock_stream
        printf("\n Error : Could not create socket \n");
        exit(1);
	} 

	// returns the current address to which the socket sockfd is bound, in my_addr. The addrlen argument
    // should be initialized to indicate the amount of space (in bytes) pointed to by addr.
    if (getsockname(sockfd, (struct sockaddr*)&my_addr, &addrsize) != 0){
    	printf("Error in function getsockname(): %s\n", strerror(errno));
		exit(errno); 
    } 
    printf("Client: initially in my socket i am %s:%d)\n", 
            inet_ntoa((my_addr.sin_addr)),  ntohs(my_addr.sin_port));     // delete    

    memset(&serv_addr, '0', sizeof(serv_addr)); // clears the server buffer

    serv_addr.sin_family = AF_INET; //TCP
    serv_addr.sin_port = htons(PORT); // Note: htons for endiannes
    serv_addr.sin_addr.s_addr = inet_addr(IP); 

	printf("Client: connecting...\n"); // delete

	  // connect socket to the above address 
    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
       printf("\n Error : Connect Failed. %s \n", strerror(errno));
       exit(errno);
	} 

	/* print socket details again */
	// to delete
    if ( getsockname(sockfd, (struct sockaddr*)&my_addr, &addrsize) != 0){
	 printf("\n Error in getsockname(): %s \n", strerror(errno));
       exit(errno);
    }
    //returns the address of the peer connected to the socket sockfd, in the buffer pointed to by peer_addr
    if (getpeername(sockfd, (struct sockaddr*)&peer_addr, &addrsize) != 0){
     printf("\n Error in getsockname(): %s \n", strerror(errno));
       exit(errno);
    } 

    printf("connected! in my socket i am %s:%d (peer is %s:%d)\n", 
            inet_ntoa((my_addr.sin_addr)),  ntohs(my_addr.sin_port),
			inet_ntoa((peer_addr.sin_addr)), ntohs(peer_addr.sin_port));

    // send the data from IN file to the server
    while ( (bytes_read_from_in = read(IN_fd, clientBuffer, sizeof(clientBuffer)-1))> 0){ /*read until EOF */
		clientBuffer[bytes_read_from_in] = 0; // end of buffer with '\0'
		bytes_written_to_server = write(sockfd, clientBuffer, bytes_read_from_in); // write what you read from IN file
		if ( bytes_written_to_server < 0){
				printf("Error while writing to server through socket: %s\n", strerror(errno));
				exit(errno);
		}
	}

	if (bytes_read_from_in < 0){ // didnt reach EOF, its an error
		printf("Error while reading from IN file: %s\n", strerror(errno));
		exit(errno);
	
	}
    // read from server the ecrypted data + write the encrypted data to OUT file
     while ( (bytes_read_from_server = read(sockfd, clientBuffer, sizeof(clientBuffer)-1)) > 0) {
        clientBuffer[bytes_read_from_server] = 0; // end of buffer with '\0'
        bytes_written_to_out = write(OUT_fd, clientBuffer, bytes_read_from_server); // write what you read from server
		if ( bytes_written_to_out < 0){
				printf("Error while using writing to output file: %s\n", strerror(errno));
				exit(errno);
		}
	} 

    if(bytes_read_from_server < 0) {
        perror("\n Read error \n"); // whats perror? what about exit?
	} 

	// exit gracefully - close files & socket
    close(sockfd); // is socket really done here?
    close(IN_fd);
    close(OUT_fd);
	exit(0); 
}