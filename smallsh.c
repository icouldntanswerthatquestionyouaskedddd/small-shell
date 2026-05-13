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

/*
* Parse a given command line string and fill the fields in a
* commandData struct with corresponding values from the command
* line
*/
void parseCommandLine(char *commandLine, struct commandData *shellCommand)
{
    int argindex = 0; // For saving the index in the arguments array

    ssize_t len = 0;
    ssize_t nread;
    char *token;
    
    // Parse the first token, the starting command
    token = strtok(commandLine, " \n");
    if (!token)
    {
        shellCommand->command = NULL;
        free(token);
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
        if (strcmp(token, "&") == 0)
        {
            shellCommand->background = true;
        }
        else if (strcmp(token, "<") == 0)
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
    free(token);
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
void killAll()
{
    
}

void sigtstpHandler(int signal)
{
    if (backgroundAllowed)
    {
        printf("\nEntering foreground-only mode (& is now ignored)\n: ");
        fflush(NULL);
    }
    if (!backgroundAllowed)
    {
        printf("\nExiting foreground-only mode\n: ");
        fflush(NULL);
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
            printf("\nterminated by signal %d\n", childStatus);
            fflush(NULL);
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

                freeCommand(shellCommand);
                continue;
            }
            else
            {
                bool inBackground = shellCommand->background;

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
                    }
                    if (shellCommand->output_file)
                    {
                        int output = open(shellCommand->output_file, O_RDWR | O_CREAT, O_TRUNC);
                        if (output == -1)
                        {
                            printf("cannot open %s for output\n", shellCommand->output_file);
                            fflush(NULL);
                            exit(1);
                        }
                        int outputDirection = dup2(output, 1);
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
                    if (inBackground)
                    {
                        printf("background pid is %d\n", child);
                        fflush(NULL);
                    }
                    else
                    {
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
        }
    }
    free(shellCommand);
    free(commandLine);
    killAll();
    return EXIT_SUCCESS;
}
