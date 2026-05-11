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

struct commandData
{
    char *command;
    char *args[514]; // Make space for the command, then 512 arguments, plus on more space for a NULL value
    char *input_file;
    char *output_file;
    bool background;
};

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
            token = strtok(commandLine, " \n");
            shellCommand->input_file = calloc(strlen(token) + 1, sizeof(char));
            strcpy(shellCommand->input_file, token);
        }
        else if (strcmp(token, ">") == 0)
        {
            token = strtok(commandLine, " \n");
            shellCommand->output_file = calloc(strlen(token) + 1, sizeof(char));
            strcpy(shellCommand->output_file, token);
        }
        else if (argindex < 513)
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
    for (int i = 0; i < 514; i++)
    {
        free(shellCommand->args[i]);
        shellCommand->args[i] = NULL;
    }
    shellCommand->command = NULL;
    free(shellCommand->input_file);
    shellCommand->input_file = NULL;
    free(shellCommand->output_file);
    shellCommand->output_file = NULL;
}

/*
* Kill all other processes or jobs started by the shell
*/
void killAll()
{

}

/*
* Provides the colon : prompt prepetually until "exit" command is entered,
* and checks for the three built-in commands before parsing the command line
* for other commands. Invokes other functions to take actions based on
* those commands. Creates structs for the command lines for other commands.
*/
int main()
{
    bool exitprogram = false;

    char *commandLine = NULL;
    ssize_t len = 0;

    struct commandData *shellCommand = malloc(sizeof(struct commandData));
    shellCommand->command = NULL;
    for (int i = 0; i < 514; i++)
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
            // TODO: STATUS

            printf("exit value %d\n", commandStatus);

            freeCommand(shellCommand);
            continue;
        }
        else
        {
            // TODO: Output redirection

            int status = 0;

            pid_t child = fork();

            if (child == 0)
            {
                if (execvp(shellCommand->command, shellCommand->args))
                {
                    printf("Cannot find command '%s'.\n", shellCommand->command);
                    fflush(NULL);
                    exit(1);
                }
                exit(0);
            }
            else
            {
                pid_t wait = waitpid(child, &status, 0);
                commandStatus = WEXITSTATUS(status);
            }
        }
        freeCommand(shellCommand);
    }
    free(shellCommand);
    free(commandLine);
    killAll();
    return EXIT_SUCCESS;
}
