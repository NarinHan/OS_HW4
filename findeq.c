// Design and construct a multithreaded program that finds groups of equal files
// - Count how much memory space is wasted by redundant files
// - Identify chances of saving up storage spaces by removing redundant files

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#include <signal.h>

typedef struct parsing {
    int tnum ;
    int size ;
    char output_path[30] ;
    char dir[30] ;
} data ;

// User Interface > Takes inputs as command-line arguments : $findeq [OPTION] DIR
// - Receives a path to a target directory DIR : all files in DIR and its subdirectories are search scope 
// - Options
//     - -t=NUM : creates upto NUM threads addition to the main thread; no more than 64
//     - -m=NUM : ignores all files whose size is less than NUM bytes from the search; default 1024
//     - -o FILE : produces to the output to FILE; by default the output must be printed to the stdout
// - For invalid inputs : show proper error messages to the use and terminates
data option_parsing(int argc, char **argv) {
    data d ;
    int opt; // option
    char temp_input[30];

    while ( (opt = getopt(argc, argv, ":t:m:o:")) != -1 ) 
    {
        switch (opt)
        {
            case 't' :
                memcpy(temp_input, optarg, strlen(optarg)) ;
                // sscanf(temp_input, "=%d", &tnum) ;    
                // tnum = atoi(temp_input + 1) ;
                char *endptr ;
                d.tnum = strtol(temp_input, &endptr, 10) ; // attempt to convert the entire string to an integer
                                                         // set the first invalid character encountered during the conversion in &endptr
                while (*endptr && !isdigit(*endptr)) { // skip any non-digit characters
                    endptr++ ;
                }
                if (*endptr) { // extract the integer value from the remaining portion of the string
                    d.tnum = strtol(endptr, &endptr, 10) ; // start from the updated endptr
                } 
                break ;
            case 'm' :
                memcpy(temp_input, optarg, strlen(optarg)) ;
                // sscanf(temp_input, "=%d", &tnum) ;    
                // tnum = atoi(temp_input + 1) ;
                d.size = strtol(temp_input, &endptr, 10) ; // attempt to convert the entire string to an integer
                                                         // set the first invalid character encountered during the conversion in &endptr
                while (*endptr && !isdigit(*endptr)) { // skip any non-digit characters
                    endptr++ ;
                }
                if (*endptr) { // extract the integer value from the remaining portion of the string
                    d.size = strtol(endptr, &endptr, 10) ; // start from the updated endptr
                }
                break ;
            case 'o' : // TODO : 생략 가능 > default stdout
                memcpy(d.output_path, optarg, strlen(optarg)) ;
                d.output_path[strlen(optarg)] = '\0' ;
                break ;
            case ':' :
                if (optopt == 't' || optopt == 'm') {
                    printf("err : -%c option requires int\n", optopt) ;
                }
                else {
                    printf("err : -%c option requires string\n", optopt) ;
                }
                exit(1) ;
            case '?' :
                printf("err : %c is undetermined option\n", optopt) ;
                exit(1) ;
        }
    }
    memcpy(d.dir, argv[optind], strlen(argv[optind])) ;

    return d ;
}

// Signal : Print the search progress to standard output every 5 sec
void sigalrm_handler(int sig) 
{
    if (sig == SIGALRM)
    {
        printf("\nsigalrm_handler function is handling 5 sec rule\n") ;
        // TODO : don't forget to add alarm(5) and alarm(0) somewhere in the main ...
        // display
    }
}

// Signal : Produces output promptly when it receives the SIGINT signal or the file search terminates
void sigint_handler(int sig) 
{
    if (sig == SIGINT) {
        printf("\nsigint_handler function is handling CTRL+C\n") ;
        // produce output
        // display
    }
    exit(0) ;
}

int 
main(int argc, char* argv[])
{
    signal(SIGALRM, sigalrm_handler) ;
    signal(SIGINT, sigint_handler) ;

    // alarm(5) ;  

    printf("...going to sleep...\n") ;
    sleep(10) ;
    printf("...now wake up...\n") ;
    
    data d = option_parsing(argc, *&argv) ;

    printf("t : %d\n", d.tnum) ;
    printf("m : %d\n", d.size) ;
    printf("o : %s\n", d.output_path) ;
    printf("dir : %s\n", d.dir) ;

    // Signal & Termination & Display
    // - Prints the list of the filepath lists such that each filepath list enumerates all relative paths of the files having the exact same content, discovered so fread
    //     - Each list guarded by square brackets []
    //     - Each element of a list must be separated by comma and newline
    // - Print the search progress to standard output every 5 sec
    //     - Show number of files known to have at least one other identical file
    //     - Other information about the program execution
    //         - TODO : should be specified > t/m/o option, target directory ?

    



    return 0;
}