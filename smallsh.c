#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <limits.h>

#ifndef MAX_BUF
#define MAX_BUF 200
#endif // !MAX_BUFF

//holds the process id's that are running
pid_t backgroundPids[201];

//background process count starts at 0
int bgCount = 0;



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

        //first arg needs to be the name of the command
        newCommand->args[0] = calloc(strlen(token) + 1, sizeof(char));
        strcpy(newCommand->args[0], token);
    }  

    //advance the token
    token = strtok_r(NULL, " \n", &saveptr);

    //if no args or redirect or BG then we just return the command
    if (token == NULL)
    {
        return newCommand;
    }

    //Now we get the arguments
    int i = 1;
    newCommand->args[1] = calloc(strlen(token) + 1, sizeof(char));
    strcpy(newCommand->args[1], token); 
     

    while ((token != NULL) && (strcmp(token, "<") != 0) && (strcmp(token, ">") != 0) && (strcmp(token, "&") != 0))
    {
        newCommand->args[i] = calloc(strlen(token) + 1, sizeof(char));
        strcpy(newCommand->args[i], token);
        i++;
        token = strtok_r(NULL, " \n", &saveptr);
    }

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

//Algorithm taken from https://www.tutorialspoint.com/c-program-to-replace-a-word-in-a-text-by-another-given-word
char* varExpansion(char* inputString)
{
    char pidVar[] = "$$";

    //get the process id as an int
    int process_id = getpid();

    // The below 3 lines of code are from https://stackoverflow.com/questions/5242524/converting-int-to-string-in-c
    int length = snprintf(NULL, 0, "%d", process_id);
    char* str = malloc(length + 1);

    //cast the Pid into the str string
    snprintf(str, length + 1, "%d", process_id);

    //replace '$$' with the PiD string
    int i = 0, cnt = 0;
    int new_len = strlen(str);
    int old_len = strlen(pidVar);

    for (i = 0; inputString[i] != '\0'; i++)
    {
        if (strstr(&inputString, pidVar) == &inputString[i])
        {
            cnt++;
            i += old_len - 1;
        }
    }

    char* expCommand = (char*)malloc(i + cnt * (new_len - old_len) + 1);
    i = 0;

    while (*inputString)
    {
        if (strstr(inputString, pidVar) == inputString)
        {
            strcpy(&expCommand[i], str);
            i += new_len;
            inputString += old_len;
        }
        else
        {
            expCommand[i++] = *inputString++;
        }
    }

    //frees the dynamically allocated memory
    free(str);

    //printf("Expanded Command is: %s\n", expCommand);
    return(expCommand);
}

char* getInput() 
{
    printf(": ");
    fflush(stdout);


    // stores the input from the user
    char* inputText = calloc(2049, sizeof(char));
    fgets(inputText, 2048, stdin);


    //call the expand function to account for the "$$"
    char* expandedInput;
    expandedInput = varExpansion(inputText);

    return expandedInput;
}

void printCommand(struct commandStruct* aCommand) 
{
    printf("command: %s | inputRedir: %s | OutputRedir: %s | Background: %d \n", 
        aCommand->command,
        aCommand->inpRedir,
        aCommand->outpRedir,
        aCommand->background);

    fflush(stdout);

    int i = 0;
    while (aCommand->args[i] != NULL)
    {
        printf("args[%d]: %s \n", i, aCommand->args[i]);
        i++;
    }
    fflush(stdout);

}

//Assuming this will be executed from mainScreen, so we will pass it the whole command struct
void changeDir(struct commandStruct* aCommand)
{
    //checks for an argument
    if (aCommand->args[1] == NULL)
    {
        chdir(getenv("HOME"));
        char currDir[MAX_BUF];
        getcwd(currDir, MAX_BUF);
        printf("Current working directory: %s\n", currDir);
        fflush(stdout);
    }
    else
    {
        if (chdir(aCommand->args[1]) != 0)
        {
            printf("Error occured, directory not changed\n");
            fflush(stdout);
        }
        else
        {
            char currDir[MAX_BUF];
            getcwd(currDir, MAX_BUF);
            printf("Current working directory: %s\n", currDir);
            fflush(stdout);
        }
    }
}

//adds the Pid to the list of running processes
//may modify this later 
void addBackgroundPid(pid_t pidNum)
{
    backgroundPids[bgCount] = pidNum;
    bgCount++;
}

