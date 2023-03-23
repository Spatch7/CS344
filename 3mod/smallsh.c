// A program by Noah Calhoun
//
// Citations:
// https://canvas.oregonstate.edu/courses/1901764/assignments/9087347?module_item_id=22777104
// https://stackoverflow.com/questions/7600304/how-to-add-a-pid-t-to-a-string-in-c
// https://stackoverflow.com/questions/27306764/capturing-exit-status-code-of-child-process
// https://canvas.oregonstate.edu/courses/1901764/pages/exploration-environment?module_item_id=22777103
// https://www.programiz.com/c-programming/library-function/ctype.h/isdigit
// https://www.educative.io/answers/how-to-convert-a-string-to-an-integer-in-c
// https://man7.org/linux/man-pages/man2/kill.2.html
// https://man7.org/linux/man-pages/man2/sigaction.2.html
// https://stackoverflow.com/questions/25315191/need-to-clean-up-errno-before-calling-function-then-checking-errno
// 

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <stddef.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <errno.h>
#include <stdint.h>

#define BUFSIZE 2048

char *getenv(const char *name);
int setenv(const char *name, const char *value, int overwrite);
int unsetenv(const char *name);
char *str_gsub(char *restrict *restrict haystack, char const *restrict needle, char const *restrict sub);
void clearerr(FILE *stream);


char *strdup(char const *s) { 
    size_t len = strlen(s) + 1;
    char *ret = malloc(len);
    if (ret != NULL)
        strcpy(ret, s);
    return ret;
}

ssize_t getline(char*[], size_t*, FILE*);       // Stop warning me

void check_background_processes() {
    pid_t pid;
    int status;

    // Check for all child processes that have exited
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        if (WIFEXITED(status)) 
        {
            fprintf(stderr, "Child process %jd done. Exit status %d\n", (intmax_t) pid, WEXITSTATUS(status));
        } 
        else if (WIFSIGNALED(status)) 
        {
            fprintf(stderr, "Child process %jd done. Signaled %d.\n", (intmax_t) pid, WTERMSIG(status));
        }
        else if (WIFSTOPPED(status)) 
        {
            fprintf(stderr, "Child process %jd stopped. Continuing.\n", (intmax_t) pid);
            kill(pid, SIGCONT);
        }
    }
}

void check_background_processes_end() {
    pid_t pid;
    int status;

    // Check for all child processes that have exited
    while ((pid = waitpid(-pid, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status)) 
        {
            fprintf(stderr, "Child process %jd done. Exit status %d\n", (intmax_t) pid, WEXITSTATUS(status));
        } 
        else if (WIFSIGNALED(status)) 
        {
            fprintf(stderr, "Child process %jd done. Signaled %d.\n", (intmax_t) pid, WTERMSIG(status));
        }
        else if (WIFSTOPPED(status)) 
        {
            fprintf(stderr, "Child process %jd stopped. Continuing.\n", (intmax_t) pid);
            kill(pid, SIGCONT);
        }
    }
}


void handle_SIGINT(int signo){
}

void handle_SIGTSTP(int signo){
    signal(SIGTSTP, SIG_IGN);
}


