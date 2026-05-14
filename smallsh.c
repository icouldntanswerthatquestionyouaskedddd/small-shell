/*
* Operating Systems I
* Assignment 3: smallsh
* A shell created to follow the requirements on the Assignment 3 page: https://canvas.oregonstate.edu/courses/2042419/assignments/10461893?module_item_id=26632749
* By Novia Weng
* 13 May 2026
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

/*
* The commandData struct stores information about a command.
* It has an array for all 512 arguments (inclusive of the command) plus the NULL ending value.
* It also includes pointers to the names of input and output files for redirection.
* An additional pointer references the command for easier access later in the program.
* A bool stores whether or not the process should be run in the background.
*/
struct commandData
{
    char *command;
    char *args[513]; // Make space for the command, then the other 511 arguments, plus on more space for a NULL value
    char *input_file;
    char *output_file;
    bool background;
};

static bool backgroundAllowed = true; // Global variable for keeping track of background allowed or foreground-only mode
static pid_t lastForegroundChild = 0; // Global variable that keeps track of the last foreground child process
int commandStatus = 0; // Global variable that keeps track of the last foreground process's status or terminating signal
bool lastForegroundTerminatedSig = false; // Global variable that keeps track of whether or not the last foreground child terminated from signal

/*
* Parse a given command line string and fill the fields in a given
* commandData struct with corresponding values from the command
* line. Perform $$ variable expansion before beginning the parsing.
*/
void parseCommandLine(char *commandLine, struct commandData *shellCommand)
{
    // Make a copy of the original commandLine string to avoid modifying it
    char *expandedCommandLine = calloc(strlen(commandLine) + 1, sizeof(char));
    strcpy(expandedCommandLine, commandLine);

    // Perform $$ variable expansion
    if (commandLine)
    {
        char tempCommandLine[20000]; // Used for temporary storage when formatting the command line
        pid_t smallshPID = getpid(); // Save the PID of the smallsh process itself
        int length = strlen(expandedCommandLine);
        int totalExpansions = 0;
        // Revise each instance of $$ into %d to create a format string to use in sprintf
        for (int i = 0; i < length; i++)
        {
            if (expandedCommandLine[i] == '$' && (i + 1 < length))
            {
                if  (expandedCommandLine[i + 1] == '$')
                {
                    totalExpansions++;
                    expandedCommandLine[i] = '%';
                    expandedCommandLine[i + 1] = 'd';
                }
            }
        }
        
        // Format a temporary command line string to replace all %d with the smallsh pid
        sprintf(tempCommandLine, expandedCommandLine, smallshPID);
        fflush(NULL);

        // Replace the copy of the original commandLine with the newly formatted commandLine
        free(expandedCommandLine);
        expandedCommandLine = calloc(strlen(tempCommandLine) + 1, sizeof(char));
        strcpy(expandedCommandLine, tempCommandLine);
    }

    // Using strtok, parse the command line for command, arguments, and special symbols

    int argindex = 0; // For saving the index in the arguments array

    ssize_t len = 0;
    ssize_t nread;
    char *token;
    
    // Parse the first token, the starting command
    token = strtok(expandedCommandLine, " \n");
    if (!token)
    {
        shellCommand->command = NULL;
        free(token);
        free(expandedCommandLine);
        return;
    }
    else
    { // Store the command into both the command field of shellCommand and the first element of the argument array
        shellCommand->command = calloc(strlen(token) + 1, sizeof(char));
        strcpy(shellCommand->command, token);
        shellCommand->args[0] = shellCommand->command;
        argindex++;
    }
    while (token = strtok(NULL, " \n")) // Go through the rest of the arguments
    {
        if (strcmp(token, "<") == 0)
        {
            token = strtok(NULL, " \n"); // Skip to the next argument, the file
            shellCommand->input_file = calloc(strlen(token) + 1, sizeof(char));
            strcpy(shellCommand->input_file, token); // Store the file as the input file
        }
        else if (strcmp(token, ">") == 0)
        {
            token = strtok(NULL, " \n"); // Skip to the next argument, the file
            shellCommand->output_file = calloc(strlen(token) + 1, sizeof(char));
            strcpy(shellCommand->output_file, token); // Store the file as the output file
        }
        else if (argindex < 512) // Fill the argument array with all remaining arguments
        {
            shellCommand->args[argindex] = calloc(strlen(token) + 1, sizeof(char));
            strcpy(shellCommand->args[argindex], token);
            argindex++;
        }

    }
    shellCommand->args[argindex] = NULL; // Set the array element after the last argument to NULL
    if (strcmp(shellCommand->args[argindex - 1], "&") == 0) // If the last argument is &
    {
        if (backgroundAllowed) // If not in foreground-only mode, set the command to be a background command
        {
            shellCommand->background = true;
        }
        argindex--;
        free(shellCommand->args[argindex]); // Remove the & operator from the arguments array
        shellCommand->args[argindex] = NULL; // Replace that element with NULL as the updated end of the arguments array
    }
    free(token);
    free(expandedCommandLine);
}

