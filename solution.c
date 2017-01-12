#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#define BUF_SIZE 4096

int main(int argc, char** argv)
{
	int fdsrc, fddst, fdkey;
	int i, numsrc, numkey, numdst, num, ret;
	DIR* dirsrc;
	struct stat statbuf;
	struct dirent* dent;
	
	// keep buffers of FIXED size
	char srcbuf[BUF_SIZE];
	char keybuf[BUF_SIZE];
	char srcname[BUF_SIZE];
	char dstname[BUF_SIZE];
	
	// not demanded, but helpful
	assert(argc == 4);

	// open source dir - to read directory entries from
	dirsrc = opendir(argv[1]);
	
	// ALWAYS check ALL return values of ALL system calls
	if (dirsrc == NULL) {
		// on error, print a DESCRIPTIVE message, and exit
		printf("error with opendir() input: %s\n", strerror(errno));
		return -1;
	}
	
	// we do NOT, at any point, open the output dir!
	// we might not have read permissions, so we may only use stat()
	// (to check if it exists)
	ret = stat(argv[3], &statbuf);
	
	// make sure we succeeded (i.e., dir exists)
	if (ret < 0) {
		// failed but error isn't ENOENT - something went wrong
		if (errno != ENOENT) {
			printf("error with stat() output dir: %s\n", strerror(errno));
			return -1;
		}
		// error is ENOENT - path doesn't exist
		else {
			// try to create output dir, exit if failed for any reason
			// give 0777 permissions - number starts with 0 for base 8!
			if (mkdir(argv[3], 0777) < 0) {
				printf("error mkdir() output: %s\n", strerror(errno));
				return -1;
			}
		}
	}

	// we may not use access() or other system calls - only those we learned!
	// that's why the correct process is: use stat(), check errno, then use mkdir()

	// open key file - useful to open it once instead of repeatedly.
	// open for read ONLY! we may not have permissions to write, and then fail
	fdkey = open(argv[2], O_RDONLY);
	if (fdkey < 0) {
		printf("error open() key file: %s\n", strerror(errno));
		return -1;
	}
	
	// make sure the key isn't empty
	// here we just read 1 byte from it and see what happens
	ret = read(fdkey, keybuf, 1);
	if (ret < 0) {
		printf("error read() from key: %s\n", strerror(errno));
		return -1;
	}
	// got EOF - key is empty, no point to proceed
	else if (ret == 0) {
		printf("key file is empty!\n");
		return -1;
	}
	
	// iterate over dir entries until done (NULL)
	while ((dent = readdir(dirsrc)) != NULL) {
		// do NOT open the output file yet, make sure it isn't a folder first!
		if ((!strcmp(dent->d_name, ".")) || (!strcmp(dent->d_name, "..")))
			continue;	// skip these
		
		// construct input and output file names - as seen in code from first recitation
		sprintf(srcname, "%s/%s", argv[1], dent->d_name);
		sprintf(dstname, "%s/%s", argv[3], dent->d_name);
		
		// another option is to skip ALL folders - we demonstrate this as well
		if (stat(srcname, &statbuf) < 0) {
			// ALWAYS check for errors, on ALL system calls
			printf("error stat() input file %s: %s\n", dent->d_name, strerror(errno));
			return -1;
		}
		
		// this is the correct way to skip folders with stat()
		// checking for S_ISREG, etc. - is incorrect
		// we need to make sure IT ISN'T a folder, and not that IT IS a regular file
		// (there are files that aren't regular but should still work - links, block devices, etc.)
		if (S_ISDIR(statbuf.st_mode))
			continue;
		
		// we start a new file operation, so reset the key file
		// (DO NOT leave it as it was, either open it again, or reset to beginning,
		// so encrypting a 2nd file doesn't begin in the middle of the key)
		if (lseek(fdkey, SEEK_SET, 0) < 0) {
			printf("error lseek() key: %s\n", strerror(errno));
			return -1;
		}
		
		// open input file - for read ONLY! we may not have permissions to write
		fdsrc = open(srcname, O_RDONLY);
		if (fdsrc < 0) {
			printf("error open() input file %s: %s\n", dent->d_name, strerror(errno));
			return -1;
		}
		
		// open destination file. there are three required flags here:
		// O_WRONLY - we might not have read permissions, only write
		// O_CREAT - output file might not exist, create it
		// O_TRUNC - if output file DOES exist, truncate it (i.e., remove all contents)
		// 0777 permissions - again, we give all permissions, number starts with 0 for base 8!
		fddst = open(dstname, O_WRONLY | O_CREAT | O_TRUNC, 0777);
		if (fddst < 0) {
			printf("error open() output file %s: %s\n", dstname, strerror(errno));
			return -1;
		}
		
		// start the encryption loop
		// we use a loop so as not to read everything at once
		// input and/or key file might be very large - we can't use the size field,
		// can't read everything to memory - we must read chunks and iterate - for both!
		// (and for writing the output, later)
		while (1) {
			// *TRY* to read BUF_SIZE bytes from the input file
			// - we must limit our reads to a fixed size, and not read the entire file
			// - we must check if read was successful
			// - we can't assume BUF_SIZE bytes are read - we need to use the return value (numsrc)
			numsrc = read(fdsrc, srcbuf, BUF_SIZE);
			if (numsrc < 0) {
				printf("error read() from input file %s: %s\n", srcname, strerror(errno));
				return -1;
			}
			// received EOF - we're done (and that's the ONLY case we're done!)
			else if (numsrc == 0) {
				break;
			}

			// --------------------------------------
			// IMPORTANT NOTE
			// we do not set '\0' at the end of an array - this is wrong!
			// - first, if we defined arr[1024], setting arr[1024]='\0' is an access violation!
			// - second, this is a char array but isn't a string!
			//		it's used for binary reading (i.e., a bytes array)
			//		we put '\0' at the end of a *STRING* only!
			// This mistake was not deducted, but was repeated a lot!
			// It WILL be deducted in future homework
			// (as are other C errors / quirks)
			// --------------------------------------
			
			// set number of bytes read from the key file to 0
			numkey = 0;
			
			// iterate reading from key until reaching numsrc bytes
			while (numkey < numsrc) {
				num = read(fdkey, keybuf + numkey, numsrc - numkey);
				
				// we do not know the size of the key or how many bytes were read successfully
				// it might be LESS than BUF_SIZE/numsrc. It might be much more
				// we might succeed reading less than we requested.
				// Key might be larger than the input - we shouldn't read excess bytes.
				// We must deal with all of these.
				
				// error reading
				if (num < 0) {
					printf("error read() key: %s\n", strerror(errno));
					return -1;
				}
				// reached end of key, reset and read from start
				else if (num == 0) {
					// we must check lseek() - it's a system call!
					if (lseek(fdkey, SEEK_SET, 0) < 0) {
						printf("error lseek() key: %s\n", strerror(errno));
						return -1;
					}
					
					// another option is to close & reopen the key file
					// several very important things to notice here:
					// - if we close & reopen - we must make sure we indeed CLOSE the previous FD and never use it again
					// 		and we must replace the old FD with the new FD that we opened
					// - we shouldn't assume BUF_SIZE (or numsrc, etc.) bytes are read, ever! any amount can be read (up to what we requested)
					// - some reset the key after reading BUF_SIZE from it - that's incorrect. reset ONLY after EOF
				}
				// success - increase our counter of bytes read from key
				else {
					numkey += num;
				}
			}
			
			// now we have 'numsrc' bytes - from both input and key file
			
			// perform encryption operation
			// (using srcbuf, no need for another buffer)
			for (i = 0; i < numsrc; ++i)
				srcbuf[i] = srcbuf[i] ^ keybuf[i];
			
			// number of bytes we wrote to output
			numdst = 0;
			
			// as with read(), we can't assume write() will successfuly write everything
			// we iterate until everything is written
			while (numdst < numsrc) {
				// ALWAYS look at write()'s return value, don't assume everything is written
				num = write(fddst, srcbuf + numdst, numsrc - numdst);
				if (num < 0) {
					printf("error write() to output file %s: %s\n", dstname, strerror(errno));
					return -1;
				}
				
				// increment our counter
				numdst += num;
			}
		}
		
		// finished - CLOSE input & output file descriptors
		close(fdsrc);
		close(fddst);
	}

	// we're done - close the remaining file descriptors (key and input dir)
	close(fdkey);
	closedir(dirsrc);

	return 0;
}