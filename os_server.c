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
#include <sys/stat.h>
#include <fcntl.h>

#define MAX 1024
#define RANDOM_FILE_PATH "/dev/random"

// TO DO LIST
// 1. init all variables at start
// 2. DONE >> see official solution for hw1
// 3. DONE >> Do we need to close thing in errors? check in solution hw1 maybe...
// 4. changes - is keylen not provided - dont use open! see moodle
// 5. what if cntl+c in client?
// 6. exit and close file properly in the init (before connection in main server)


// Headers:
void server_handler(int signal);

// Global variables:
int exit_flag = 0; // if flag == 1 then exit gracefully from all child processes

void server_handler(int signal){ 
	// terminal: to check after/during program:  ps -A | grep os_
	// zombie - <defunct> child finished
	// check if errors are interrupts and if they are - exit - EINTR stopped because signal - then: exit gracefully
	// in Blocking funcs: read, write, accept...

	if (signal == SIGINT) { // flag
		printf("\n SIGINT wad entered ...\n");
		exit_flag = 1; // raised flag - child process should exit gracefully
		printf("\n raised flag ...\n");
	//	if (kill(0, SIGINT) != 0){
	//			printf("Error killing all processes: %s\n", strerror(errno));
				exit(errno);
	//	} // should kill all processes in its group - including children

		//printf("\n Exiting gracefully...\n");
		//exit(EXIT_FAILURE);
	}
}

