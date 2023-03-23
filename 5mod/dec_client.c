// A Program by Noah Calhoun
// Acts as a client to communicate with dec_server
// sends a textfile, key, port, and outputFile for the server to decrpyt and return.
// example input: ./enc_client plaintext1 mykey 55124 > ciphertext1
// see sources.txt for references used.

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <ctype.h>
#include <netdb.h>      // gethostbyname()
/**
* Client code
* 1. Create a socket and connect to the server specified in the command arugments.
* 2. Prompt the user for input and send that input as a message to the server.
* 3. Print the message received from the server and exit the program.
*/

// Error function used for reporting issues
void error(const char *msg) { 
  perror(msg); 
  exit(0); 
} 

// Set up the address struct
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber, 
                        char* hostname){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);

  // Get the DNS entry for this host name
  struct hostent* hostInfo = gethostbyname("localhost"); 
  if (hostInfo == NULL) { 
    fprintf(stderr, "CLIENT: Error, no such host\n"); 
    exit(0); 
  }
  // Copy the first IP address from the DNS entry to sin_addr.s_addr
  memcpy((char*) &address->sin_addr.s_addr, 
        hostInfo->h_addr_list[0],
        hostInfo->h_length);
}

int secure_connection(int connectionSocket);
char* read_input_file( char* file_name);


int main(int argc, char *argv[]) {
  int socketFD, portNumber, charsWritten, charsRead;
  char* input_text;
  int count_input_char;
  char* key;
  char input_text_size[10];
  int current_char = 0;
  struct sockaddr_in serverAddress;
  char buffer[256];
  // Check usage & args
  if (argc < 4) { 
    fprintf(stderr,"USAGE: %s hostname port\n", argv[0]); 
    exit(0); 
  } 
  
  // Read input file and return info
  input_text = read_input_file(argv[1]);
  key = read_input_file(argv[2]);

  // Compare text and key for validation
  if (strlen(input_text) > strlen(key))
  {
    // error("CLIENT: plaintext length is not compatable with key");
    fprintf(stderr,"CLIENT: plaintext length is not compatable with key");
    exit(1);
  }

  // Loop through input, check if any bad characters in key or txt
  for (int i = 0; i < strlen(input_text); i++) 
  {
		if (input_text[i] != '@' && isupper(input_text[i]) == 0 && input_text[i] != '\n') 
    {
      fprintf(stderr, "CLIENT: bad plaintext");
			// error("CLIENT: bad plaintext");
      exit(1);
    }
    if (key[i] != '@' && isupper(key[i]) == 0 && key[i] != '\n')
    {
			fprintf(stderr, "CLIENT: bad key");
			// error("CLIENT: bad key");
      exit(1);
		}
	}

  // Create a socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0); 
  if (socketFD < 0){
    error("CLIENT: Error trying to open socket");
  }

   // Set up the server address struct
  setupAddressStruct(&serverAddress, atoi(argv[3]), argv[1]);

  // Connect to server
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
    error("CLIENT: Error connecting");
  }


  // ----------------------- Begin connection operations -----------------------------------
  if (secure_connection(socketFD) == 1)
  {
    // Complete verification of author with server
    char read_buffer[1024];
    memset(read_buffer, '\0', 1024);
    char input_text_ct[10];
    char* request_txt = "sendtxt";
    char* request_key = "sendkey";
    int charsRead;
    int current_char = 0;

    // send size of plainText to server
    sprintf(input_text_ct, "%ld", strlen(input_text));
    while (current_char < strlen(input_text_ct)) 
    {
      charsRead = send(socketFD, input_text_ct + current_char, strlen(input_text_ct), 0); // Write to the server
      if (charsRead < 0) 
      {
        error("CLIENT: Error trying to write plainText with socket");
      }

      if (charsRead <= strlen(input_text_ct) && charsRead >= 0) 
      {
        current_char += charsRead;
      }
    }

    memset(read_buffer, '\0', sizeof(read_buffer)); 
    // Read data from the socket, leaving \0 at end
    charsRead = recv(socketFD, read_buffer, sizeof(read_buffer) - 1, 0); 
    if (charsRead < 0) 
    {
      error("CLIENT: Error trying to read key request with socket");
    }

    if (strcmp(read_buffer, request_txt) == 0) 
    {
      // Send plainText to server
      current_char = 0;
      while (current_char < strlen(input_text)) 
      {
        // Write to server
        charsRead = send(socketFD, input_text + current_char, strlen(input_text), 0); 
        if (charsRead < 0) 
        {
          error("CLIENT: Error writing input_text to socket");
        }

        if (charsRead <= strlen(input_text) && charsRead >= 0) 
        {
          current_char += charsRead;
        }
      }
    } 
    else 
    {
      error("CLIENT: server failed to request input_text properly\n");
    }

    // Get key request message from server
    memset(read_buffer, '\0', sizeof(read_buffer)); 
    charsRead = recv(socketFD, read_buffer, sizeof(read_buffer) - 1, 0); 
    if (charsRead < 0) 
    {
      error("CLIENT: Error reading key request with socket");
    }

    if (strcmp(read_buffer, request_key) == 0) {
        // Send key to server
        current_char = 0;
        while (current_char < strlen(input_text)) 
        {
          // Write to server
          charsRead = send(socketFD, key + current_char, strlen(input_text), 0); 
          if (charsRead < 0) 
          {
            error("CLIENT: Error writing key with socket");
          }

          if (charsRead <= strlen(input_text) && charsRead >= 0) {
              current_char += charsRead;
          }
        }
    } 
    else 
    {
      error("CLIENT: server failed to request key\n");
    }

    // Get encrypted plaintext from encryptor server
    charsRead = recv(socketFD, input_text, strlen(input_text), 0); 
  
  }

  else
  {
    // Bad operation, close and exit
    close(socketFD);
    fprintf(stderr, "CLIENT: Bad handshake connection" );
    // error("CLIENT: Bad handshake connection");
    exit(2);
  }

  printf("%s", input_text);
  close(socketFD);
  return 0;
}


