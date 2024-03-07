#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_INPUT_SIZE 300 // very arbitrary
#define MAX_PATH_SIZE 1000 // very arbitrary

void secret_stuff();

int main(int argc, char *argv[]) {

    // make sure valid syntax was supplied with program for prompt setup
    if (argc != 3 && argc != 1) {
        printf("Invalid syntax. Please supply no arguments or use the option '-p' followed by your desired name for the prompt.\n");
        // exit(1);
        return 1; // using return and not exit so that destructors are called
    }
    // getting here means the user either specified a special name for the prompt or ran with default

    // used for everything
    char *prompt = "308sh> "; // set prompt to use default name
    char input[MAX_INPUT_SIZE]; // arbitrary max input size, not sure what to set to!
    int i; // used to iterate for loops
    
    // used for built-ins
    char path[MAX_PATH_SIZE]; // used in pwd
    char dir[MAX_INPUT_SIZE]; // used in cd <path>

    // used for program commands
    int runAsBg = 0; // run child process as background
    int numArgs; 
    int child; // for executing program command
    int j; // additional counter for loops (iterating columns in 2D array for args)
    int status;
    int bg_exit = -1;
    char argToPrint[1000];

    signal(SIGINT, secret_stuff); // nothing to see here...


    if (argc == 3)  { // user suppled a special name for the prompt

        // make sure user actually used the option -p
        if (strlen(argv[1]) != 2 || strcmp(argv[1], "-p") != 0) { // strcmp = string compare, returns 0 if gucci
            printf("Invalid option supplied. Please instead use the option '-p' to set the name of the shell prompt.\n");
            // exit(1);
            return 1;
        }
        prompt = argv[2];
    }

    // now onto actually being a shell!
    while (1) {
        
        // see if any background processes that were running are finished
        bg_exit = waitpid(-1, &status, WNOHANG);
        while (bg_exit > 0) { // argToPrint should never be null if the code enters this while loop, so should be ok
            if (1) {
                if (WIFEXITED(status)) {
                    printf("[%d] %s Exit %d\n", bg_exit, argToPrint, WEXITSTATUS(status));
                } else if (WIFSIGNALED(status)) {
                    printf("[%d] %s Killed, signal: %d\n", bg_exit, argToPrint, WTERMSIG(status));
                }
            } else {
                printf("\n[%d] %s Exit %d\n", bg_exit, argToPrint, WEXITSTATUS(status));
            }

            fflush(stdout);

            bg_exit = waitpid(-1, &status, WNOHANG);
        }

        // print the prompt
        printf(prompt);

        // fgets is blocking, wait for user input, store in "input"
        fgets(input, MAX_INPUT_SIZE, stdin);

        // need to get rid of new-line character at end of input, second condition probably unnecessary
        if (strlen(input) > 0 && input[strlen(input) - 1 != '\0']) {
            input[strlen(input) - 1] = '\0';
        }

        // if they didn't enter anything at all just re-prompt
        // don't think c has a trim() function like java, so im just
        // gonna keep things simple and test these 2
        if (strcmp(input, " ") == 0 || strcmp(input, "") == 0) {
            continue;
        }

        /*      now handle da built-in commands...       */

        if (strcmp(input, "exit") == 0) {                                       /* exit */
            return 0;
        } else if (strcmp(input, "pid") == 0) {                                 /* pid */
            printf("Shell PID: %d\n", getpid());
        } else if (strcmp(input, "ppid") == 0) {                                /* ppid */
            printf("Shell's PPID (parent PID): %d\n", getppid());
        } else if (strcmp(input, "pwd") == 0) {                                 /* pwd */
            if (getcwd(path, MAX_PATH_SIZE) == NULL) { // null terminates btw
                printf("Failed finding the pwd. Path too long.\n");
            } else {
                printf("%s\n", path);
            }
        } else if (strncmp(input, "cd", 2) == 0) {                              /* cd (i love writing each built-in to be ordered in terms of increasing size/code) */
            // see if just said cd or a particular directory
            if (strlen(input) == 2) {
                // documentation said to navigate to home directory
                chdir(getenv("HOME"));
                // setenv used for creating new env labels, so not really needed in these contexts
            } else {

                // get the path for the desired directory
                for (i = 0; i < strlen(input) - 3; i++) { // strlen(input) - 3 to account for "cd " in the input
                    dir[i] = input[i + 3]; // input starting at i + 3 to account for "cd " in the input

                    // make sure to null-terminate it (so doing cd multiple times won't give an incorrect path (e.g. cd blah/bleh/blar; cd/blah/bleh --> but second one keeps /blar at the end))
                    dir[i+1] = '\0';
                }

                if (chdir(dir) != 0) {
                    printf("Failed navigating to specified directory.\n");
                } // if this message doesn't happen, then it already changed successfully :)             
            }
        } else {

            /*      now handle program commands... (ps, ls, emacs, etc.)    */

            // reset some stuff
            numArgs = 1;    // program to be executed itself is counted as an arg

            // how many args are we working with here
            for (i = 0; i < strlen(input); i++) {
                // can roughly count number of args by number of spaces, overcounting just means some wasted space (should still work fine though)
                if (input[i] == ' ')
                    numArgs++;
            }

            // see if it's supposed to run as background process
            if (input[strlen(input) - 1] == '&') {
                runAsBg = 1;
                numArgs--; // don't want to count '&' as an arg
            }

            // create a 2D array of characters (essentially a string) and an array of char pointers
            // each row in the 2D array will be an individual argument (over multiple columns)
            // the array of char pointers is for easier referencing

            // note: taking this approach b/c execvp is weird, could surely make easier with strtok (which would theoretically break string into args easily) but im not 
            //       very familiar with the command and previous attempts at using it to simplify things failed. TA and peers explained this approach they've seen and it 
            //       makes sense to me so im running with it
            char args[numArgs][MAX_INPUT_SIZE];
            char* argsPtr[numArgs + 1];
            numArgs = 0; // reset the number of args for indexing in correct spots of 2D array
            for (i = 0; i < strlen(input) + 1; i++) {
                if (input[i] == ' ' || input[i] == '\0') { // increment args when done reading each arg (and tack on null terminator to 2D array, as well as setting ptr)
                    args[numArgs][j] = '\0';
                    argsPtr[numArgs] = args[numArgs]; // argsPtr now points to this row (argument string) in the 2D array
                    numArgs++;
                    j = 0;
                } else if ((input[i] == '&') && (i = strlen(input) - 1)) { // get outta da loop when at the end
                    break; 
                } else {
                    args[numArgs][j] = input[i];
                    j++;
                }
            }
            // god i hope i did that right otherwise debugging is gonna suck

            // tack on a null pointer at the end for easier processing & debugging
            argsPtr[numArgs] = NULL; // probably don't need to cast

    
            if (runAsBg == 1) {
                runAsBg = 0;
                child = fork();

                if (child == 0) { // child branch

                    // argToPrint = argsPtr[0];

                    // execute the command
                    execvp(argsPtr[0], argsPtr);

                    // if execution gets here then the command wasn't recognized
                    perror("**Error");
                    exit(255);

                } else { // parent branch

                    // print the child PID and the command executed
                    printf("[%d] %s\n", child, argsPtr[0]);
                    // printf(prompt);

                    if (bg_exit != 0) { // doesn't actually update it as intended, so it's a bug. (or rather make sure argtoprint doesn't actually update)
                        strcpy(argToPrint, argsPtr[0]); // would've fixed via using strcpy (rn it's a shallow copy which is the bug), but was causing seg faults for some reason
                    }
                    

                    // wait for child process to complete
                    status = -1;
                    bg_exit = waitpid(-1, &status, WNOHANG); // maybe 0 instead of WNOHANG? idk the project documentation a little confusing 
                    


                }
            } else { // not running as a background process

                child = fork();

                if (child == 0) { // child branch

                    // print the child PID and the command executed
                    printf("[%d] %s\n", getpid(), argsPtr[0]);

                    // argToPrint = argsPtr[0];

                    // actually execute the command
                    execvp(argsPtr[0], argsPtr);
                    
                    // if execution gets here then the command wasn't recognized
                    perror("**Error");
                    exit(255);

                } else { // parent branch

                    usleep(1000); // arbitrary value (need to test if it's actually ok)

                    if (bg_exit != 0) { // doesn't actually update it as intended, so it's a bug. (or rather make sure argtoprint doesn't actually update)
                        strcpy(argToPrint, argsPtr[0]); // would've fixed via using strcpy (rn it's a shallow copy which is the bug), but was causing seg faults for some reason
                    }
                    
                    
                    // wait for child process to complete
                    status = -1;
                    waitpid(child, &status, 0);

                    // print that stuff out 
                    if (WIFEXITED(status)) {
                        printf("[%d] %s Exit %d\n", child, argsPtr[0], WEXITSTATUS(status));
                    } else if (WIFSIGNALED(status)) {
                        printf("[%d] %s Killed, signal: %d\n", child, argsPtr[0], WTERMSIG(status));
                    }
                }
            }

            // see if any background processes that were running are finished
            while (bg_exit > 0) { // argToPrint should never be null if the code enters this while loop, so should be ok
                if (1) {
                    if (WIFEXITED(status)) {
                        printf("[%d] %s Exit %d\n", child, argToPrint, WEXITSTATUS(status));
                    } else if (WIFSIGNALED(status)) {
                        printf("[%d] %s Killed, signal: %d\n", child, argToPrint, WTERMSIG(status));
                    }
                } else {
                    printf("\n[%d] %s Exit %d\n", child, argToPrint, WEXITSTATUS(status));
                }

            fflush(stdout);

            bg_exit = waitpid(-1, &status, WNOHANG);
            }


        }

    }

    return 0; // dunno when it'll get here but better to be safe :)
}











void secret_stuff() {
    printf("\nEveryone likes a little secret :D\n"); 
    // imagine i created a fork bomb here, it would be a little funny right?
    exit(69);
}