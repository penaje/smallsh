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
//Using a max of 200 https://edstem.org/us/courses/16718/discussion/1080332
pid_t backgroundPids[200];



//Struct for storing commands from the user
struct commandStruct {
    char* command;
    char* args[514];
    char* inpRedir;
    char* outpRedir;
    bool background;
};

//Used for handling sigtstp
bool fgFlag = false;


/* Parse the command the user types in and create a struct with the information
*/
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

    //Now we get the arguments, start at index 1 since index 0 is the command
    int i = 1;

    //if no args or redirect or BG then we just return the command
    if (token == NULL)
    {
        return newCommand;
    } 

    //ignore input redirect & output redirect symbols
    while ((token != NULL) && (strcmp(token, "<") != 0) && (strcmp(token, ">") != 0))
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
        token = strtok_r(NULL, " \n", &saveptr);
    }

    // check if '&' was the last argument
     if ((strcmp(newCommand->args[(i - 1)], "&") == 0) && (fgFlag == false))
        {
            newCommand->background = true;
            newCommand->args[(i - 1)] = NULL;
        }

     //if in fg only mode then we don't set background to true, but still remove the '&' argument
     else if ((strcmp(newCommand->args[(i - 1)], "&") == 0) && (fgFlag == true))
     {
         newCommand->args[(i - 1)] = NULL;
     }

    return newCommand;

}


/*
* Algorithm adapted from
* CITATION: https ://www.tutorialspoint.com/c-program-to-replace-a-word-in-a-text-by-another-given-word
* This function handles variable expansion when the user types $$
*/
char* varExpansion(char* inputString)
{
    char pidVar[] = "$$";

    //get the process id as an int
    int process_id = getpid();

    // The below 3 lines of code are from https://stackoverflow.com/questions/5242524/converting-int-to-string-in-c
    int length = snprintf(NULL, 0, "%d", process_id);
    fflush(stdout);
    char* str = malloc(length + 1);

    //cast the Pid into the str string
    snprintf(str, length + 1, "%d", process_id);
    fflush(stdout);

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

    return(expCommand);
}

/*This function first checks for any background processes that have terminated
* and then it prompts the user for input
*/
char* getInput()
{
    //clean up background process before returning control
    backgroundCheck();


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

/* This function was used for testing parseInput
* it prints out the commandStruct
*/
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
        fflush(stdout);
        i++;
    }

}

/*This function allows the user to change directory, it uses
* 'chdir'
*/
void changeDir(struct commandStruct* aCommand)
{
    //checks for an argument
    if (aCommand->args[1] == NULL)
    {
        chdir(getenv("HOME"));
        char currDir[MAX_BUF];
        getcwd(currDir, MAX_BUF);
        //printf("Current working directory: %s\n", currDir);
        //fflush(stdout);
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
            //printf("Current working directory: %s\n", currDir);
            //fflush(stdout);
        }
    }
}

/* This function takes a pid number and adds it to a list of current running background
* pids
*/
void addBackgroundPid(pid_t pidNum)
{
    for (int i = 0; i < 200; i++)
    {
        if (backgroundPids[i] == NULL)
        {
            backgroundPids[i] = pidNum;
            break;
        }
    }
}


/* This function checks if any background processes have terminated
*/
void backgroundCheck()
{
    int childExitMethod;

    //check if any child processes have terminated
    pid_t childPID = waitpid(-1, &childExitMethod, WNOHANG);


    //if no error and child has terminated we print what caused the 
    //termination and the pid
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

    //Remove the pid of the terminated process from list of running
    //background pirds
    for (int i = 0; i < 200; i++)
    {
        if (backgroundPids[i] == childPID)
        {
            backgroundPids[i] = NULL;
        }
    }
}


/* This function kills all child processes and then kills
* the parent process
*/
void exitShell()
{
    int i;

    //iterate through the child processes and terminate them if they have not completed
    for (i = 0; i < 200; i++)
    {
        int childExitMethod;

        //if child has not completed then we terminate
        if (waitpid(backgroundPids[i], &childExitMethod, WNOHANG) == 0)
        {
            //call SIGTERM
            kill(backgroundPids[i], 15);
        }        
    }
    //kill the parent process
    kill(getpid(), 15);
}


