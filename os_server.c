#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h> // for wait macros etc
#include <time.h> 
#include <assert.h>

#define MAX 1024
#define RANDOM_FILE_PATH "/dev/random"

// TO DO LIST
// 1. init all variables at start
// 2. see notes about hw1
// 3. Does the handler need to take care of the child processes?


// Headers:
void server_handler(int signal);

void server_handler(int signal){ // fill in the handler
	if (signal == SIGINT) {

		exit(EXIT_FAILURE);
	}
}

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
	char clientBuffer[MAX];
	int bytes_left_to_init = 0; // for while loop - creating key file
	int bytes_read_from_urandom = 0; // for creating key file
	int listen_fd = 0;
	struct sigaction sa; // to handle SIGINT
	int client_connection_fd = 0; // conntection to client
	pid_t process_id = 0; // check fork() return value
	int bytes_read_from_client = 0; // for child process
	//pid_t parent_process_id = getpid(); // return the parent / main process id
	//pid_t current_process_id = 0; // getpid() in while loop - to check if forked or not

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

				bytes_read_from_urandom = read(urandom_fd, keyBuffer, bytes_left_to_init);
				if ( bytes_read_from_urandom < 0){
					printf("Error while reading from /dev/urandom: %s\n", strerror(errno));
					exit(errno);
				}

				else{ // we need to close the urandom and open it agagin - reached EOF

					if( bytes_read_from_urandom == 0 ){ 
						close(urandom_fd);
						urandom_fd = open(RANDOM_FILE_PATH , O_RDONLY);
						if (urandom_fd < 0){ // check for error 
							printf("Error opening %s file: %s\n", RANDOM_FILE_PATH, strerror(errno));
							exit(errno); 
						}
						bytes_read_from_urandom = read(urandom_fd, keyBuffer, bytes_left_to_init);
						if ( bytes_read_from_urandom < 0){ // check for error
							printf("Error while reading from /dev/urandom: %s\n", strerror(errno));
							exit(errno);
						}

					}
				}

				bytes_written_to_key = write(key_fd, keyBuffer, bytes_read_from_urandom); // write what you read from urandom
				if ( bytes_written_to_key < 0){
						printf("Error while using writing to output file: %s\n", strerror(errno));
						exit(errno);
				}

				bytes_left_to_init = bytes_left_to_init - bytes_written_to_key; // update number of bytes left to write

			} // END OF WHILE LOOP - for creating key file (if KEYLEN was entered)

			// close opened files, we will open them again for each process / client
			close(key_fd);
			close(urandom_fd);
		} 

		struct sockaddr_in serv_addr, my_addr, peer_addr;  
	    char sendBuff[MAX]; // MAX IN EXAMPLE WAS 1025

	    listen_fd = socket(AF_INET, SOCK_STREAM, 0); // a TCP socket
	    if (listen_fd < 0){
	    	// AF_INET means ipv4, SOCK_STREAM means reliable, 
		    // 0 means that the operating system chooses TCP for sock_stream
		      printf("\n Error : Could not create socket \n");
		      exit(errno);
		} 
	    
	    memset(&serv_addr, '0', sizeof(serv_addr)); // reset bits in serv_addr
		memset(sendBuff, '0', sizeof(sendBuff)); // reset bits in send buffer

		serv_addr.sin_family = AF_INET;
   		serv_addr.sin_addr.s_addr = htonl(PORT); // INADDR_ANY = any local machine address
		serv_addr.sin_port = htons(10000); // it doensnt matter whats in here - its the same computer

		if( bind(listen_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))){ // maps between a socket and an address
	       printf("\n Error : Bind Failed. %s \n", strerror(errno));
	       exit(errno);
	    }

	    if(listen(listen_fd, 10)){ // mark a socket as special - to accept incoming connections from
	       printf("\n Error : Listen Failed. %s \n", strerror(errno));
	       exit(errno);
		}

		// handle SIGINT
		sa.sa_handler = server_handler;
		if (sigaction(SIGINT, &sa, NULL) == -1) {
				printf("Error while defining SIGINT: %s\n", strerror(errno));
				exit(errno);
		}

		while (1){
			// accepting connection
			client_connection_fd = accept(listen_fd, NULL, NULL);

        	if(client_connection_fd < 0){ // error in connection
           		printf("\n Error : Accept Failed. %s \n", strerror(errno));
           		exit(errno); 
			}

			// fork a new process
			process_id = fork();
			if (process_id < 0){ //error forking
				printf("Error while defining SIGINT: %s\n", strerror(errno));
				exit(errno);
				
			}

			if (process_id == 0){ // handle child

				key_fd = open(KEY, O_RDWR | O_CREAT | O_TRUNC,0777 ); // opens key file 
				if (key_fd<0){ // check for error 
					printf("Error opening output file: %s\n", strerror(errno));
					exit(errno); 
				}

				// loop: read -- encrypt -- send result to client
				while ( (bytes_read_from_client = read(client_connection_fd, clientBuffer, MAX -1)) >0){ // while didn't reach to EOF

						
						bytes_written_to_key = write(key_fd, keyBuffer, bytes_read_from_urandom); // write what you read from urandom
						if ( bytes_written_to_key < 0){
								printf("Error while using writing to output file: %s\n", strerror(errno));
								exit(errno);
						}

						
				}

				if (bytes_read_from_client < 0){ // error reading from client
					printf("Error reading from client socket: %s\n", strerror(errno));
					exit(errno); 
				}

				// exit gracefully
				close(client_connection_fd); // close when finish handling the client
				close(key_fd);
				exit(0);

			}
			 // if not entered to  if (process == 0) then we continue to iterate to get next connection



			//close(client_connection_fd); // close when finish handling the client
		}


}