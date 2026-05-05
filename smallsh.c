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

void killAll()
{

}

int main()
{
    bool exit = false;
    while (!exit)
    {
        printf(": ");
        fflush(NULL);
        char *commandLine;
        ssize_t len = 0;
        ssize_t nread = getline(&commandLine, &len, stdin);
        if (strcmp(commandLine, "exit\n") == 0)
        {
            bool exit = true;
            break;
        }
    }
    killAll();
    return EXIT_SUCCESS;
}