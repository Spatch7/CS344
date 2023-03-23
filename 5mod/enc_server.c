// A Program by Noah Calhoun
// Acts as a server to communicate with enc_client
// Only input is a port for client to connect to
// example input: ./enc_server 55123
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

// Error function used for reporting issues
void error(const char *msg) {
  perror(msg);
  exit(1);
} 

int secure_connection(int connectionSocket);
void encrypt_item(int connectionSocket);

// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);
  // Allow a client at any address to connect to this server
  address->sin_addr.s_addr = INADDR_ANY;
}


int main(int argc, char *argv[])
{
  int connectionSocket, charsRead;
  struct sockaddr_in serverAddress, clientAddress;
  socklen_t sizeOfClientInfo;

  // Check usage & args
  if (argc < 2) { 
    fprintf(stderr,"USAGE: %s port\n", argv[0]); 
    exit(1);
  } 
  
  // Create the socket that will listen for connections
  int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (listenSocket < 0) {
    error("ERROR opening socket");
  }

  // Set up the address struct for the server socket
  setupAddressStruct(&serverAddress, atoi(argv[1]));

  // Associate the socket to the port
  if (bind(listenSocket, 
          (struct sockaddr *)&serverAddress, 
          sizeof(serverAddress)) < 0){
    error("ERROR on binding");
  }

  // Start listening for connetions. Allow up to 5 connections to queue up
  listen(listenSocket, 5); 
  
  int count_sockets_used = 0;
  int count_child_processes = 0;
  int child_processes[5];
  pid_t parent_pid = getpid();
  int status;


  // Accept connection, block until connection made 
  while(parent_pid == getpid())
  {

		if (count_child_processes > 0)
    {
      int i = 0;
      while ( i < count_child_processes)
      {
        pid_t pid_child = waitpid(child_processes[i], &status, WNOHANG);

        // Check for signals
        if (pid_child != 0)
        {
          if (WIFSIGNALED(status) != 0)
          {
            fprintf(stderr, "PID: %d terminated, signaled", pid_child );
          }
          if (WIFEXITED(status) != 0)
          {
            fprintf(stderr, "PID: %d terminated, exited", pid_child );
          }
          child_processes[i] = child_processes[count_child_processes - 1];
          count_sockets_used--;
          count_child_processes--;
        }
        i++;
      }
    }

    // This program only allows 5 streams
    if (count_sockets_used < 5)
    {
      sizeOfClientInfo = sizeof(clientAddress);
      connectionSocket = accept(listenSocket, 
                (struct sockaddr *)&clientAddress, 
                &sizeOfClientInfo); 
      if (connectionSocket < 0)
      {
        error("ERROR on accept");
      }

      // Fork off max 5 child streams
      pid_t pid = fork();
      switch (pid)
      {
        case -1:
          error("Error with server fork()");
          return -1;
          break;
        
        case 0:
          // If handshake with client successful, encrpyt text
          if (secure_connection(connectionSocket) == 1)
          {
            encrypt_item(connectionSocket);
            close(connectionSocket);
          }
          
          else
          {
					// Bad connection, close
            close(connectionSocket);
            fprintf(stderr, "SERVER: Error, child could not secure connection");
						// error("SERVER: Error, child could not secure connection");
            return -1;
          }
          return 0;
          break;    // Does this stop "Success"?

        default:
          // increment stream count
          child_processes[count_child_processes] = pid;
          count_child_processes++;
          count_sockets_used++;
          break;
      }
    }
  }

  close(listenSocket);
  return 0;
}


