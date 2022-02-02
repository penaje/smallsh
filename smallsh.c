#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>


//Struct for storing commands from the user
struct commandStruct {
    char* command;
    char* args[514];
    char* iRedirect;
    char* oRedirect;
    bool background;
};

struct commandStruct* getInput(char* currLine)
{
    struct commandStruct* newCommand = malloc(sizeof(struct commandStruct));

    //for use with strtok_r
    char* saveptr;

    //the token will be the command
    char* token = strtok_r(currLine, " ", &saveptr);

    //If there is no command we return NULL
    if (token != NULL)
    {
       newCommand->command = calloc(strlen(token) + 1, sizeof(char));
       strcpy(newCommand->command, token);
    }
    else
    {
        return token;
    }  

    //Now we get the arguments
    token = strtok_r(NULL, " ", &saveptr);
    int i = 0;
    newCommand->args[0] = calloc(strlen(token) + 1, sizeof(char));
    strcpy(newCommand->args[0], token);
    

    while ((token != NULL) && (strcmp(token, "<") != 0) && (strcmp(token, ">") != 0) && (strcmp(token, "&") != 0))
    {
        newCommand->args[i] = calloc(strlen(token) + 1, sizeof(char));
        strcpy(newCommand->args[i], token);
        i++;
        token = strtok_r(NULL, " ", &saveptr);
    }

    //ends the array with a NULL pointer
    newCommand->args[i] = malloc(sizeof(char));
    newCommand->args[i] = NULL;







}