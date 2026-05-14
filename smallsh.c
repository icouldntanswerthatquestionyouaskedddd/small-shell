/*
* Operating Systems I
* Assignment 3: smallsh
* By Novia Weng.
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

struct commandData
{
    char *command;
    char *args[513]; // Make space for the command, then the other 511 arguments, plus on more space for a NULL value
    char *input_file;
    char *output_file;
    bool background;
};

static bool backgroundAllowed = true;
static pid_t lastForegroundChild = 0;


/*
* Parse a given command line string and fill the fields in a
* commandData struct with corresponding values from the command
* line. Perform $$ variable expansion before beginning the parsing.
*/
void parseCommandLine(char *commandLine, struct commandData *shellCommand)
{
    char *expandedCommandLine = calloc(strlen(commandLine) + 1, sizeof(char));
    strcpy(expandedCommandLine, commandLine);
    // Perform $$ variable expansion
    if (commandLine)
    {
        char tempCommandLine[20000];
        pid_t smallshPID = getpid();
        int length = strlen(expandedCommandLine);
        int totalExpansions = 0;
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
        
        sprintf(tempCommandLine, expandedCommandLine, smallshPID);
        fflush(NULL);

        free(expandedCommandLine);
        expandedCommandLine = calloc(strlen(tempCommandLine) + 1, sizeof(char));
        strcpy(expandedCommandLine, tempCommandLine);
    }

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
    {
        shellCommand->command = calloc(strlen(token) + 1, sizeof(char));
        strcpy(shellCommand->command, token);
        shellCommand->args[0] = shellCommand->command;
        argindex++;
    }
    while (token = strtok(NULL, " \n"))
    {
        if (strcmp(token, "<") == 0)
        {
            token = strtok(NULL, " \n");
            shellCommand->input_file = calloc(strlen(token) + 1, sizeof(char));
            strcpy(shellCommand->input_file, token);
        }
        else if (strcmp(token, ">") == 0)
        {
            token = strtok(NULL, " \n");
            shellCommand->output_file = calloc(strlen(token) + 1, sizeof(char));
            strcpy(shellCommand->output_file, token);
        }
        else if (argindex < 512)
        {
            shellCommand->args[argindex] = calloc(strlen(token) + 1, sizeof(char));
            strcpy(shellCommand->args[argindex], token);
            argindex++;
        }

    }
    shellCommand->args[argindex] = NULL;
    if (strcmp(shellCommand->args[argindex - 1], "&") == 0) // If the last argument is &
    {
        if (backgroundAllowed)
        {
            shellCommand->background = true;
        }
        argindex--;
        free(shellCommand->args[argindex]);
        shellCommand->args[argindex] = NULL;
    }
    free(token);
    free(expandedCommandLine);
}

/*
* Free all memory used by a given commandData struct
*/
void freeCommand(struct commandData *shellCommand)
{
    for (int i = 0; i < 513; i++)
    {
        free(shellCommand->args[i]);
        shellCommand->args[i] = NULL;
    }
    shellCommand->command = NULL;
    free(shellCommand->input_file);
    shellCommand->input_file = NULL;
    free(shellCommand->output_file);
    shellCommand->output_file = NULL;
    shellCommand->background = false;
}

/*
* Kill all other processes or jobs started by the shell
*/
void killAll(pid_t childProcesses[100], int childProcessIndex)
{
    for (int i = 0; i < childProcessIndex; i++)
    {
        fflush(NULL);
        if (childProcesses[i] != 0)
        {
            kill(childProcesses[i], SIGTERM);
        }
    }
}

void sigtstpHandler(int signal)
{
    int statusForegroundChild = 0;
    if (lastForegroundChild != 0)
    {
        waitpid(lastForegroundChild, &statusForegroundChild, 0);
    }
    
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
    backgroundAllowed = !backgroundAllowed;
}

void sigintHandler(int signal)
{
    int childStatus = 0;
    pid_t childProcess = 0;
    while (childProcess == 0)
    {
        childProcess = waitpid(-1, &childStatus, WNOHANG);
        if (WIFSIGNALED(childStatus))
        {
            childStatus = WTERMSIG(childStatus);
            char *message = "\nterminated by signal 2\n";
            write(STDOUT_FILENO, message, 24);
        }
    }
}