// Creates a handshake with the enc_client, returns 1 if 
int secure_connection (int connectionSocket)
{
  char* charsRead;
  int chars_total;
  int char_count = 0;
  char* server_name = "enc_server";
  char* client_name = "enc_client";
  char buffer[1024];
  memset(buffer, '\0', 1024);

	// Receive from client
  chars_total = recv(connectionSocket, buffer, 1024, 0);
  // Check if good client
  if (strcmp(buffer, client_name) == 0)
  {
			// Return 1 if successfull connection
      while (1)
      {
          int charsRead = send(connectionSocket, server_name + char_count, strlen(server_name), 0);
          if (charsRead < 0)
          {
              error("SERVER: Error writing message to socket");
              break;
          }
          if (charsRead <= strlen(server_name) && charsRead >= 0)
          {
              char_count += charsRead;
          }
          if (charsRead >= strlen(server_name))
          {
              break;
          }
      }
			
      return 1;
  }

  else
  {
			// Return 0, check first for bad connection  
      while (1)
      {   
          // Server is imposter, he shouldn't say "hello"
          int charsRead = send(connectionSocket, "hello", 5, 0);
          if (charsRead < 0)
          {
            error("SERVER: Error writing message to socket");
            return -1;
          }
          if (charsRead <= 5 && charsRead >= 0)
          {
            char_count += charsRead;
          }
          if (charsRead >= 5)
          {
            break;
          }
      }
      return 0;
  }
}


void encrypt_item(int connectionSocket)
{
  char buffer[1024];
  int charsRead;
  int char_count = 0;
  char* input_text;
  int input_text_size;
  char* text_req = "sendtxt";
  char* key_req = "sendkey";

  memset(buffer, '\0', 1024); 
  charsRead = recv(connectionSocket, buffer, 1024, 0);
  if (charsRead < 0)
  {
    fprintf(stderr, " Error: Could not read input plain-text");
    exit(1);
  }

  // Get and set plaintxt from client
  input_text_size = atoi(buffer) + 1;
  input_text = (char* )calloc(input_text_size, sizeof(char));
  memset(input_text, '\0', input_text_size);

  // Get and set key info
  char* key = (char* )calloc(input_text_size, sizeof(char));
  memset(key, '\0', input_text_size);
	// -------------------------------------------------------------------------------
	// Series of req and rec, confirming key/text, and resetting current_character
	// 		after each check
	
  // Request input_text from client
	while (char_count < strlen(text_req)) 
	{
		charsRead = send(connectionSocket, text_req + char_count, strlen(text_req), 0);
		if (charsRead < 0) 
		{
		  fprintf(stderr, "SERVER: error requesting input_text");
      exit(1);
		}
		{
		if (charsRead <= strlen(text_req) && charsRead >= 0) 
		{
			char_count += charsRead;
		}
		}
	}
	char_count = 0;

	// Receive input_text from client
	charsRead = recv(connectionSocket, input_text, input_text_size - 1, 0);
	if (charsRead < 0)
	{
		fprintf(stderr, "SERVER: Error receiving input_text");
    exit(1);
	}

	// Request key from client
	while (char_count < strlen(key_req)) 
	{
		charsRead = send(connectionSocket, key_req + char_count, strlen(key_req), 0);
		if (charsRead < 0)
		{
		  fprintf(stderr, "SERVER: Error requesting key");
      exit(1);
		} 
		if (charsRead <= strlen(key_req) && charsRead >= 0) {

		char_count += charsRead;
		}
	}
	char_count = 0;

	// FINALLY receive key from client
	charsRead = recv(connectionSocket, key, input_text_size - 1, 0); 
	if (charsRead < 0)
	{
		fprintf(stderr, "SERVER: Error receiving key");
    exit(1);
	} 
	// -------------------------------------------------------------------------------

	// ------------------------------ Encrypt plaintext ------------------------------
	for (int i = 0; i < strlen(input_text); i++) {
		if (input_text[i] == ' ') 
		{
      input_text[i] = (key[i] - 64) % 27 + 64;
		}
		else if (isupper(input_text[i]) != 0) 
		{
      input_text[i] = (input_text[i] + key[i] - 128) % 27 + 64;
		}
		else if (input_text[i] == '\n') 
    {
			break;
		}
    else if (input_text[i] == '\0')
    {
      input_text[i] = '\n';
			break;
    }
	}
	// -------------------------------------------------------------------------------

  // Return encrpytion
  while (char_count < strlen(input_text)) 
  {
    charsRead = send(connectionSocket, input_text + char_count, strlen(input_text), 0); 
    if (charsRead < 0) 
    {
			error("SERVER: Failed to return encrpyted plaintext");
      // fprintf(stderr, "Error: server issue returning encrpyted input_text");
      break;
    }
    if (charsRead <= strlen(input_text) && charsRead >= 0) 
		{
      char_count += charsRead;
    }
  }

}