int main(int argc, char *argv[]) {
    char *input = NULL;
    size_t input_size = 0;
    char *args[BUFSIZE];
    int status = 0;
    pid_t backPid;
    char *input_file = NULL;
    char *output_file = NULL;
    char *arg;
    char* PS1 = "PS1";
    char* IFS = "IFS";

    // -------------------- Signal Setup -----------------
    // Setup signals
    struct sigaction SIGINT_action = {0};
	SIGINT_action.sa_handler = handle_SIGINT;
	sigfillset(&SIGINT_action.sa_mask);
	// No flags set
	SIGINT_action.sa_flags = SA_RESTART;

    struct sigaction SIGTSTP_action = {0};
	SIGTSTP_action.sa_handler = handle_SIGTSTP;
	sigfillset(&SIGTSTP_action.sa_mask);
	// No flags set
	SIGTSTP_action.sa_flags = SA_RESTART;


	// Install our signal handler
	sigaction(SIGINT, &SIGINT_action, NULL);
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);
    // ---------------------------------------------------

    // set Prompt String
    setenv(PS1, getenv(PS1), 0);
    char* checkIFS = getenv(IFS);
    if (checkIFS == NULL)
    {
        setenv(IFS, " \t\n", 0);
    }
    else
    {
        setenv(IFS, getenv(IFS), 0);
    }

    while (1) 
    {
        memset(args, 0, sizeof(args));
        fflush(stdout);
		fflush(stdin);

        check_background_processes();

        printf("%s", getenv(PS1));

        ssize_t input_len = getline(&input, &input_size, stdin);
        if (input_len == -1) 
        {
            if (feof(stdin)) 
            { 
                break; 
            }
            clearerr(stdin);
            clearerr(stdout);
            goto end;   // retry cycle
        }
        if (errno == EINTR) {
                goto end;
            }


        input[input_len - 1] = '\0';

        
        if (strcmp(input, "") == 0) 
        {
            goto end;
        } 
        
        // -------------------- Expansion ----------------------
        // Set up arrays for replacing extensions
        char pid_str[255]; 
        char homeAray[255];
        char str_status[255];
        char str_backPID[255];
        snprintf(pid_str, 255, "%d", getpid());
        snprintf(homeAray, 255, "%s", getenv("HOME"));
        snprintf(str_status, 255, "%i", status);
        if (backPid > 0) {
            snprintf(str_backPID, 255, "%i", backPid);
        } 
        else 
        {
            snprintf(str_backPID, 255, "%s", "");
        }

        // Sub variables with values
        char *substitutions[] = { "$$", pid_str, "~/", homeAray, "$?", str_status, "$!", str_backPID };
        char *substituted_input = input;
        for (int i = 0; i < sizeof(substitutions) / sizeof(*substitutions); i += 2) 
        {
            char *needle = substitutions[i];
            char *sub = substitutions[i + 1];
            substituted_input = str_gsub(&substituted_input, needle, sub);
            if (!substituted_input) 
            {
                fprintf(stderr, "Error: Failed to allocate memory\n");
                break;
            }
        }
        // --------------------------------------------------------
        
        // --------------- Split input words ----------------------

        char* delimiter = getenv(IFS);
        arg = strtok(substituted_input, delimiter);
        int arg_count = 0;
        while (arg != NULL) {
            args[arg_count++] = strdup(arg);
            arg = strtok(NULL, delimiter);
        }
        args[arg_count] = NULL;
        // --------------------------------------------------------

        if (strcmp(args[0], "exit") == 0) 
        {
            if (arg_count == 1 )
            {
                fprintf(stderr, "\nexit\n" );
                exit(status);
            }
            if (arg_count > 2 || isdigit(*args[1]) == 0)     // exit with error if > 1 arg or not an int
            {   
                fprintf(stderr, "\nexit\n" );
                exit(WEXITSTATUS(status));
            }
            if (args[1] != NULL)                            // Regular exit
            {
                int exit_int = atoi(args[1]);
                status = exit_int;   
                fprintf(stderr, "\nexit\n" );
                exit(exit_int);
            }
            else
            {   
                fprintf(stderr, "\nexit\n" );
                exit(WEXITSTATUS(status));
            }   
        }
      
        int i;
        int has_output_redirect = 0;
        int has_input_redirect = 0;
        int has_background_process = 0;
        for (i = 0; i < arg_count; i++) 
        {   
            if (args[i] == NULL)
            {
                break;
            }            
            if (strcmp(args[i], "#") == 0) 
            {
                args[i] = NULL;
                break;
            } 
            else if (strcmp(args[i], "&") == 0) 
            {
                has_background_process = 1;
                args[i] = NULL;
            } 
            else if (strcmp(args[i], ">") == 0) 
            {
                has_output_redirect = 1;
                if (args[i + 1] != NULL) 
                {
                    output_file = strdup(args[i + 1]);
                    args[i] = NULL;
                    
                } 
                else 
                {
                    fprintf(stderr, "Error: No output file specified\n");
                    goto end;
                }
            } 
            else if (strcmp(args[i], "<") == 0) 
            {
                has_input_redirect = 1;
                if (args[i + 1] != NULL) 
                {
                    input_file = strdup(args[i + 1]);
                    args[i] = NULL;
                } 
                else 
                {
                    fprintf(stderr, "Error: No input file specified\n");
                    goto end;
                }
            }
            
        }        


        if (strcmp(args[0], "cd") == 0) 
        {
            if (args[1] == NULL) 
            {
                chdir(getenv("HOME"));
                continue;
            }

            if (chdir(args[1]) != 0) 
            {
                perror("cd");
                goto end;
            }
            continue;
        }

        pid_t pid = fork();
        switch (pid)
        {
            case -1:

                perror("fork() failed!");
                exit(1);
                break;

            case 0:
                
                if (has_input_redirect) 
                {
                    if (input_file != NULL) 
                    {
                        int in = open(input_file, O_RDONLY);
                        if (in == -1) 
                        {
                            fprintf(stderr, "Error redirecting input \n");
                            goto end;
                        }
                        if (dup2(in, 0) == -1) 
                        {
                            fprintf(stderr, "Target open()");
                            goto end;
                        }
                        close(in);
                        free(input_file);
                    }
                }

                if (has_output_redirect) 
                {
                    if (output_file != NULL) 
                    {
                        int out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0777);
                        if (out == -1) 
                        {
                            fprintf(stderr, "Error redirecting input \n");
                            goto end;
                        }
                        if (dup2(out, 1) == -1) 
                        {
                            fprintf(stderr, "Target open()");
                            goto end;
                        }
                        close(out);
                        free(output_file);
                    }
                }
                
                execvp(args[0], args);
                perror("execvp");
                exit(2);

            default:
                if (has_background_process) 
                {
                    backPid = pid;
                    waitpid(pid, &status, WNOHANG);
                    if (WIFSTOPPED(status)) 
                    {
                        fprintf(stderr, "Child process %jd stopped. Continuing.\n", (intmax_t) pid);
                        kill(pid, SIGCONT);
                    }
                    fflush(stdout);
			    }

                else
                {
                    waitpid(pid, &status, 0);
                    if (WIFEXITED(status)) 
                    {
                        status = WEXITSTATUS(status);
                    } 
                    else if (WIFSIGNALED(status)) 
                    {
                        status = WTERMSIG(status);                        
                    }
                    if (WIFSTOPPED(status)) 
                    {
                        fprintf(stderr, "Child process %jd stopped. Continuing.\n", (intmax_t) pid);
                        kill(pid, SIGCONT);
                    }
                }
                check_background_processes_end();
                fflush(stdout);               

        }
        
        end:;
        errno = 0;
        clearerr(stdin);
        

    }
    free(input);
    free(input_file);
    free(output_file);
    return 0;
}


char *str_gsub(char *restrict *restrict haystack, char const *restrict needle, char const *restrict sub) 
{
    char *original = *haystack;
    char *match = strstr(original, needle);
    if (!match) {
        return original;
    }
    size_t originalLen = strlen(original);
    size_t newLen = originalLen - strlen(needle) + strlen(sub) + 1;
    char *new = malloc(newLen);
    if (!new) {
        return NULL;
    }
    *match = '\0';
    snprintf(new, newLen, "%s%s%s", original, sub, match + strlen(needle));
    *haystack = new;
    return new;
}

