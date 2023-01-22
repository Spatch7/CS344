// Base64 Encoding, a program by Noah Calhoun, calhounn@okstate.edu
// 1/21/2023

//--------------------------------------------------------------------------
// Citations: 1/19/2023
// https://man7.org/linux/man-pages/man3
// https://edstem.org/us/courses/32080/discussion/2372392
// https://en.cppreference.com/w/c/io/fread
// https://en.cppreference.com/w/c/io/fopen
// https://stackoverflow.com/questions/4415524/common-array-length-macro-for-c
// https://edstem.org/us/courses/32080/discussion/2419916
// https://canvas.oregonstate.edu/courses/1901764/assignments/9087342?module_item_id=22777078
// https://canvas.oregonstate.edu/courses/1901764/pages/exploration-memory-allocation?module_item_id=22777084
// https://canvas.oregonstate.edu/courses/1901764/pages/exploration-debugging-c?module_item_id=22777088
// https://canvas.oregonstate.edu/courses/1901764/pages/exploration-pointers?module_item_id=22777082
//--------------------------------------------------------------------------

// TO RUN: 
// gcc -std=c99 base64.c -o base64
// ./base64 <some_file_here>.txt

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))


static char const alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

int countCharacters(FILE * file_pointer)
{
	int counter = 0;
	int valid;
	while ((valid = fgetc(file_pointer)) != EOF) 
	{ 
       counter = counter + 1;
    }
	// printf("chars: %i \n", counter);
	return counter;
}

int processFile(size_t *size, uint8_t *inptr, uint8_t *outptr)
{
	size_t i;
	
	if (*size == 2)
	{
		*(inptr+2) = 0;
	}
	else if (*size == 1)
	{
		*(inptr+2) = 0;
		*(inptr+1) = 0;
	}

	i = 0;
	while(i < *size + 1)
	{
		if(i == 0)
		{
			*(outptr+i) = *inptr >> 2;
			i++;
		}
		else if (i == 1)
		{
			*(outptr+i) = ((*inptr & 0x03u) << 4) | (*(inptr+1)>> 4);
			i++;
		}
		else if (i == 2)
		{
			*(outptr+i)  = ((*(inptr+1) & 0x0Fu) << 2) | (*(inptr+2) >> 6);
			i++;
		}
		else if (i == 3)
		{
			*(outptr+i)  = *(inptr+2) & 0x3Fu;
			i++;
		}		
	}
	if (*size == 1)
	{
		*(outptr+3)  = 64;
		*(outptr+2)  = 64;
	}
	else if (*size == 2)
	{
		*(outptr+3)= 64;
	}
	return -1;
}



int main(int argc, char * argv[]){

	int remaining;
	int charWritten;
	int terminate;
	uint8_t intake[3];
	uint8_t out[4];
	
	// printf("%i \n", argc);
	if ( argc > 2 )
	{
		fprintf( stderr, "Error: Two Arguments Entered \n");
		return -1;
	}

	// Open file
	FILE* file_pointer; 

	file_pointer = fopen(argv[1], "r");
	
	if (file_pointer == NULL )
	{
		FILE* tmpPt = tmpfile();

		int count_temp = 0;
		char buffer[1000];
		size_t readBytes;

		for (;;) {
			readBytes = fread(buffer, sizeof(char), sizeof(buffer), stdin);
			if (readBytes <= 0) 
			{
				break;
			}
			count_temp++;
		}

		size_t writeBytes = fwrite(buffer, sizeof(char), sizeof(buffer), tmpPt);
		if (writeBytes <= 0)
        {

            fprintf(stderr, "error writing temp file");
        }
		
        file_pointer = tmpPt;
		if (ferror(file_pointer))
		{
			fprintf(stderr, "error reading temp file");
		}
		printf("\n");

	}
	
	// Reading from a file, get remaining characters
	remaining = countCharacters(file_pointer);
	if (remaining == -1)
	{ 
		fprintf(stderr, "File character error, authenicate input data"); 
		fclose(file_pointer);
		return -1;
	}
	fseek(file_pointer, 0, SEEK_SET); //reset pointer to start of file
	
	charWritten = 0;
	terminate = 0;
	while (true)	
	{
		char encoded;

		// set fp* to current, read 3 characters
		fseek(file_pointer, 0, SEEK_CUR); 

		size_t size = fread(intake, sizeof(*intake), ARRAY_SIZE(intake), file_pointer);  // buffer, size(byte), count, stream 

		if (feof(file_pointer))
		{ 
			terminate = 1;
		}
		else if (ferror(file_pointer)) {
           	fprintf(stderr, "File character error, authenicate input data"); 
			fclose(file_pointer);
			return -1;
       	}
	         
		processFile(&size, intake, out); // Process characters
		remaining = remaining - size;
		
		for (int j = 0; j<4; j++)
		{
			encoded = alphabet[out[j]];

			// Write to file
			if (charWritten == 76)
			{
				//write new line
				charWritten = 0;
				fprintf( stdout, "\n");
			}
			charWritten++;
			fprintf( stdout, "%c", encoded );
		}

		if (terminate == 1 || remaining <= 0) // Exit True Loop, end of file 
		{
			fprintf( stdout, "\n");
			break;
		}
	}
	
	fclose(file_pointer);
	return 0;

}