/*
* Free all memory used by a given commandData struct
*/
void freeCommand(struct commandData *shellCommand)
{
    for (int i = 0; i < 513; i++) // Free all elements of the argument array
    {
        free(shellCommand->args[i]);
        shellCommand->args[i] = NULL;
    }
    shellCommand->command = NULL; // The command was already free'd from the array freeing
    free(shellCommand->input_file); // Free the input file string
    shellCommand->input_file = NULL;
    free(shellCommand->output_file); // Free the output file string
    shellCommand->output_file = NULL;
    shellCommand->background = false; // Set background bool back to default (false)
}

/*
* Kill all other processes or jobs started by the shell
*/
void killAll(pid_t childProcesses[100], int childProcessIndex)
{
    for (int i = 0; i < childProcessIndex; i++) // Loop through array of child processes
    {
        if (childProcesses[i] != 0)
        {
            kill(childProcesses[i], SIGTERM); // Terminate the process if it exists
        }
    }
}

/*
* Handler for catching SIGTSTP signals, used by main()
*/
void sigtstpHandler(int signal)
{
    // Wait for currently executing foreground process to finish if there is one
    int statusForegroundChild = 0;
    if (lastForegroundChild != 0)
    {
        waitpid(lastForegroundChild, &statusForegroundChild, 0);
    }
    
    // According to what mode (background allowed or foreground-only), print different messages
    if (backgroundAllowed)
    {
        char *message = "\nEntering foreground-only mode (& is now ignored)\n: ";
        write(STDOUT_FILENO, message, 53);
    }
    if (!backgroundAllowed)
    {
        char *message = "\nExiting foreground-only mode\n: ";
        write(STDOUT_FILENO, message, 33);
    }
    // Set backgroundAllowed to opposite
    backgroundAllowed = !backgroundAllowed;
}

