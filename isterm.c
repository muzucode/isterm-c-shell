#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "TestEnvironment.h"
#include <fcntl.h>

// This shell administers usage of test environments

#define SHELL_PREFIX "isT>"
#define MAX_TOKENS_AMOUNT 16
TestEnvironment* activeTestEnvironment;
TestEnvironmentList* testEnvironmentList;



void resetActiveTestingEnvironment() {
    // TODO: If activeTestEnvironment is not null, free it
    // and all of its fields
    activeTestEnvironment = (TestEnvironment*)malloc(sizeof(TestEnvironment));
    if(activeTestEnvironment == NULL) {
        perror("Errored mallocing activeTestEnvironment");
        exit(1);
    }
}

void startTe() {
    printf("Starting test environment...\n");   
    int nCommands = 0;
    char** commands = (char**)malloc(MAX_START_STOP_COMMANDS * sizeof(char*)); 
    char** args = (char**)malloc(MAX_START_STOP_COMMANDS_ARGUMENTS * sizeof(char*));

    // Get all the commands separated by commas
    commands = parseStartCommands(activeTestEnvironment, &nCommands);


    // Parse arguments from command
    for(int i = 0; i < nCommands; i++) {
        int nArgs = 0;
        char* arg = strtok(commands[i], " ");
        args[0] = strdup(arg);
        // printf("arg %d: %s\n", nArgs, args[i]);

        while(arg != NULL) {
            arg = strtok(NULL, " ");
            args[nArgs++] = strdup(arg);
            printf("arg %d: %s\n", nArgs, args[nArgs]);
        }
    }
    exit(1);

    // Print each command
    for(int j = 0; j < nCommands; j++) {
        printf("Loaded command %d:%s\n",j, commands[j]);
    }


    pid_t p = fork();
    if (p < 0) {
        perror("Error forking child process");
        exit(1);
    } else if (p == 0) {
        // In child process: redirect stdout to /dev/null
        int fd = open("/dev/null", O_WRONLY);
        if (fd != -1) {
            dup2(fd, STDOUT_FILENO);
            close(fd);
        } else {
            perror("Error opening /dev/null");
            exit(1);
        }
        execvp(commands[0], commands);
        exit(0);  // Ensure the child process exits
    }
}

char** parseTokens(char* input, int* tokensCount) {

    // Duplicate the input arg so we retain its value despite
    // strtok
    char* inputToParse = strdup(input);
    if(inputToParse == NULL) {
        perror("Error strdup inputToParse");
        return NULL;
    }

    // Initialize tokensCount
    *tokensCount = 0;

    // Allocate memory for tokens array
    char** tokens = (char**)malloc(MAX_TOKENS_AMOUNT * sizeof(char*));
    if (tokens == NULL) {
        perror("Error mallocing tokens in parseTokens");
        return NULL;
    }

    // Begin strtok
    char* token = strtok(inputToParse, " ");

    // Loop through all tokens
    while (token != NULL && *tokensCount < MAX_TOKENS_AMOUNT) {
        tokens[*tokensCount] = strdup(token);
        if (tokens[*tokensCount] == NULL) {
            perror("Error strdup a token");
            for (int i = 0; i < *tokensCount; i++) {
                free(tokens[i]);
            }
            free(tokens);
            return NULL;
        }
        (*tokensCount)++;
        token = strtok(NULL, " ");
    }

    free(inputToParse);

    return tokens;
}

int setActiveTestingEnvironment(char* label) {
    TestEnvironment* environmentFound = findTestEnvironmentByLabel(testEnvironmentList, label);
    if(environmentFound == NULL) {
        return 1;
    } else {
        activeTestEnvironment = environmentFound;
        return 0;
    }

}

void printShellStamp() {
    if(activeTestEnvironment->label == NULL) {
        printf("(-) %s ", SHELL_PREFIX);
    } else {
        printf("(%s) %s ", activeTestEnvironment->label, SHELL_PREFIX);
    }
    // printf("\nPrinted the prefix\n");
}

void listenForInput() {
    char buf[128];
    char* input = NULL;
    char** tokens;
    int tokensCount;


    while (1) {

        printShellStamp();

        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            perror("Error reading input");
            break;
        }

        buf[strcspn(buf, "\n")] = '\0';  // Remove newline character

        input = strdup(buf);
        if (input == NULL) {
            perror("Error strdup input");
            break;
        }

        tokens = parseTokens(input, &tokensCount);
        if (tokens == NULL) {
            printf("Tokens is NULL for some reason... restarting loop\n");
            free(input);
            continue;
        }

        if (tokensCount > 0 && strcmp(tokens[0], "exit") == 0) {
            printf("Seeya!\n");
            for (int i = 0; i < tokensCount; i++) {
                free(tokens[i]);
            }
            free(tokens);
            free(input);
            break;
        } else if (tokensCount > 0 && strcmp(tokens[0], "list") == 0) {
            // TODO: Implement printing list of testenvlist
            printf("TODO: implement\n");
        } else if (tokensCount > 0 && strcmp(tokens[0], "use") == 0) {

            // If a testenv argument is provided...
            if(tokens[1] != NULL) {
                int result = setActiveTestingEnvironment(tokens[1]);
                if(result != 0) {
                    printf("Invalid environment \"%s\". Run \"list\" to see a list of available environments.\n", tokens[1]);
                    continue;
                }
            } else {
                printf("No environment specified.  If you'd like to reset your shell to the default 'environment-less' state (-), run 'reset'.\n");
            }
        } else if (tokensCount > 0 && strcmp(tokens[0], "reset") == 0) {
                resetActiveTestingEnvironment();
        } else if (tokensCount > 0 && strcmp(tokens[0], "start") == 0) {
            // TODO: Add handling to prevent "start" if no activeEnvironment is set
            
            startTe();
        } else if (tokensCount > 0 && strcmp(tokens[0], "stop") == 0) {
            printf("Stopping test environment...\n");
            testEnvironmentStop();
        } else if (tokensCount > 0 && strcmp(tokens[0], "te:add")  == 0 ) {
            printf("Add a test environment, below:\n");
            // promptTestEnvironmentCreation();
        } else {
            printf("Unrecognized command \"%s\"\n", input);
        }


        // Free the tokens and all their data
        for (int i = 0; i < tokensCount; i++) {
            free(tokens[i]);
        }

        free(tokens);
        free(input);
        
    } // end while()


    free(activeTestEnvironment->label);
}


int main() {
    activeTestEnvironment = (TestEnvironment*)malloc(sizeof(TestEnvironment));
    testEnvironmentList = (TestEnvironmentList*)malloc(sizeof(TestEnvironmentList));
    testEnvironmentList = readTestEnvironmentsFromConfig(); // malloc for each env in list
    listenForInput();
    return 0;
}
