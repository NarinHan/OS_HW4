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
#include <dirent.h> 
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>

typedef struct _data {
    int tnum ;
    int size ;
    char output_path[30];
    char dir[30] ;
} data ;

data d ;

char **identical[1000] ;
int iden_file_count[1000];
char *file_name[1000] ;
int file_count = 0 ;
int identical_count = -1 ;

int GetCurrentUsec() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec;
}

// User Interface > Takes inputs as command-line arguments : $findeq [OPTION] DIR
// - Receives a path to a target directory DIR : all files in DIR and its subdirectories are search scope 
// - Options
//     - -t=NUM : creates upto NUM threads addition to the main thread; no more than 64
//     - -m=NUM : ignores all files whose size is less than NUM bytes from the search; default 1024
//     - -o FILE : produces to the output to FILE; by default the output must be printed to the stdout
// - For invalid inputs : show proper error messages to the use and terminates
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
    // Prints the list of the filepath lists such that each filepath list enumerates 
    //     all relative paths of the files having the exact same content, discovered so far
    //     - Each list guarded by square brackets []
    //     - Each element of a list must be separated by comma and newline
    // Print the search progress to standard output every 5 sec
    //     - Show number of files known to have at least one other identical file
    //     - Other information about the program execution
    //         - TODO : should be specified > t/m/o option, target directory ?
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
void readDirectory(const char* dir_name, char **file_name, int *file_count)  
{
    // Do not follow hard and soft links
    // Do not consider non-regular files

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
        // to avoid infinite recursion
        if(strncmp(entry->d_name,".",1)==0 || strcmp(entry->d_name,"..")==0){
            continue;
        }

        char file_path[256] ;
        snprintf(file_path, sizeof(file_path), "%s/%s", dir_name, entry->d_name) ; // write formatted data to a string buffer

        if (lstat(file_path, &file_stat) == -1) {
            printf("Failed to get a file status!\n") ;
            continue ;
        }

        if (S_ISREG(file_stat.st_mode)) { // regular file
            // allocate memory to store the directory + file name
            size_t len = strlen(dir_name) + 1 + entry->d_reclen + 1 ;
            file_name[*file_count] = (char *) malloc(len * sizeof(char)) ;
            strcpy(file_name[*file_count], dir_name) ;
            strcat(file_name[*file_count], "/") ;
            strcat(file_name[*file_count], entry->d_name) ;
            (*file_count)++ ;
        } 
        else if (S_ISDIR(file_stat.st_mode)) { // subdirectory 
            char *subdir[256] ;
            snprintf(subdir, sizeof(subdir), "%s/%s", dir_name, entry->d_name) ;
            readDirectory(subdir, file_name, file_count) ;
        }
        else if (S_ISLNK(file_stat.st_mode)) continue;
    }

    closedir(dir) ;

/* 
    // need to read all the files exisiting on the target directory
    DIR *dir_pointer;
    struct dirent *dir;
    dir_pointer = opendir(d.dir);
    int count = 0;
    if (dir_pointer) {
        while ((dir = readdir(dir_pointer)) != NULL) {
            count++;
            printf("%s\n", dir->d_name);
        }
        closedir(dir_pointer);
    }

    //using these files, we need to store two values
    //1. each files' size in byte
    //2. each files' sequence of byte

    //size of each files can be identified by ftell()
    //sequence of byte can be known by fgetc
    //  -> since fgetc reads one character at a time,
    //     we just have to compare two files parellel.
*/
}

void compare(int index) 
{

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

    int local_file_count = 0;

    for (int i = 0 ; i < file_count ; i++) {
        if( i == index ) continue;
        rewind(fp1);
        fp2 = fopen(file_name[i], "rb") ; // open in binary mode
        if (fp2 == NULL) {
            printf("Failed to open a file %s!\n", file_name[i]) ;
            return ;
        }

        // get the file size
        fseek(fp2, 0L, SEEK_END) ; // move the file pointer to the end of the file
        fp2s = ftell(fp2) ; // get the current position of the file pointer == file size
        rewind(fp2) ; // rewind to get the sequence of bytes
        
        //printf("        TEST : currently opening %s\n", file_name[i]) ;

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

        if (flag == 0) {
            if(local_file_count == 0){
                printf("------------------------\n");
                printf("found identical files! \n(%s)\n###  and  ###\n(%s)\n",file_name[index],file_name[i]);
                printf("------------------------\n\n");
                identical_count++ ;
                identical[identical_count] = (char **) malloc(sizeof(char*));
                identical[identical_count][local_file_count] = (char *) malloc((strlen(file_name[index]) + 1) * sizeof(char)) ;
                strcpy(identical[identical_count][local_file_count], file_name[index]) ;
                // printf("1 file's name: %s\n",identical[*identical_count][local_file_count]);
                local_file_count++;
            }
            // printf("size of realloc: %d\n",local_file_count+1);
            // printf("hey %s\n",identical[0][0]);
            identical[identical_count] = (char**)realloc(identical[identical_count],sizeof(char*)*(local_file_count+1));
            identical[identical_count][local_file_count] = (char *) malloc((strlen(file_name[i]) + 1) * sizeof(char)) ;
            strcpy(identical[identical_count][local_file_count], file_name[i]) ;
            // printf("%d, %d\n",*identical_count, local_file_count);
            // printf("2 file's name: %s\n",identical[*identical_count][local_file_count]);
            // printf("%s\n%s\n",identical[0][0],identical[0][1]);
            // if(local_file_count==2) printf("%s\n",identical[0][2]);
            local_file_count++;
            
            //printf("first one: %d\n",iden_file_count[*identical_count]);
            iden_file_count[identical_count] = local_file_count;
        }
    }
    
    fclose(fp1) ;
    fclose(fp2) ;
}

int 
main(int argc, char* argv[])
{
    int start_time = GetCurrentUsec();
    signal(SIGALRM, sigalrm_handler) ;
    signal(SIGINT, sigint_handler) ;

    // alarm(5) ;  

    // printf("...going to sleep...\n") ;
    // sleep(10) ;
    // printf("...now wake up...\n") ;
    
    option_parsing(argc, *&argv) ;
    display() ;

    
    readDirectory(d.dir, file_name, &file_count) ;

    for (int i = 0 ; i < file_count ; i++) {
        printf("%s\n", file_name[i]) ;
    }

    
    for(int i=0; i<file_count; i++){
        compare(i) ;
    }

    //compare(file_name, file_count, identical, &identical_count, 0, iden_file_count);
    
    printf("\nNumber of identical files : %d\n", identical_count+1) ;
    printf("[\n") ;
    
    for (int i = 0 ; i < identical_count+1 ; i++) {
        printf("  [\n");
        for (int j = 0; j < iden_file_count[i]; j++){
            printf("    %s\n", identical[i][j]) ;
        }
        printf("  ]\n") ;
    }
    printf("]\n");
    
    int finish_time = GetCurrentUsec();

    printf("\n---------------------------------\n");
    printf("whole searching took %d sec\n", finish_time - start_time);
    printf("---------------------------------\n");

    // // deallocation
    // for (int i = 0 ; i < file_count ; i++) {
    //     free(file_name[i]) ;
    // }
    // for (int i = 0 ; i < identical_count ; i++) {
    //     free(identical[i]) ;
    // }
    return 0;
}