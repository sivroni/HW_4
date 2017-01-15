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
// 1. DONE >> init all variables at start
// 2. what does Client - instruction 5 means? What to do with instruction 2? 
// 3. DONE >> read comments carefully
// 4. DONE >> clientBuffer[bytes_read_from_in] = 0; // end of buffer with '\0' == DOUBLE CHECK IT !
// 5. DONE >> Do we need to use perror? see moodle
// 6. DONE >> Client opens before server? 
// 7. DONE >> Add loop in reading from socket
// 8. DONE >> clear printf + comments when not needed

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
	int total_bytes_read_from_server = 0;
    char clientBuffer[MAX] = {0};
    char sendBuffer[MAX] = {0};
    struct sockaddr_in serv_addr = {0};  // contain the address of the server to which we want to connect

    socklen_t addrsize = sizeof(struct sockaddr_in );

    // define the program arguments:
	IP = argv[1]; // where to set it? now changed field of inet_addr to hardcoded

	errno = 0;
	PORT = strtol(argv[2], &ptr, 10);
	if (errno != 0){
		printf("Error converting PORT from string to short: %s\n", strerror(errno));
		exit(errno);
	}

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


    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    // AF_INET means ipv4, SOCK_STREAM means reliable, 
    // 0 means that the operating system chooses TCP for sock_stream
        printf("\n Error : Could not create socket \n");
        exit(-1);
	} 

    // define server socket:
    serv_addr.sin_family = AF_INET; //TCP
    serv_addr.sin_port = htons(PORT); // Note: htons for endiannes
    serv_addr.sin_addr.s_addr = inet_addr(IP); 

	  // connect socket to the above address 
    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
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
				else{
					total_bytes_written_to_server = total_bytes_written_to_server + bytes_written_to_server;
				}

			} // finished writing to server the current buffer

			// now read encrypted data from server

			total_bytes_read_from_server = 0;
			while (total_bytes_read_from_server < bytes_read_from_in){
				bytes_read_from_server = read(sockfd, sendBuffer + total_bytes_read_from_server, bytes_read_from_in - total_bytes_read_from_server);
				
				if (bytes_read_from_server < 0){ // error reading from client
					printf("Error reading from server: %s\n", strerror(errno));
					exit(errno); 
				}

				else { // the reading was succesful!
								total_bytes_read_from_server = total_bytes_read_from_server + bytes_read_from_server;
				}

			}
			
			// write encryped data to out file
			total_bytes_written_to_out = 0; // sum the bytes we write - make sure we wrote everything
			while (total_bytes_written_to_out < bytes_read_from_server) {
				
				bytes_written_to_out = write(OUT_fd, sendBuffer + total_bytes_written_to_out, bytes_read_from_server - total_bytes_written_to_out);
				if (bytes_written_to_out < 0) {
					printf("error write() to server : %s\n", strerror(errno));
					exit(errno);
				}

				// increment our counter
				total_bytes_written_to_out = total_bytes_written_to_out + bytes_written_to_out;

			} // finished writing to server
	
	
		} // nothing else to read from IN file
	
	
		 

	//printf("finished reading from IN file\n");

	// exit gracefully - close files & socket
	//printf("Exiting from client\n");
    close(sockfd); 
    close(IN_fd);
    close(OUT_fd);
	exit(0); 
}