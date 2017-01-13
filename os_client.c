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
#include <sys/stat.h>
#include <fcntl.h>

#define MAX 1024
// To do list:
// 1. init all variables at start
// 2. what does Client - instruction 5 means? What to do with instruction 2? 
// 3. read comments carefully
// 4. clientBuffer[bytes_read_from_in] = 0; // end of buffer with '\0' == DOUBLE CHECK IT !
// 5. Do we need to use perror?
// 6. Client before server? 

void main(int argc, char *argv[]){

	if (argc != 5){
		printf("Invalid number of arguments\n");
		exit(-1);
	}

	// init variables
	char * ptr; // for strtol function
	int returnVal; // return value to be check from various functions
	char * IP;
	short PORT;
	char * IN; // IN path
	char * OUT; // OUT path
	int IN_fd; // file descriptor for IN file
	int OUT_fd; // file descriptor for OUT file

	int sockfd = 0;
	int bytes_read_from_server = 0;
	int bytes_written_to_out = 0;
	int bytes_read_from_in = 0;
	int bytes_written_to_server = 0;
	int total_bytes_written_to_server = 0;
	int total_bytes_written_to_out = 0;
    char clientBuffer[MAX];
    struct sockaddr_in serv_addr;  // contain the address of the server to which we want to connect

    socklen_t addrsize = sizeof(struct sockaddr_in );

    // define the program arguments:
	IP = argv[1];

	errno = 0;
	PORT = strtol(argv[2], &ptr, 10);
	if (errno != 0){
		printf("Error converting PORT from string to short: %s\n", strerror(errno));
		exit(errno);
	}
	printf("The port that was entered is:%hi\n", PORT);

	IN = argv[3];

	// check if IN file exists 
   	IN_fd = open(IN, O_RDONLY); // open IN file
	
   	if( IN_fd < 0 ){
        	printf( "Error opening IN file : %s\n", strerror(errno) );
        	exit(errno); 
	} 

	OUT = argv[4];

	// create OUT file: 
	OUT_fd = open(OUT, O_WRONLY | O_CREAT | O_TRUNC,0777 ); // opens/creates an output file 
	if (OUT_fd < 0){ // check for error 
			printf("Error opening output file: %s\n", strerror(errno));
			exit(errno); 
	}

	

    memset(clientBuffer, '0',sizeof(clientBuffer)); // clear client buffer

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    // AF_INET means ipv4, SOCK_STREAM means reliable, 
    // 0 means that the operating system chooses TCP for sock_stream
        printf("\n Error : Could not create socket \n");
        exit(-1);
	} 

    memset(&serv_addr, '0', sizeof(serv_addr)); // clears the server

    // define server socket:
    serv_addr.sin_family = AF_INET; //TCP
    serv_addr.sin_port = htons(PORT); // Note: htons for endiannes
    serv_addr.sin_addr.s_addr = inet_addr(IP); 

	printf("Client: connecting...\n"); // delete

	  // connect socket to the above address 
    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
       printf("\n Error : Connect Failed. %s \n", strerror(errno));
       exit(errno);
	} 


    // send the data from IN file to the server

	while (1){ // read until we have nothing to read from IN file

			bytes_read_from_in = read(IN_fd, clientBuffer, MAX); // try reading from IN

			if (bytes_read_from_in < 0){ // error reading from client
				printf("Error reading from IN file: %s\n", strerror(errno));
				exit(errno); 
			}

			else if (bytes_read_from_in == 0){ // finish reading - THIS WILL END THE WHILE LOOP
				break;
			}
			
			total_bytes_written_to_server = 0; // sum the bytes we write - make sure we wrote everything
			while (total_bytes_written_to_server < bytes_read_from_in) {
				
				bytes_written_to_server = write(sockfd, clientBuffer + total_bytes_written_to_server, bytes_read_from_in - total_bytes_written_to_server);
				if (bytes_written_to_server < 0) {
					printf("error write() to server : %s\n", strerror(errno));
					exit(errno);
				}

				// increment our counter
				total_bytes_written_to_server = total_bytes_written_to_server + bytes_written_to_server;

			} // finished writing everything to server
	
	
		} // nothing else to read from IN file
	
	/*while ( (bytes_read_from_in = read(IN_fd, clientBuffer, sizeof(clientBuffer)-1))> 0){ 
		clientBuffer[bytes_read_from_in] = 0; // end of buffer with '\0' == DOUBLE CHECK IT !
		bytes_written_to_server = write(sockfd, clientBuffer, bytes_read_from_in); // write what you read from IN file
		if ( bytes_written_to_server < 0){
				printf("Error while writing to server through socket: %s\n", strerror(errno));
				exit(errno);
		}
	}

	if (bytes_read_from_in < 0){ // didnt reach EOF, its an error
		printf("Error while reading from IN file: %s\n", strerror(errno));
		exit(errno);
	
	}*/

	// read from server the ecrypted data + write the encrypted data to OUT file

		while (1){ // read until we have nothing to read from server

			bytes_read_from_server = read(sockfd, clientBuffer, MAX); // try reading from IN

			if (bytes_read_from_server < 0){ // error reading from client
				printf("Error reading from server: %s\n", strerror(errno));
				exit(errno); 
			}

			else if (bytes_read_from_server == 0){ // finish reading - THIS WILL END THE WHILE LOOP
				break;
			}
			
			total_bytes_written_to_out = 0; // sum the bytes we write - make sure we wrote everything
			while (total_bytes_written_to_out < bytes_read_from_server) {
				
				bytes_written_to_out = write(sockfd, clientBuffer + total_bytes_written_to_out, bytes_read_from_server - total_bytes_written_to_out);
				if (bytes_written_to_out < 0) {
					printf("error write() to server : %s\n", strerror(errno));
					exit(errno);
				}

				// increment our counter
				total_bytes_written_to_out = total_bytes_written_to_out + bytes_written_to_out;

			} // finished writing everything to server
	
	
		} // nothing else to read from IN file

	
	 // read from server the ecrypted data + write the encrypted data to OUT file
		/*
	while ( (bytes_read_from_in = read(IN_fd, clientBuffer, sizeof(clientBuffer)-1))> 0){ 
		clientBuffer[bytes_read_from_in] = 0; // end of buffer with '\0' == DOUBLE CHECK IT !
		bytes_written_to_server = write(sockfd, clientBuffer, bytes_read_from_in); // write what you read from IN file
		if ( bytes_written_to_server < 0){
				printf("Error while writing to server through socket: %s\n", strerror(errno));
				exit(errno);
		}
	}

	if (bytes_read_from_in < 0){ // didnt reach EOF, its an error
		printf("Error while reading from IN file: %s\n", strerror(errno));
		exit(errno);
	
	}*/

	// exit gracefully - close files & socket
    close(sockfd); 
    close(IN_fd);
    close(OUT_fd);
	exit(0); 
}