#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#include <signal.h>
#include <dirent.h> 
#include <sys/stat.h>

typedef struct _data {
    int tnum ;
    int size ;
    char output_path[30];
    char dir[30] ;
} data ;

data d ;

char *file_name[1000] ;
int file_count = 0 ;

char *identical[1000] ;
int identical_count = 0 ;
int iden_file_count = 0 ;

void option_parsing(int argc, char **argv) {
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
                memcpy(temp_input, optarg, strlen(optarg)) ;
                int idx = 0 ;
                for (int i = 0 ; i < strlen(temp_input) ; i++) {
                    if (isalpha(temp_input[i])) {
                        d.output_path[idx] = temp_input[i] ;
                        idx++ ;
                    }
                }
                d.output_path[strlen(d.output_path)] = '\0' ;
                break ;
            case ':' :
                // TODO : invalid input 일 때 handling, this one is not currently not working
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
}

// Display 
void display() 
{
    printf("(t) Number of thread : %d\n", d.tnum) ;
    printf("(m) Ignoring file size : %d\n", d.size) ;
    printf("(o) Output path : %s\n", d.output_path) ;
    printf("Target directory : %s\n", d.dir) ;
    printf("// TODO : Print the list of the filepath in []\n") ;
    printf("// TODO : Print the number of files known to have at least one other identical file\n") ;
}

// Signal : Print the search progress to standard output every 5 sec
void sigalrm_handler(int sig) 
{
    if (sig == SIGALRM)
    {
        printf("\nsigalrm_handler function is handling 5 sec rule\n") ;
        // TODO : don't forget to add alarm(5) and alarm(0) somewhere in the main ...
        display() ;
    }
}

// Signal : Produces output promptly when it receives the SIGINT signal or the file search terminates
void sigint_handler(int sig) 
{
    if (sig == SIGINT) {
        printf("\nsigint_handler function is handling CTRL+C\n") ;
        // TODO : produce output
        display() ;
    }
    exit(0) ;
}

// Checks all regular files in the target directory and its subdirectory recursively
void readDirectory(const char* dir_name)  
{
    DIR *dir = opendir(dir_name) ;
    if (dir == NULL) {
        printf("Failed to open a directory!\n") ;
        return ;
    }

    // read directory entries
    struct dirent *entry ;
    struct stat file_stat ;
    while ((entry = readdir(dir)) != NULL) 
    {
        char file_path[256] ;
        snprintf(file_path, sizeof(file_path), "%s/%s", dir_name, entry->d_name) ; // write formatted data to a string buffer

        if (lstat(file_path, &file_stat) == -1) {
            printf("Failed to get a file status!\n") ;
            continue ;
        }

        if (S_ISREG(file_stat.st_mode)) { // regular file
            if (strncmp(entry->d_name, ".", 1) == 0) { // to avoid .DS_Store, .gitignore, etc
                continue ;
            }
            // allocate memory to store the directory + file name
            file_name[file_count] = (char *) malloc((strlen(file_path) + 1) * sizeof(char)) ;
            strcpy(file_name[file_count], file_path) ;
            file_count++ ;
        } 
        else if (S_ISDIR(file_stat.st_mode)) { // subdirectory 
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) { // to avoid infinite recursion
                char subdir[256] ;
                snprintf(subdir, sizeof(subdir), "%s/%s", dir_name, entry->d_name) ;
                readDirectory(subdir) ;
            }
        }
        // else if (S_ISLINK(file_stat.st_mode)) {
        //     continue ;
        // }
    }

    closedir(dir) ;
}

void compare(int index) 
{
    // initialize
    if (identical[0] != NULL) {
        for (int i = 0 ; i < identical_count ; i++) {
            free(identical[i]) ;
        }
        identical_count = 0 ;
    }

    identical[identical_count] = (char *) malloc((strlen(file_name[index]) + 1) * sizeof(char)) ;
    strcpy(identical[identical_count], file_name[index]) ;
    identical_count++ ;

    // file open
    FILE *fp1, *fp2 ;
    long fp1s, fp2s ;
    long fp1b, fp2b ;

    fp1 = fopen(file_name[index], "rb") ; // open pivot file
    if (fp1 == NULL) {
        printf("Failed to open a file %s!\n", file_name[index]) ;
        return ;
    }

    // get the file size
    fseek(fp1, 0L, SEEK_END) ; // move the file pointer to the end of the file
    fp1s = ftell(fp1) ; // get the current position of the file pointer == file size
    rewind(fp1) ; // rewind to get the sequence of bytes

    for (int i = 0 ; i < file_count ; i++) {
        if (i == index)
            continue ;
        
        rewind(fp1) ;

        fp2 = fopen(file_name[i], "rb") ; // open in binary mode
        if (fp2 == NULL) {
            printf("Failed to open a file %s!\n", file_name[i]) ;
            return ;
        }
        
        // get the file size
        fseek(fp2, 0L, SEEK_END) ; // move the file pointer to the end of the file
        fp2s = ftell(fp2) ; // get the current position of the file pointer == file size
        rewind(fp2) ; // rewind to get the sequence of bytes
        
        int flag = 0 ;
        
        // compare the sizes
        if (fp1s == fp2s) {
            // get the sequence of bytes and compare
            while (1) {
                fp1b = fgetc(fp1) ;
                fp2b = fgetc(fp2) ;

                if (fp1b != fp2b) {
                    flag = 1 ;
                    break ;
                }
                
                if (fp1b == EOF || fp2b == EOF) 
                    break ;
            }
        }
        else {
            flag = 1 ;
        }

        if (flag == 0) { // identical
            identical[identical_count] = (char *) malloc((strlen(file_name[i]) + 1) * sizeof(char)) ;
            strcpy(identical[identical_count], file_name[i]) ;
            identical_count++ ;
        }
    }

    if (identical_count != 1) {
        iden_file_count++ ;
    }
    
    fclose(fp1) ;
    fclose(fp2) ;
}

int 
main(int argc, char* argv[])
{
    signal(SIGALRM, sigalrm_handler) ;
    signal(SIGINT, sigint_handler) ;

    // alarm(5) ;  

    // printf("...going to sleep...\n") ;
    // sleep(10) ;
    // printf("...now wake up...\n") ;
    
    option_parsing(argc, *&argv) ;
    display() ;

    readDirectory(d.dir) ;

    for (int i = 0 ; i < file_count ; i++) {
        printf("%s\n", file_name[i]) ;
    }

    for (int i = 0 ; i < file_count ; i++) {
        compare(i) ;

        printf("\nNumber of identical files : %d\n", iden_file_count) ;
        printf("[\n") ;
        for (int j = 0 ; j < identical_count ; j++) {
            printf("    %s\n", identical[j]) ;
        }
        printf("]\n") ;
    }

    // deallocation
    for (int i = 0 ; i < file_count ; i++) {
        free(file_name[i]) ;
    }
    for (int i = 0 ; i < identical_count ; i++) {
        free(identical[i]) ;
    }
    return 0;
}