/*
* The main function of the program manages all prompting operations and does
* the majority of the work for the shell. It calls the helper functions
* defined above to simplify some operations.
*
* Provides the colon : prompt prepetually until "exit" command is entered,
* creates structs for the command lines, calls helper function to parse the command lines,
* and checks for the three built-in commands before forking off child processes
* and using execvp() to execute other commands. Invokes other functions to help
* take actions based on those commands.
*/
int main()
{
    // Link signal handlers defined above for main process
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, sigtstpHandler);

    // Initialize array of child process pids
    pid_t childProcesses[100];
    int childProcessIndex = 0;
    for (int i = 0; i < 100; i++)
    {
        childProcesses[i] = 0;
    }

    bool exitprogram = false; // This is used to break out of the while loop of execution

    char *commandLine = NULL; // Pointer that is used to save command lines read by getline
    ssize_t len = 0; // For getline

    // Allocate memory for and initialize values for a commandData struct
    struct commandData *shellCommand = malloc(sizeof(struct commandData));
    shellCommand->command = NULL;
    for (int i = 0; i < 513; i++)
    {
        shellCommand->args[i] = NULL;
    }
    shellCommand->input_file = NULL;
    shellCommand->output_file = NULL;
    shellCommand->background = false;

    // Check for the three built-in commands (exit, cd, and status) first.
    while (!exitprogram) // Top-level while loop for program execution
    {
        int childStatus = 0; // For saving background process status
        pid_t childProcess = waitpid(-1, &childStatus, WNOHANG); // For checking background process termination
        if (childProcess == -1 || childProcess == 0) // Before each prompt, check if a child process has terminated
        {
            printf(": "); // Prompt users with colon : prompt
            fflush(NULL);
            ssize_t nread = getline(&commandLine, &len, stdin); // Get user input line
            parseCommandLine(commandLine, shellCommand); // Call parseCommandLine on the user input line to fill shellCommand
            if (!shellCommand->command) // If there was no command entered (i.e. blank line), free the shellCommand using the helper function defined above
            {
                freeCommand(shellCommand);
                continue;
            }
            else if (shellCommand->command[0] == '#') // If the first command starts with the character #, it's a comment line
            {
                freeCommand(shellCommand);
                continue;
            }
            else if (strcmp(shellCommand->command, "exit") == 0) // If the first command is exit, change the exitProgram bool and escape the control flow
            {
                exitprogram = true;
                freeCommand(shellCommand);
                break;
            }
            else if (strcmp(shellCommand->command, "cd") == 0) // If the first command is cd, handle it
            {
                int changedir = -1;
                if (!shellCommand->args[1]) // If there are no more arguments, change to directory specified in HOME environment variable
                {
                    changedir = chdir(getenv("HOME"));
                }
                else if (shellCommand->args[1]) // If there is a directory argument, pass it to chdir
                {
                    char *path = shellCommand->args[1];
                    changedir = chdir(path);
                }
                if (changedir != 0) // If chdir returned an error, print an error message
                {
                    printf("Incorrect syntax for cd.\n");
                    fflush(NULL);
                }
                freeCommand(shellCommand);
                continue;
            }
            else if (strcmp(shellCommand->command, "status") == 0) // If the command was status, print the exit status or termination signal of the last foreground command
            {
                if (!lastForegroundTerminatedSig)
                {
                    printf("exit value %d\n", commandStatus);
                    fflush(NULL);
                }
                else if (lastForegroundTerminatedSig)
                {
                    printf("terminated by signal %d\n", commandStatus);
                    fflush(NULL);
                }

                freeCommand(shellCommand);
                continue;
            }
            else // If the command given is not one of the built-in commands
            {
                bool inBackground = false; // By default, consider new process to be in foreground
                
                if (backgroundAllowed) // If not in foreground-only mode, update inBackground based on shellCommand's background value
                {
                    inBackground = shellCommand->background;
                }

                int status = 0; // To use with waitpid for foreground children

                pid_t child = fork(); // Fork off a child process

                if (child == 0) // If inside of the child process
                {
                    signal(SIGTSTP, SIG_IGN); // Make child processes ignore SIGTSTP

                    if (inBackground) // If the child is a background process
                    {
                        // Redirect input and output to /dev/null by default
                        int devNull = open("/dev/null", O_RDWR);
                        int inputDirection = dup2(devNull, 0);
                        int OutputDirection = dup2(devNull, 1);
                        close(devNull);

                        // Ignore SIGINT
                        signal(SIGINT, SIG_IGN);
                    }
                    else // If the child is a foreground process
                    {
                        // Use default SIGINT behavior (terminate)
                        signal(SIGINT, SIG_DFL);
                    }

                    // Redirect input and output to any user-specified files if they exist
                    if (shellCommand->input_file)
                    {
                        int input = open(shellCommand->input_file, O_RDONLY);
                        if (input == -1) // Print error message if file can't be opened
                        {
                            printf("cannot open %s for input\n", shellCommand->input_file);
                            fflush(NULL);
                            exit(1);
                        }
                        int inputDirection = dup2(input, 0); // If file can be opened, redirect input
                        close(input);
                    }
                    if (shellCommand->output_file)
                    {
                        int output = open(shellCommand->output_file, O_RDWR | O_CREAT | O_TRUNC, 0660);
                        if (output == -1) // Print error message if file can't be opened
                        {
                            printf("cannot open %s for output\n", shellCommand->output_file);
                            fflush(NULL);
                            exit(1);
                        }
                        int outputDirection = dup2(output, 1); // If file can be opened, redirect output
                        close(output);
                    }

                    // Attempt to call execvp on the command
                    if (execvp(shellCommand->command, shellCommand->args)) // If the operation failed, print an error message
                    {
                        // Restore stdin and stdout
                        int restoreInput = dup2(dup(0), 0);
                        int restoreOutput = dup2(dup(1), 1);

                        printf("%s: no such file or directory\n", shellCommand->command);
                        fflush(NULL);
                        exit(1); // Terminate with status 1
                    }
                    
                    // Restore stdin and stdout
                    int restoreInput = dup2(dup(0), 0);
                    int restoreOutput = dup2(dup(1), 1);

                    exit(0); // Since the operation was successful, terminate with status 0.
                }
                else // If in parent process
                {
                    // Store the child process ID into the array of child process PIDs
                    int thisChildIndex = childProcessIndex;
                    childProcesses[thisChildIndex] = child;
                    childProcessIndex++; // Update the overall array index

                    if (inBackground) // If the child is a background process, print its PID information
                    {
                        printf("background pid is %d\n", child);
                        fflush(NULL);
                    }
                    else // If the child is a foreground process
                    {
                        // Record the new child process as last foreground child
                        lastForegroundChild = child;

                        // Suspend user control until foreground process terminates
                        pid_t wait = waitpid(child, &status, 0);
                        if (WIFEXITED(status))
                        {
                            commandStatus = WEXITSTATUS(status); // Set the commandStatus to the exit status of the foreground process
                            lastForegroundTerminatedSig = false; // The process did not terminate from a signal
                        }
                        else if (WIFSIGNALED(status))
                        {
                            commandStatus = WTERMSIG(status); // Set the commandStatus to the terminating signal of the foreground process
                            lastForegroundTerminatedSig = true; // The process terminated from a signal
                            char *message = "\nterminated by signal 2\n"; // Print out SIGINIT termination information
                            write(STDOUT_FILENO, message, 24);
                        }
                    }
                }
            }
            freeCommand(shellCommand);
        }
        else // If a background process has terminated
        {
            if (WIFEXITED(childStatus)) // If the terminated child was a background process terminating normally
            {
                // Print the exit status of the background process
                childStatus = WEXITSTATUS(childStatus);
                printf("background pid %d is done: exit value %d\n", childProcess, childStatus);
                fflush(NULL);
            }
            else if (WIFSIGNALED(childStatus)) // If the terminated child terminated due to a signal
            {
                // Print the signal termination status of the background process
                childStatus = WTERMSIG(childStatus);
                printf("background pid %d is done: terminated by signal %d\n", childProcess, childStatus);
                fflush(NULL);
            }
        }
    }
    // exit has been called
    
    free(shellCommand);
    free(commandLine);
    // Kill all remaining child processes using the killAll function defined above
    killAll(childProcesses, childProcessIndex);
    return EXIT_SUCCESS;
}
