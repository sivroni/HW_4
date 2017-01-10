#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 
#include <assert.h>

#define MAX 1024
#define RANDOM_FILE_PATH "/dev/random"

// TO DO LIST
// 1. Do we need to iterate on urandom file? can I asumme it is big enough?
// 2. init all variables at start

void main(int argc, char *argv[]){

	if ((argc > 5) || (argc < 2)){
		printf("Invalid number of arguments\n");
		exit(-1);
	}

	// init variables
	char * ptr; // for strtol function
	int returnVal; // return value to be check from various functions

	errno = 0;
	short PORT = strtol(argv[1], &ptr, 10);
	if (errno != 0){
		printf("Error converting PORT from string to short: %s\n", strerror(errno));
		exit(errno);
	}

	char * KEY = argv[2];
	long KEYLEN;
	int key_fd = 0; // key file
	int urandom_fd = 0; // urandom file
	char keyBuffer[MAX]; // buffer to create key file 
	int bytes_left_to_init = 0; // for while loop - creating key file
	int bytes_read_from_urandom = 0; // for creating key file
	int listen_fd = 0;

	if (argc == 4){ // user entered KEYLEN:
			errno = 0;
			KEYLEN = strtol(argv[3], &ptr, 10);
			if (errno != 0){
				printf("Error converting KEYLEN from string to long: %s\n", strerror(errno));
				exit(errno);
			}
			key_fd = open(KEY, O_RDWR | O_CREAT | O_TRUNC,0777 ); // opens/creates a key file 
			if (key_fd<0){ // check for error 
				printf("Error opening output file: %s\n", strerror(errno));
				exit(errno); 
			}

			urandom_fd = open(RANDOM_FILE_PATH , O_RDONLY);
			if (urandom_fd<0){ // check for error 
				printf("Error opening %s file: %s\n", RANDOM_FILE_PATH, strerror(errno));
				exit(errno); 
			}

			// iterate on /dev/urandom in order to create key file
			bytes_left_to_init = KEYLEN;
			while (bytes_left_to_init > 0){ // while there's something left to write
				bytes_read_from_urandom = read(urandom_fd, keyBuffer, bytes_left_to_init-1);
				if ( bytes_read_from_urandom < 0){
					printf("Error while reading from /dev/urandom: %s\n", strerror(errno));
					exit(errno);
				}
				keyBuffer[bytes_read_from_urandom] = 0; // end of buffer with '\0'
				bytes_written_to_key = write(key_fd, keyBuffer, bytes_read_from_urandom); // write what you read from server
				if ( bytes_written_to_out < 0){
						printf("Error while using writing to output file: %s\n", strerror(errno));
						exit(errno);
				}

				bytes_left_to_init = bytes_left_to_init - bytes_written_to_key; // update number of bytes left to write

			}

			// close opened files, we will open them again for each process / client
			close(key_fd);
			close(urandom_fd);
	}

		struct sockaddr_in serv_addr, my_addr, peer_addr;  
	    char sendBuff[MAX]; // MAX IN EXAMPLE WAS 1025

	    listen_fd = socket(AF_INET, SOCK_STREAM, 0); // a TCP socket
	    if (listen_fd <0){
	    	// AF_INET means ipv4, SOCK_STREAM means reliable, 
		    // 0 means that the operating system chooses TCP for sock_stream
		      printf("\n Error : Could not create socket \n");
		      exit(1);
		} 
	    
	    memset(&serv_addr, '0', sizeof(serv_addr)); // reset bits in serv_addr
		memset(sendBuff, '0', sizeof(sendBuff)); // reset bits in send buffer

		serv_addr.sin_family = AF_INET;
   		serv_addr.sin_addr.s_addr = htonl(PORT); // INADDR_ANY = any local machine address
		serv_addr.sin_port = htons(10000); // it doensnt matter whats in here - its the same computer
		if(bind(listen_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))){ // maps between a socket and an address
	       printf("\n Error : Bind Failed. %s \n", strerror(errno));
	       exit(errno);
	    }

	    if(listen(listen_fd, 10)){ // mark a socket as special - to accept incoming connections from
	       printf("\n Error : Listen Failed. %s \n", strerror(errno));
	       exit(errno);
		}

		



}