/* This function uses execvp to execute the non built in commands
*/
void execCommand(struct commandStruct* aCommand, char *statusString, int *statusCode, struct sigaction SIGINT_action, struct sigaction SIGTSTP_action)
{
    pid_t spawnPid;
    int childExitStatus;

    //variables used for I/O redirection
    int inpFD;
    int outFD;
    int result;

    //fork into a new process
    spawnPid = fork();

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
            //handle control Z
            SIGTSTP_action.sa_handler = SIG_IGN;
            sigaction(SIGTSTP, &SIGTSTP_action, NULL);


            //handle control C
            if (aCommand->background == false)
            {
                SIGINT_action.sa_handler = SIG_DFL;
                sigaction(SIGINT, &SIGINT_action, NULL);
            }

            //check for input redirection or background flag
            if ((aCommand->background == true) || (aCommand->inpRedir != NULL))
            {
                //send to dev/null if no redirection
                if (aCommand->inpRedir == NULL)
                {
                    inpFD = open("/dev/null", O_RDONLY, 0666);
                }
                else
                {
                    inpFD = open(aCommand->inpRedir, O_RDONLY, 0666);
                }

                // if error set exit status to 1 and print message
                if (inpFD == -1) 
                {
                    printf("cannot open %s for input\n", aCommand->inpRedir);
                    fflush(stdout);
                    *statusCode = 1;
                }

                // Redirects stdin to the input file
                result = dup2(inpFD, 0);

                // Checks for an error 
                if (result == -1) 
                {
                    exit(1);
                }

                //close the input file
                fcntl(inpFD, F_SETFD, FD_CLOEXEC);
            }

            //check for output redirection or background flag
            if ((aCommand->background == true) || (aCommand->outpRedir != NULL))
            {
                //send to dev/null if no redirection
                if (aCommand->outpRedir == NULL)
                {
                    outFD = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0666);
                }
                else
                {
                    outFD = open(aCommand->outpRedir, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                }

                //if error set exit status to 1 and print message
                if (outFD == -1)
                {
                    printf("cannot open %s for output\n", aCommand->outpRedir);
                    fflush(stdout);
                    *statusCode = 1;
                }

                // Redirects stdin to the output file
                result = dup2(outFD, 1);

                // Checks for an error 
                if (result == -1)
                {
                    exit(1);
                }

                //close the output file
                fcntl(outFD, F_SETFD, FD_CLOEXEC);
            }

            //execute the command
            if (execvp(aCommand->command, aCommand->args) < 0)
            {
                //if there is an error we change the exit status
                perror(aCommand->command);
                fflush(stdout);
                *statusCode = 1;
                exit(1);
            }


            break;
        }

        //This behavior is only run in the parent process
        default:
        {          
            //we run the command in the background
            if (aCommand->background == true)
            {
                printf("background pid is %d\n", spawnPid);
                fflush(stdout);

                //add the pid to list of background pids
                addBackgroundPid(spawnPid);

                //give control back to parent with child running in the background
                spawnPid = waitpid(spawnPid, &childExitStatus, WNOHANG);
            }

            else

                //run in the foreground
            {
                spawnPid = waitpid(spawnPid, &childExitStatus, 0);

                //tell us why the child exited/terminated
                if (WIFEXITED(childExitStatus) != 0)
                {
                    //update the exit status & code
                    strcpy(statusString, "exit value");
                    *statusCode = WEXITSTATUS(childExitStatus);

                }
                else if (WIFSIGNALED(childExitStatus) != 0)
                {
                    int termSignal = WTERMSIG(childExitStatus);
                    printf("terminated by signal %d\n", termSignal);
                    fflush(stdout);

                    //update the exist status and terminating signal
                    strcpy(statusString, "terminated by signal");
                    *statusCode = WTERMSIG(childExitStatus);
                }
            }
            break;
        }
    }
}

/* This function handles our sigtstp signal
*/
void handle_SIGTSTP(int signo)
{

    // If it's false we then set it to true and print a message
    if (fgFlag == false)
    {
        char* message = "\nEntering foreground-only mode (& is now ignored)\n";
        write(STDOUT_FILENO, message, 49);
        fflush(stdout);
        fgFlag = true;
    }

    // If it's true we then set it to false and print a message
    else
    {
        char* message = "\nExiting foreground-only mode\n";
        write(STDOUT_FILENO, message, 29);
        fflush(stdout);
        fgFlag = false;
    }
}

/* This function runs the loop for our main screen
*/
void mainScreen(char* statusString, int *statusCode)
{
    //Set up for ignoring control C
    //initialize the SIGINT_action struct to be empty
    struct sigaction SIGINT_action = {{0}};

    // Register SIG_IGN as the signal handler
    SIGINT_action.sa_handler = SIG_IGN;

    // Block all catchable signals while SIG_IGN is running
    sigfillset(&SIGINT_action.sa_mask);

    // No flags set
    SIGINT_action.sa_flags = 0;

    // Install our signal handler
    sigaction(SIGINT, &SIGINT_action, NULL);

    //set up for handling control Z
    //initialize the SIGTSTP_action struct to be empty
    struct sigaction SIGTSTP_action = {{0}};

    // Register handle_SIGTSTP as the signal handler
    SIGTSTP_action.sa_handler = handle_SIGTSTP;

    // Block all catchable signals while handle_SIGTSTP is running
    sigfillset(&SIGTSTP_action.sa_mask);

    // no flags set
    SIGTSTP_action.sa_flags = 0;

    // Install our signal handler
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);


    bool loopFlag = true;

    //as long as 'exit' not entered continue looping
    while (loopFlag == true)
    {
        char* input;
        struct commandStruct* commandLine;
        input = getInput();
        commandLine = parseInput(input, fgFlag);


        if (commandLine == NULL)
        {
            continue;
        }
        //if user enters nothing or '#' then we restart
        else if (commandLine->args[0][0] == '#' || (strcmp(commandLine->command, "\n") == 0))
        {
            continue;
        }

        //If user enters "cd"
        else if (strcmp(commandLine->command, "cd") == 0)
        {
            changeDir(commandLine);
            continue;
        }

        //If the user enters "status"
        else if (strcmp(commandLine->command, "status") == 0)
        {
            printf("%s %d\n", statusString, *statusCode);
            fflush(stdout);
            continue;
        }

        else if (strcmp(commandLine->command, "exit") == 0)
        {
            exitShell();
        }

        else
            //use execvp to run commands as child
        {
            execCommand(commandLine, statusString, statusCode, SIGINT_action, SIGTSTP_action);
            continue;
        }
    }
}


/* This is our main function, it sets up our
* sigaction structs for signal handling and then 
* calls the mainScreen() function
*/
int main(void)
{
    //initalize the status variables for use in Status command
    char statusString[30];
    strcpy(statusString, "exit value");
    int statusCode = 0;

    printf("$ smallsh\n");
    fflush(stdout);

    mainScreen(statusString, &statusCode);

    return 0;

}