void main(int argc, char *argv[]){

	if ((argc > 5) || (argc < 2)){
		printf("Invalid number of arguments\n");
		exit(-1);
	}

	// init variables
	char * ptr; // for strtol function
	int key_fd = 0; // key file
	int urandom_fd = 0; // urandom file
	char keyBuffer[MAX]; // buffer to create key file 
	char clientBuffer[MAX]; // buffer to send to client
	int bytes_left_to_init = 0; // for while loop - creating key file
	int bytes_read_from_urandom = 0; // for creating key file
	int listen_fd = 0; // listen fd (in listen() function)
	struct sigaction sa; // to handle SIGINT
	int client_connection_fd = 0; // connection to client
	int process_id = 0; // check fork() return value
	int bytes_read_from_client = 0; // for child process - read data from client
	int bytes_read_from_key = 0; // to check if key file is empty (if KEYLEN wasnt entered) and for XOR
	int total_bytes_written_to_client = 0; // sum the number of bytes written to client
	int total_bytes_read_from_key = 0; // sum of number of bytes read from key
	int bytes_written_to_client = 0; //  return value for each write() call
	int bytes_written_to_key = 0; // return value for each write() call
	int i=0; // iteration index
	struct sockaddr_in serv_addr; // a TCP socket
	int j=0;


	char * KEY;
	short PORT;
	long KEYLEN; // argc[4], may not exist

	errno = 0;
	PORT = strtol(argv[1], &ptr, 10);
	if (errno != 0){
		printf("Error converting PORT from string to short: %s\n", strerror(errno));
		exit(errno);
	}

	KEY = argv[2];

	if (argc == 4){ // user entered KEYLEN:
			errno = 0;
			KEYLEN = strtol(argv[3], &ptr, 10);
			if (KEYLEN != 0){
				printf("Error converting KEYLEN from string to long: %s\n", strerror(errno));
				exit(errno);
			}

			key_fd = open(KEY, O_RDWR | O_CREAT | O_TRUNC,0777 ); // opens/creates a key file 
			if (key_fd < 0){ // check for error 
				printf("Error opening key file: %s\n", strerror(errno));
				exit(errno); 
			}

			urandom_fd = open(RANDOM_FILE_PATH , O_RDONLY);
			if (urandom_fd < 0){ // check for error 
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

				else{ // we need to close the urandom and open it again - reached EOF

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

				 // now we can write what we read from urandom in key file
				bytes_written_to_key = write(key_fd, keyBuffer, bytes_read_from_urandom); // write what you read from urandom
				if ( bytes_written_to_key < 0){
							printf("Error while using writing to output file: %s\n", strerror(errno));
							exit(errno);
					}

				bytes_left_to_init = bytes_left_to_init - bytes_written_to_key; // update number of bytes left to write
				


			} // END OF WHILE LOOP - for creating key file (if KEYLEN was entered)
			
			printf("Finished initializing the key file using urandom!\n");
			// close opened files, we will open the key file again for each process / client
			close(key_fd);
			close(urandom_fd);
		}

		else{ // if KEYLEN is not provided - need to check that key file is not empty and that it exists

			key_fd = open(KEY, O_RDONLY ,0777 ); // opens a key file 
			if (key_fd < 0){ // check for error 
				printf("Error opening key file: %s\n", strerror(errno));
				exit(errno); 
			}

			bytes_read_from_key = read(key_fd, keyBuffer, MAX);
			if ( bytes_read_from_key < 0){
					printf("Error while reading from key file: %s\n", strerror(errno));
					exit(errno);
			}

			else if (bytes_read_from_key == 0){ // key file is empty
					printf("Error - key file is empty: %s\n", strerror(errno));
					exit(errno);
			}
			
			close(key_fd);

		}

	    listen_fd = socket(AF_INET, SOCK_STREAM, 0); // a TCP socket
	    // AF_INET means ipv4, SOCK_STREAM means reliable, 
		// 0 means that the operating system chooses TCP for sock_stream
	    if (listen_fd < 0){
		      printf("\n Error : Could not create socket \n");
		      exit(errno);
		} 
	    
	    memset(&serv_addr, '0', sizeof(serv_addr)); // reset bits in serv_addr
		memset(clientBuffer, '0', sizeof(clientBuffer)); // reset bits in client buffer
		memset(keyBuffer, '0', sizeof(keyBuffer)); // reset bits in key buffer
	
		

		serv_addr.sin_family = AF_INET;
   		serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY = any local machine address
		serv_addr.sin_port = htons(PORT); // the argument PORT

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

				key_fd = open(KEY, O_RDONLY,0777 ); // opens key file
				if (key_fd < 0){ // check for error 
					printf("Error opening output file: %s\n", strerror(errno));
					exit(errno); 
				}

				printf("Process ID is: %d\n",getpid());
				
				
				// loop: read -- encrypt -- send result to client
				while (1){ // read until we have nothing to read from client

						bytes_read_from_client = read(client_connection_fd, clientBuffer, MAX); // try reading from client

						if (bytes_read_from_client == EINTR){
							if (exit_flag){
								// exit gracefully
								printf("Exiting:  %d\n",getpid());
								close(client_connection_fd); // close when finish handling the client
								close(key_fd);
								exit(EXIT_FAILURE);
							}
						}
						else if (bytes_read_from_client < 0){ // error reading from client
							printf("Error reading from client socket: %s\n", strerror(errno));
							exit(errno); 
						}

						else if (bytes_read_from_client == 0){ // finish reading - THIS WILL END THE WHILE LOOP
							break;
						}

						// we sum the number of bytes read every time from key file - until reaching bytes_read_from_client
						total_bytes_read_from_key = 0; 

						while (total_bytes_read_from_key < bytes_read_from_client){
							// read from key - enter the data to next avaliable location in buffer
							bytes_read_from_key = read(key_fd, keyBuffer + total_bytes_read_from_key, bytes_read_from_client - total_bytes_read_from_key);
							
							if (bytes_read_from_key == EINTR){
								if (exit_flag){
									// exit gracefully
									printf("Exiting:  %d\n",getpid());
									close(client_connection_fd); // close when finish handling the client
									close(key_fd);
									exit(EXIT_FAILURE);
								}
							}
							else if ( bytes_read_from_key < 0){ // check for error in reading key
								printf("Error while reading from key file: %s\n", strerror(errno));
								exit(errno);
							}

							else // maybe we need to iterate on key file...

								if (bytes_read_from_key == 0){ // key file reached EOF
									
										close(key_fd);
										key_fd = open(KEY, O_RDONLY); /* opens key file from the beggining */
						
					   					if( key_fd < 0 ){
					        				printf( "Error opening key file : %s\n", strerror(errno) );
					        				exit(errno); 
					   					}   
								}
							
							else { // the reading was succesful!
								total_bytes_read_from_key = total_bytes_read_from_key + bytes_read_from_key;
							}

						}

						// now we got 'bytes_read_from_client' from key file in keyBuffer - can XOR
						
						// XOR the bytes from key and client
						for (i = 0; i < bytes_read_from_client; ++i)
								clientBuffer[i] = clientBuffer[i] ^ keyBuffer[i];
						
						total_bytes_written_to_client = 0; // sum the bytes we write - make sure we wrote everything
						while (total_bytes_written_to_client < bytes_read_from_client) {
							
							bytes_written_to_client = write(client_connection_fd, clientBuffer + total_bytes_written_to_client, bytes_read_from_client - total_bytes_written_to_client);
							
							if (bytes_written_to_client == EINTR){
								if (exit_flag){
									// exit gracefully
									printf("Exiting:  %d\n",getpid());
									close(client_connection_fd); // close when finish handling the client
									close(key_fd);
									exit(EXIT_FAILURE);
								}
							}
							else if (bytes_written_to_client < 0) {
								printf("error write() to client : %s\n", strerror(errno));
								exit(errno);
							}

							// increment our counter
							total_bytes_written_to_client = total_bytes_written_to_client + bytes_written_to_client;

						} // finished writing everything to client

				
					} // nothing else to read from client
				

				// exit gracefully
				printf("FINISHED PROCESS: %d\n", getpid());
				printf("iteraten on KEY %d times\n",j);
				close(client_connection_fd); // close when finish handling the client
				close(key_fd);
				exit(0);

			}
			 // if we got here, that means that (process_id != 0) - we continue to iterate to get next connection

		}


}