/*
* Provides the colon : prompt prepetually until "exit" command is entered,
* and checks for the three built-in commands before parsing the command line
* for other commands. Invokes other functions to take actions based on
* those commands. Creates structs for the command lines for other commands.
*/
int main()
{
    signal(SIGINT, sigintHandler);
    signal(SIGTSTP, sigtstpHandler);

    pid_t childProcesses[100];
    int childProcessIndex = 0;
    for (int i = 0; i < 100; i++)
    {
        childProcesses[i] = 0;
    }

    bool exitprogram = false;

    char *commandLine = NULL;
    ssize_t len = 0;

    struct commandData *shellCommand = malloc(sizeof(struct commandData));
    shellCommand->command = NULL;
    for (int i = 0; i < 513; i++)
    {
        shellCommand->args[i] = NULL;
    }
    shellCommand->input_file = NULL;
    shellCommand->output_file = NULL;
    shellCommand->background = false;

    int commandStatus = 0;

    // Check of the three built-in commands (exit, cd, and status) first.
    while (!exitprogram)
    {
        int childStatus = 0;
        pid_t childProcess = waitpid(-1, &childStatus, WNOHANG);
        if (childProcess == -1 || childProcess == 0)
        {
            printf(": ");
            fflush(NULL);
            ssize_t nread = getline(&commandLine, &len, stdin);
            parseCommandLine(commandLine, shellCommand);
            if (!shellCommand->command)
            {
                freeCommand(shellCommand);
                continue;
            }
            else if (shellCommand->command[0] == '#')
            {
                freeCommand(shellCommand);
                continue;
            }
            else if (strcmp(shellCommand->command, "exit") == 0)
            {
                exitprogram = true;
                freeCommand(shellCommand);
                break;
            }
            else if (strcmp(shellCommand->command, "cd") == 0)
            {
                int changedir = -1;
                if (!shellCommand->args[1])
                {
                    changedir = chdir(getenv("HOME"));
                }
                else if (shellCommand->args[1])
                {
                    char *path = shellCommand->args[1];
                    changedir = chdir(path);
                }
                if (changedir != 0)
                {
                    printf("Incorrect syntax for cd.\n");
                    fflush(NULL);
                }
                freeCommand(shellCommand);
                continue;
            }
            else if (strcmp(shellCommand->command, "status") == 0)
            {
                printf("exit value %d\n", commandStatus);
                fflush(NULL);

                freeCommand(shellCommand);
                continue;
            }
            else
            {
                bool inBackground = false;
                
                if (backgroundAllowed)
                {
                    inBackground = shellCommand->background;
                }

                int status = 0;

                pid_t child = fork();

                if (child == 0)
                {
                    signal(SIGTSTP, SIG_IGN);

                    if (inBackground)
                    {
                        // Redirect input and output to /dev/null by default
                        int devNull = open("/dev/null", O_RDWR);
                        int inputDirection = dup2(devNull, 0);
                        int OutputDirection = dup2(devNull, 1);
                        close(devNull);

                        // Ignore SIGINT
                        signal(SIGINT, SIG_IGN);
                    }
                    else
                    {
                        // Use default SIGINT behavior (terminate)
                        signal(SIGINT, SIG_DFL);
                    }

                    // Redirect input and output to any user-specified files
                    if (shellCommand->input_file)
                    {
                        int input = open(shellCommand->input_file, O_RDONLY);
                        if (input == -1)
                        {
                            printf("cannot open %s for input\n", shellCommand->input_file);
                            fflush(NULL);
                            exit(1);
                        }
                        int inputDirection = dup2(input, 0);
                        close(input);
                    }
                    if (shellCommand->output_file)
                    {
                        int output = open(shellCommand->output_file, O_RDWR | O_CREAT | O_TRUNC, 0660);
                        if (output == -1)
                        {
                            printf("cannot open %s for output\n", shellCommand->output_file);
                            fflush(NULL);
                            exit(1);
                        }
                        int outputDirection = dup2(output, 1);
                        close(output);
                    }

                    if (execvp(shellCommand->command, shellCommand->args))
                    {
                        int restoreInput = dup2(dup(0), 0);
                        int restoreOutput = dup2(dup(1), 1);
                        printf("%s: no such file or directory\n", shellCommand->command);
                        fflush(NULL);
                        exit(1);
                    }
                    
                    // Restore stdin and stdout
                    int restoreInput = dup2(dup(0), 0);
                    int restoreOutput = dup2(dup(1), 1);

                    exit(0);
                }
                else
                {
                    int thisChildIndex = childProcessIndex;
                    childProcesses[thisChildIndex] = child;
                    childProcessIndex++;

                    if (inBackground)
                    {
                        printf("background pid is %d\n", child);
                        fflush(NULL);
                    }
                    else
                    {
                        // Record the new child process as last foreground child
                        lastForegroundChild = child;

                        // Suspend user control until foreground process terminates
                        pid_t wait = waitpid(child, &status, 0);
                    }
                    commandStatus = WEXITSTATUS(status);
                }
            }
            freeCommand(shellCommand);
        }
        else
        {
            if (WIFEXITED(childStatus)) // If the terminated child was a background process terminating normally
            {
                childStatus = WEXITSTATUS(childStatus);
                printf("background pid %d is done: exit value %d\n", childProcess, childStatus);
                fflush(NULL);
            }
            if (WIFSIGNALED(childStatus)) // If the terminated child terminated due to a signal
            {
                childStatus = WTERMSIG(childStatus);
                printf("background pid %d is done: terminated by signal %d\n", childProcess, childStatus);
                fflush(NULL);
            }
        }
    }
    free(shellCommand);
    free(commandLine);
    killAll(childProcesses, childProcessIndex);
    return EXIT_SUCCESS;
}
