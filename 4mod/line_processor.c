// A program by Noah Calhoun.
// Citations:
// https://edstem.org/us/courses/32080/discussion/2595618
// https://replit.com/@RyanGambord/Producer-Consumer-Pipeline?v=1#main.c

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "fifo.h"
#include <string.h>
#define arrlen(arr) (sizeof(arr) / sizeof *(arr))

struct thread_args 
{
	struct fifo *in, *out;
};

FILE *input_file;
FILE *outputFile;
// FILE *output_file;
char* output_file = NULL;
char converted = '^';
int in_flag;
int out_flag;
const int line_max = 80; 
int char_count;


// Takes input from user
void *input_thread(void *_targs)
{
	struct thread_args *targs = _targs;
	char buffer[1001]; // buffer for storing input line
	int line_len = 0;  // length of input line
	int stop_received = 0;  // flag to indicate if "STOP\n" line has been received
	
	while (!stop_received && fgets(buffer, sizeof(buffer), input_file) != NULL) 
	{
		// check for line length
		line_len += strlen(buffer);
		if (line_len > 1000) 
		{
			err(1, "input line too long");
		}

		// check for STOP\n
		if (strncmp(buffer, "STOP\n", 5) == 0) 
        {
			stop_received = 1;
        }

		// filter non-printable characters
		for (int i = 0; i < strlen(buffer); i++) 
		{
			if (buffer[i] < 32 || buffer[i] > 126) {
				buffer[i] = ' ';
			}
		}

		

		// write input line to output fifo
		if (fifo_write(targs->out, buffer, strlen(buffer)) == -1) 
		{
			err(1, "fifo_write");
		}
	}
	// close fifos and return thread args
	fifo_close_read(targs->in);
	fifo_close_write(targs->out);
	return targs;
}


void * separator_thread(void *_targs)
{
	struct thread_args *targs = _targs;
	for (;;) 
	{
		char c;
		ssize_t r = fifo_read(targs->in, &c, 1);
		if (r < 0) err(1, "fifo_read");
		if (r == 0) break;
		if (c == '\n') c = ' ';
		if (fifo_write(targs->out, &c, r) == -1) err(1, "fifo_write");
	}
	fifo_close_read(targs->in);
	fifo_close_write(targs->out);
	return targs;
}


void * convert_thread(void *_targs)
{
	struct thread_args *targs = _targs;
	char buffer[2] = { 0 };  // buffer for storing 2 characters from fifo
	int index = 0;  // current position in buffer

	for (;;) 
	{
		ssize_t r = fifo_read(targs->in, &buffer[index],  1); 
		if (r < 0) err(1, "fifo_read");
		if (r == 0) break;
		
		if (buffer[index] == '+') {  
			index++;  // increment index and write
			if (index == 2) 
			{  
				if (fifo_write(targs->out, &converted, 1) == -1) err(1, "fifo_write");  
				index = 0;  
			}
		} 
		else 
		{
			if (index == 1) 
			{  
				if (fifo_write(targs->out, "+", 1) == -1) err(1, "fifo_write");  
			}
			if (fifo_write(targs->out, &buffer[0], index + 1) == -1) err(1, "fifo_write");  // write buffer to fifo
			index = 0;
		}
	}
	fifo_close_read(targs->in);
	fifo_close_write(targs->out);
	// printf("END OF SWAP FUNCTION\n");
	return targs;
}


void *output_thread(void *_targs)
{
    struct thread_args *targs = _targs;
    char buffer[81];  

    if (output_file != NULL) {
        outputFile = fopen(output_file, "w");
        if (outputFile == NULL) {
            err(1, "fopen");
        }
    }

	int line_len = 0;
    for (;;) {
        // read a character from the input FIFO
        char c;
        ssize_t r = fifo_read(targs->in, &c, 1);
        if (r < 0) err(1, "fifo_read");
        if (r == 0) break;
        
		buffer[line_len] = c;
		line_len++;
		if (line_len == line_max) 
		{
			if (output_file != NULL) 
			{
				fwrite(buffer, 1, line_max, outputFile); // write line to file
				if (buffer[line_max-1] != '\n') {
					fputc('\n', outputFile); // write newline to file
				}
			} 
			else 
			{
				fwrite(buffer, 1, line_max, stdout); // write line to standard output
				if (buffer[line_max-1] != '\n') {
					fputc('\n', stdout); // write newline to standard output
				}
			}
			line_len = 0;
		}
    }

    if (output_file != NULL) 
	{
        fclose(outputFile);
		
    }

    fifo_close_read(targs->in);
    fifo_close_write(targs->out);

    return targs;
}



int main(int argc, char *argv[])
{
	in_flag = 0;
	out_flag = 0;
	if (argc > 1) 
	{
		for (int i = 0; i < argc; i++)
		{
			if (strcmp(argv[i], "<") == 0 && in_flag == 0) 
			{
				if (argv[i + 1] != NULL) 
                {	
					input_file = fopen(argv[i + 1], "r");
					if (input_file == NULL) 
					{
						fprintf(stderr, "input file %s not found", argv[i + 1] );
					}
					in_flag = 1;
					if (freopen(argv[i + 1], "r", stdin) == NULL) 
					{
						fprintf(stderr, "freopen input file %s", argv[i + 1] );
					}
				}
			}
			else if (strcmp(argv[i], ">") == 0 && out_flag == 0) 
			{
				if (argv[i + 1] != NULL) 
                {
					strcpy(output_file, argv[i + 1] );
					if (freopen(argv[i + 1], "w", stdout) == NULL) 
					{
						fprintf(stderr, "freopen output file %s", argv[i + 1] );
					}
					out_flag = 1;
				}
			}
		}

		// if < was not used in redirection
		if (in_flag == 0)
		{
			input_file = fopen(argv[1], "r");
			if (input_file == NULL) 
			{
				fprintf(stderr, "input file %s not found", argv[1] );
			}
			in_flag = 1;
			if (freopen(argv[1], "r", stdin) == NULL) 
			{
				fprintf(stderr, "freopen input file %s", argv[11] );
			}
		}
		
	} 

	if ( in_flag == 0)
	{
		// printf("inputing manually \n");
		input_file = stdin;
	}

	struct fifo *fifos[3];
	for (size_t i = 0; i < arrlen(fifos); ++i) {
		fifo_create(&fifos[i], 1024);
	}

	pthread_t threads[4];
	pthread_create(&threads[0], NULL, input_thread, &(struct thread_args) {.in=NULL, .out=fifos[0]});
	pthread_create(&threads[1], NULL, separator_thread, &(struct thread_args) {.in=fifos[0], .out=fifos[1]});
	pthread_create(&threads[2], NULL, convert_thread, &(struct thread_args) {.in=fifos[1], .out=fifos[2]});
	pthread_create(&threads[3], NULL, output_thread, &(struct thread_args) {.in=fifos[2], .out=NULL});
	
	for (size_t i = 0; i < arrlen(threads); ++i) {
		pthread_join(threads[i], NULL);
	}
	for (size_t i = 0; i < arrlen(fifos); ++i) {
		fifo_destroy(fifos[i]);
		fifos[i] = NULL;
	}
}