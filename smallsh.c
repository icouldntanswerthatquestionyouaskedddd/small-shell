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

struct commandData
{
    char *command;
    char *args[512];
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
        return;
    }
    else
    {
        shellCommand->command = calloc(strlen(token) + 1, sizeof(char));
        strcpy(shellCommand->command, token);
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
        else if (argindex < 512)
        {
            shellCommand->args[argindex] = calloc(strlen(token) + 1, sizeof(char));
            strcpy(shellCommand->args[argindex], token);
            argindex++;
        }

    }
    
}

/*
* Free all memory used by a given commandData struct
*/
void freeCommand(struct commandData *shellCommand)
{
    if (shellCommand->command)
    {
        free(shellCommand->command);
    }
    shellCommand->command = NULL;
    for (int i = 0; i < 512; i++)
    {
        if (shellCommand->args[i])
        {
            free(shellCommand->args[i]);
        }
        shellCommand->args[i] = NULL;
    }
    if (shellCommand->input_file)
    {
        free(shellCommand->input_file);
    }
    shellCommand->input_file = NULL;
    if (shellCommand->output_file)
    {
        free(shellCommand->output_file);
    }
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
    bool exit = false;

    // Check of the three built-in commands (exit, cd, and status) first.
    while (!exit)
    {
        printf(": ");
        fflush(NULL);
        char *commandLine = NULL;
        ssize_t len = 0;
        ssize_t nread = getline(&commandLine, &len, stdin);
        struct commandData *shellCommand = malloc(sizeof(struct commandData));
        parseCommandLine(commandLine, shellCommand);
        if (!shellCommand->command)
        {
            continue;
        }
        else if (shellCommand->command[0] == '#')
        {
            continue;
        }
        else if (strcmp(shellCommand->command, "exit") == 0)
        {
            bool exit = true;
            break;
        }
        else if (strcmp(shellCommand->command, "cd") == 0)
        {

            continue;
        }
        else if (strcmp(shellCommand->command, "status") == 0)
        {

            continue;
        }
        else
        {

        }
        freeCommand(shellCommand);
    }
    killAll();
    return EXIT_SUCCESS;
}