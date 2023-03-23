// A Program by Noah Calhoun
// Creates a random key for encrpytion/decryption using One-Time pad
// example input: ./keygen 100
// see sources.txt for references used.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


int main(int argc, char **argv) {
	
	if (atoi(argv[1]) <= 0) 
    {
		perror("Please enter a valid number of characters");
		exit(1);
	}
	// Create a seed basedon current time
	srand(time(0));

	// Generate random character from A-Z
	for (int i = 0; i < atoi(argv[1]); i++) 
    {
		int value = (rand() % (26 + 1));
		value += 64;
		printf("%c", value);
	}

    putchar('\n');
}