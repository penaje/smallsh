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
    char* inpRedir;
    char* outpRedir;
    bool background;
};

bool fgFlag = false;

struct commandStruct* parseInput(char* currLine, bool fgFlag)
{
    struct commandStruct* newCommand = malloc(sizeof(struct commandStruct));

    //for use with strtok_r
    char* saveptr;

    //the first token will be the command
    char* token = strtok_r(currLine, " \n", &saveptr);

    //If there is no command we return NULL
    if (token == NULL)
    {
        return NULL;
    }
    else
    {
        newCommand->command = calloc(strlen(token) + 1, sizeof(char));
        strcpy(newCommand->command, token);
    }  

    //advance the token
    token = strtok_r(NULL, " \n", &saveptr);

    //if no args or redirect or BG then we just return the command
    if (token == NULL)
    {
        return newCommand;
    }

    //Now we get the arguments
    int i = 0;
    newCommand->args[0] = calloc(strlen(token) + 1, sizeof(char));
    strcpy(newCommand->args[0], token); 
     

    while ((token != NULL) && (strcmp(token, "<") != 0) && (strcmp(token, ">") != 0) && (strcmp(token, "&") != 0))
    {
        newCommand->args[i] = calloc(strlen(token) + 1, sizeof(char));
        strcpy(newCommand->args[i], token);
        i++;
        token = strtok_r(NULL, " \n", &saveptr);
    }

    //ends the array with a NULL pointer
    newCommand->args[i] = malloc(sizeof(char));
    newCommand->args[i] = NULL;

    while (token != NULL)
    {
        if (strcmp(token, "<") == 0)
        {
            token = strtok_r(NULL, " \n", &saveptr);
            newCommand->inpRedir = calloc(strlen(token) + 1, sizeof(char));
            strcpy(newCommand->inpRedir, token);
        }
        else if (strcmp(token, ">") == 0)
        {
            token = strtok_r(NULL, " \n", &saveptr);
            newCommand->outpRedir = calloc(strlen(token) + 1, sizeof(char));
            strcpy(newCommand->outpRedir, token);
        }
        else if ((strcmp(token, "&") == 0) && (fgFlag == false))
        {
            newCommand->background = true;
        }

        token = strtok_r(NULL, " \n", &saveptr);
    }

    return newCommand;

}

char* getInput() 
{
    // Displays the command line prompt
    printf(": ");
    fflush(stdout);

    // Stores input from stdin up to 2048 characters or until a new line character
    char* inputText = calloc(2049, sizeof(char));
    fgets(inputText, 2048, stdin);

    return inputText;
}

void printCommand(struct commandStruct* aCommand) {
    printf("command: %s | inputRedir: %s | OutputRedir: %s | Background: %d | ", 
        aCommand->command,
        aCommand->inpRedir,
        aCommand->outpRedir,
        aCommand->background);

    int i = 0;
    while (aCommand->args[i] != NULL)
    {
        printf("args[%d]: %s ", i, aCommand->args[i]);
        i++;
    }
    printf("\n");
}

int main(void)
{
    printf("$ smallsh\n");
    fflush(stdout);

    char* input;
    struct commandStruct* commandLine;

    input = getInput();

    commandLine = parseInput(input, fgFlag);

    if (commandLine != NULL)
    {
        printCommand(commandLine);
    }
    else
    {
        input = getInput();
    }
    

    return 0;

}