char* read_input_file(char* input_file) 
{
  // Simple function for reading from a file
	char* input_char;
	size_t count_input_char = 32;
	size_t count_chars;
  // Open file and read/set characters into structure
	FILE* fd = fopen(input_file, "r");
	if (fd == NULL) 
  {
		fprintf(stderr, "CLIENT: error opening file %s\n", input_file);
	}
	input_char = (char *)malloc(count_input_char * sizeof(char));

	if (input_char == NULL) 
  {
		error("CLIENT: unable to allocate space for input file");
	}

	count_chars = getline(&input_char, &count_input_char, fd);
  if (count_chars == -1) 
  {
		error("CLIENT: could not read file");
	}
	else if (count_chars == 0) 
  {
		error("CLIENT: error, no input text");
	}
	
	return input_char;
}


int secure_connection(int socketFD) {
    char buffer[1024];
    char* client_name = "dec_client";
    char* server_name = "dec_server";
    memset(buffer, '\0', 1024);
    int charsRead;
    int current_char = 0;

    // Make handshake with server
    while (current_char < strlen(client_name)) {
        
        charsRead = send(socketFD, client_name + current_char, strlen(client_name) - current_char, 0); 
        if (charsRead < 0) 
        {
          error("CLIENT: Error writing id message to socket");
        }
        if (charsRead <= strlen(client_name) - current_char && charsRead >= 0) 
        {
          current_char += charsRead;
        }
    }

    // Read handshake from server
    memset(buffer, '\0', sizeof(buffer)); 
    charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0);
    if (charsRead < 0) 
    {
      error("CLIENT: Error reading from socket");
    }

    // Confirm identity is the encryptor server 
    if (strcmp(buffer, server_name) == 0) 
    {
      return 1;
    }
    else
    {
      return 0;
    }
}
