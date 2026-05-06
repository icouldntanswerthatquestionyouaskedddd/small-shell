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
    ssize_t len = 0;
    ssize_t nread;
    char *token;
    
    // Parse the first token, the starting command
    token = strtok(commandLine, " \n");
    if (!token)
    {
        shellCommand->command = NULL;
    }
    else
    {
        shellCommand->command = calloc(strlen(token) + 1, sizeof(char));
        strcpy(shellCommand->command, token);
    }
    
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
        
    }
    killAll();
    return EXIT_SUCCESS;
}