//TODO: This is not working....
//checks if any processes have terminated
//need to handle -1
void backgroundCheck(pid_t backgroundPids[])
{
    printf("Printing out bg processes!\n");
    fflush(stdout);
    int j = 0;
    while (backgroundPids[j] != NULL)
    {
        printf("%d\n", backgroundPids[j]);
        fflush(stdout);
        j++;
    }
    printf("Thats all the processes\n");
    fflush(stdout);

    int childExitMethod;

    //wait for ANY child processes
    pid_t childPID = waitpid(-1, &childExitMethod, WNOHANG);


    //if no error and child has terminated
    if (childPID > 0)
    {
        if (WIFEXITED(childExitMethod) != 0)
        {
            printf("background pid %d is done: ", childPID);
            fflush(stdout);
            int exitStatus = WEXITSTATUS(childExitMethod);
            printf("exit value %d\n", exitStatus);
            fflush(stdout);
        }
        else if (WIFSIGNALED(childExitMethod) != 0)
        {
            printf("background pid %d is done: ", childPID);
            fflush(stdout);
            int termSignal = WTERMSIG(childExitMethod);
            printf("terminated by signal %d\n", termSignal);
            fflush(stdout);
        }
        
    }

    //Can I use NULL in an array of pid_t's???
    for (int i = 0; i < 200; i++)
    {
        if (backgroundPids[i] == childPID)
        {
            backgroundPids[i] = NULL;
        }
    }

    //for testing
    printf("Printing out remaining processes!\n");
    fflush(stdout);
    int k = 0;
    while (backgroundPids[k] != NULL)
    {
        printf("%d\n", backgroundPids[k]);
        fflush(stdout);
        k++;
    }
    printf("Thats all the processes\n");
    fflush(stdout);
}


//TODO: NEED TO TEST THIS
void exitShell()
{
    int i;

    //iterate through the child processes and terminate them if they have not completed
    for (i = bgCount; i > 0; i--)
    {
        int childExitMethod;
        pid_t childPID;

        //if child has not completed then we terminate
        if (waitpid(backgroundPids[i], &childExitMethod, WNOHANG) == 0)
        {
            //call SIGTERM
            kill(backgroundPids[i], 15);
            i--;
        }
        else
        {
            i--;
        }
        
    }
    //do I need this?
    exit(0);
}


//Testing this
void execCommand(struct commandStruct* aCommand)
{
    pid_t spawnPid = -5;
    int childExitStatus;

    spawnPid = fork();

    //add the new Pid to list of background Pids
    addBackgroundPid(spawnPid);

    switch (spawnPid)
    {
        case -1:
        {
            perror("Error with forking!\n");
            fflush(stdout);
            exit(1);
            break;
        }
        case 0:
        {
            addBackgroundPid(getpid());
            printf("CHILD '%d': Sleep for 1 sec\n", getpid());
            fflush(stdout);
            sleep(1);
            printf("CHILD '%d': Converting into '%s'\n", getpid(), aCommand->command);
            fflush(stdout);
            if (execvp(aCommand->command, aCommand->args) < 0)
            {
                perror("Exec Failure!\n");
                fflush(stdout);
                exit(1);
            }               
        }
        default:
        {
            printf("PARENT '%d: Sleeping for 2 seconds\n", getpid());
            fflush(stdout);
            sleep(2);
            printf("PARENT '%d: Waiting for child '%d' to terminate\n", getpid(), spawnPid);
            fflush(stdout);


            pid_t actualPid = waitpid(spawnPid, &childExitStatus, 0);
            printf("PARENT '%d': Child '%d' terminated, Exiting!\n", getpid(), actualPid);
            fflush(stdout);

            //tell us why the child exited/terminated
            if (WIFEXITED(childExitStatus) != 0)
            {
                printf("background pid %d is done: ", spawnPid);
                fflush(stdout);
                int exitStatus = WEXITSTATUS(childExitStatus);
                printf("exit value %d\n", exitStatus);
                fflush(stdout);
            }
            else if (WIFSIGNALED(childExitStatus) != 0)
            {
                printf("background pid %d is done: ", spawnPid);
                fflush(stdout);
                int termSignal = WTERMSIG(childExitStatus);
                printf("terminated by signal %d\n", termSignal);
                fflush(stdout);
            }
            break;
        }
    }
}

void mainScreen(char* statusString, int statusCode)
{
    char* input;
    struct commandStruct* commandLine;
    input = getInput();
    commandLine = parseInput(input, fgFlag);

    //if user enters nothing or '#' then we restart
    if ((commandLine->command == NULL) || (strcmp(commandLine->command, "#") == 0) || (strcmp(commandLine->command, " \n") == 0))
    {
        mainScreen(statusString, statusCode);
    }

    //If user enters "cd"
    else if (strcmp(commandLine->command, "cd") == 0)
        {
            changeDir(commandLine);
        }

    else if (strcmp(commandLine->command, "status") == 0)
    {
        printf("%s %d\n", statusString, statusCode);
        fflush(stdout);
    }
    else if (strcmp(commandLine->command, "exit") == 0)
    {
        exitShell();
    }
    else
    //use execvp to run commands as child
    {
        printCommand(commandLine);
        execCommand(commandLine);
    }

    //backgroundCheck(backgroundPids);
    exit(1);
}

int main(void)
{
    //initalize the status variables for use in Status command
    char statusString[30];
    strcpy(statusString, "exit value");
    int statusCode = 0;

    printf("$ smallsh\n");
    fflush(stdout);

    mainScreen(statusString, statusCode);

    return 0;

}