#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
// Citations: https://man7.org/linux/man-pages/man3

// gcc -std=c99 base64.c -o base64
// ./base64 foo.txt
// QWZvb2w=

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))


static char const alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

int countCharacters(FILE * file_pointer){
	int counter = 0;
	int valid;
	while ((valid = fgetc(file_pointer)) != EOF) { // standard C I/O file reading loop
       counter = counter + 1;
    }
	// printf("chars: %i \n", counter);
	return counter;
}



int main(int argc, char * argv[]){

	int remaining;
	int charWritten;
	int terminate;
	uint8_t intake[3];
	uint8_t out[4];


	// Open file
	FILE* file_pointer; 

	file_pointer = fopen(argv[1], "r");
	
	if (file_pointer == NULL )
	{
		printf("no file entered \n");
	}
	
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
		int i;
		// printf("chars written %i \n", charWritten );

		// Read 3 characters
		fseek(file_pointer, 0, SEEK_CUR); 

		// size_t ret = fread(intake, sizeof(*intake), ARRAY_SIZE(intake), file_pointer);  // buffer, size(byte), count, stream 
		size_t size =fread(intake, sizeof(*intake), ARRAY_SIZE(intake), file_pointer);  // buffer, size(byte), count, stream 

		if (feof(file_pointer))
		{ 
			terminate = 1;
		}
		else if (ferror(file_pointer)) {
           	fprintf(stderr, "File character error, authenicate input data"); 
			fclose(file_pointer);
			return -1;
       }
	         
		
		
		// printf("three selected letters %c %c %c \n", intake[0], intake[1], intake[2] );
		// printf("remaining %i \n", intake[2] );
		// printf("size %li \n", size );

		// Process characters
		i = 0;
		while(i < size + 1)
		{
			if(i == 0)
			{
				out[i] = intake[0] >> 2;
				i++;
			}
			else if (i == 1)
			{
				out[i] = ((intake[0] & 0x03u) << 4) | (intake[1] >> 4);
				i++;
			}
			else if (i == 2)
			{
				out[i] = ((intake[1] & 0x0Fu) << 2) | (intake[2] >> 6);
				i++;
			}
			else if (i == 3)
			{
				out[i] = intake[2] & 0x3Fu;
				i++;
			}		
		}

		if ((remaining - size) == 0)
			{
				for(long int i = size+1; i < ARRAY_SIZE(out); i++)
				{
					if( out[i]==0)
					{
						out[i] = 64;
					}
					
				}	
			}

		remaining = remaining - size;

		// printf("OUT: %i %i %i %i \n \n", out[0], out[1], out[2], out[3] );

		char encoded;
		
		for (int j = 0; j<4; j++)
		{
			// printf("%d %d %d %d  \n", out[0], out[1],out[2],out[3]);
			encoded = alphabet[out[j]];
			// printf("Converted to: %c \n", encoded);

			// Write to file
			if (charWritten == 76)
			{
				//write new line
				charWritten = 0;
				fprintf( stdout, "\n");
			}
			// printf("%c \n", encoded);
			charWritten++;
			
			fprintf( stdout, "%c", encoded );
			

		}
		// fprintf( stdout, "\n");
		if (terminate == 1) // Exit True Loop, end of file 
		{
			fprintf( stdout, "\n");
			break;
		}
		
		

	}
	
	// if (some_function(...) == -1) err(errno, "some_function failed");
	fclose(file_pointer);


}




