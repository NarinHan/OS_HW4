#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
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

char *file_name[1000] ;
int file_count;
char **identical[1000] ;
int identical_count = -1;
int iden_file_count[1000];

pthread_mutex_t iden_cnt_lock ;
pthread_mutex_t file_cnt_lock ;
pthread_mutex_t lock_n_threads ;
pthread_mutex_t subtasks_lock ;

int *threads ;
int max_threads = 64;
int max_size = 1024;
int n_threads = 0 ;

sem_t unused ;
sem_t inused ;


typedef struct _subtask {
    int index;
    int used;
} subtask ;

subtask * subtasks[64] ;

int head = 0 ;
int tail = 0 ;

void print_to_file(){
    printf("printing to file..\n");
    FILE* fp = fopen(d.output_path, "w");

    fprintf(fp,"\nNumber of identical files : %d\n", identical_count+1) ;

    fputs("[\n",fp) ;
    for (int i = 0 ; i < identical_count+1 ; i++) {
        fputs("  [\n",fp);
        for (int j = 0; j < iden_file_count[i]; j++){
            fprintf(fp,"\t%s\n", identical[i][j]) ;
        }
        fputs("  ]\n",fp) ;
    }
    fputs("]\n",fp);

    fclose(fp);
}

int GetCurrentUsec() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec;
}

void put_subtask (subtask * s) 
{
	sem_wait(&unused) ;
	pthread_mutex_lock(&subtasks_lock) ;
		subtasks[tail] = s ;
		tail = (tail + 1) % max_threads ;
	pthread_mutex_unlock(&subtasks_lock) ;
	sem_post(&inused) ;
}

subtask * get_subtask () 
{
	subtask * s ;
	sem_wait(&inused) ;
	pthread_mutex_lock(&subtasks_lock) ;
		s = subtasks[head] ;
		head = (head + 1) % max_threads ;
	pthread_mutex_unlock(&subtasks_lock) ;
	sem_post(&unused) ;

	return s ;
}

void option_parsing(int argc, char **argv) {
    int opt; 
    char temp_input[30];

    while ( (opt = getopt(argc, argv, ":t:m:o:")) != -1 ) 
    {
        switch (opt)
        {
            case 't' :
                memcpy(temp_input, optarg, strlen(optarg)) ; 
                char *endptr ;
                d.tnum = strtol(temp_input, &endptr, 10) ; 
                while (*endptr && !isdigit(*endptr)) { 
                    endptr++ ;
                }
                if (*endptr) {
                    d.tnum = strtol(endptr, &endptr, 10) ; 
                } 
                break ;
            case 'm' :
                memcpy(temp_input, optarg, strlen(optarg)) ;
                d.size = strtol(temp_input, &endptr, 10) ; 
                while (*endptr && !isdigit(*endptr)) { 
                    endptr++ ;
                }
                if (*endptr) { 
                    d.size = strtol(endptr, &endptr, 10) ;
                }
                break ;
            case 'o' : 
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

void sigalrm_handler(int sig) 
{
    if (sig == SIGALRM)
    {
        printf("\nsigalrm_handler function is handling 5 sec rule\n") ;
        printf("Current identical count: %d\n", identical_count);

        alarm(5);
    }
}

void sigint_handler(int sig) 
{
    if (sig == SIGINT) {
        pthread_mutex_lock(&iden_cnt_lock);
        printf("\nsigint_handler function is handling CTRL+C\n") ;
        print_to_file();
        pthread_mutex_unlock(&iden_cnt_lock);
    }
    exit(0) ;
}

void readDirectory(const char* dir_name)  
{
    DIR *dir = opendir(dir_name) ;
    if (dir == NULL) {
        printf("Failed to open a directory!\n") ;
        return ;
    }

    struct dirent *entry ;
    struct stat file_stat ;
    while ((entry = readdir(dir)) != NULL) 
    {
        if(strncmp(entry->d_name,".",1)==0 || strcmp(entry->d_name,"..")==0){
            continue;
        }

        char file_path[256] ;
        snprintf(file_path, sizeof(file_path), "%s/%s", dir_name, entry->d_name) ;

        if (lstat(file_path, &file_stat) == -1) {
            printf("Failed to get a file status!\n") ;
            continue ;
        }

        if (S_ISREG(file_stat.st_mode)) { // regular file
            if(file_stat.st_size < max_size) continue;

            size_t len = strlen(dir_name) + 1 + entry->d_reclen + 1 ;
            file_name[file_count] = (char *) malloc(len * sizeof(char)) ;
            strcpy(file_name[file_count], dir_name) ;
            strcat(file_name[file_count], "/") ;
            strcat(file_name[file_count], entry->d_name) ;
            file_count++ ;
        } 
        else if (S_ISDIR(file_stat.st_mode)) { // subdirectory 
            char *subdir[256] ;
            snprintf(subdir, sizeof(subdir), "%s/%s", dir_name, entry->d_name) ;
            readDirectory(subdir) ;
        }
        else if (S_ISLNK(file_stat.st_mode)) continue;
    }

    closedir(dir) ;
}

void _compare(int index) 
{
    FILE *fp1, *fp2 ;
    long fp1s, fp2s ;
    long fp1b, fp2b ;

    fp1 = fopen(file_name[index], "rb") ; // open pivot file
    if (fp1 == NULL) {
        printf("Failed to open a file %s!\n", file_name[index]) ;
        return ;
    }

    // get the file size
    fseek(fp1, 0L, SEEK_END) ; 
    fp1s = ftell(fp1) ; 
    rewind(fp1) ; 

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
        fseek(fp2, 0L, SEEK_END) ; 
        fp2s = ftell(fp2) ; 
        rewind(fp2) ; 

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
            pthread_mutex_lock(&file_cnt_lock);
            if(local_file_count == 0){

                pthread_mutex_lock(&iden_cnt_lock);
                    identical_count++ ;
                pthread_mutex_unlock(&iden_cnt_lock);

                identical[identical_count] = (char **) malloc(sizeof(char*));
                identical[identical_count][local_file_count] = (char *) malloc((strlen(file_name[index]) + 10) * sizeof(char)) ;
                strcpy(identical[identical_count][local_file_count], file_name[index]) ;
                
                local_file_count++;
            }
            identical[identical_count] = (char**)realloc(identical[identical_count],sizeof(char*)*(local_file_count+1));
            identical[identical_count][local_file_count] = (char *) malloc((strlen(file_name[i]) + 10) * sizeof(char)) ;
            strcpy(identical[identical_count][local_file_count], file_name[i]) ;
            
            local_file_count++;
            
            iden_file_count[identical_count] = local_file_count;
            
            pthread_mutex_unlock(&file_cnt_lock);
        }
    }
    
    fclose(fp1) ;
    fclose(fp2) ;
}

void * compare(void * arg){
    subtask * s = (subtask *)arg;
    int index = s->index;
    int used = s->used;

    free(arg) ;

    if(used == 0){
        _compare(index);
    }

    return NULL ;
}

void producer(int arg){
    subtask * s = (subtask *) malloc(sizeof(subtask));
    s->index = arg;
    s->used = 0;

    put_subtask(s);
}

void * worker (void * arg)
{
	subtask * s ;

	while ((s = (subtask *)get_subtask())) {
		compare(s) ;
	}
	return NULL ;
}

int 
main(int argc, char* argv[])
{
    int start_time = GetCurrentUsec();
    pthread_mutex_init(&iden_cnt_lock, NULL) ;
    pthread_mutex_init(&file_cnt_lock, NULL) ;
    pthread_mutex_init(&lock_n_threads, NULL) ;
    pthread_mutex_init(&subtasks_lock, NULL) ;

    signal(SIGALRM, sigalrm_handler) ;
    signal(SIGINT, sigint_handler) ;

    alarm(5) ;  
    
    option_parsing(argc, *&argv) ;
    display() ;

    if (d.tnum < 64) {
        max_threads = d.tnum ; 
    }
    pthread_t threads[max_threads];

    if (d.size < 1024) {
        max_size = d.size;
    }

    sem_init(&inused, 0, 0) ;
    sem_init(&unused, 0, max_threads) ;


    readDirectory(d.dir) ;

    for (int i = 0 ; i < file_count ; i++) {
        printf("%s\n", file_name[i]) ;
    }

    for (int i = 0 ; i < max_threads ; i++) {
		pthread_create(&(threads[i]), NULL, worker, NULL) ;
	}

    for(int i=0; i<file_count; i++){
       producer(i); 
    }

    for (int i = 0 ; i < max_threads ; i++) 
		put_subtask(NULL) ;


    for(int i = 0; i < max_threads; i++){
        pthread_mutex_lock(&lock_n_threads) ;
            pthread_join(threads[i], NULL) ;
        pthread_mutex_unlock(&lock_n_threads) ;
    }

    printf("Program finished\n");

    printf("\nNumber of identical files : %d\n", identical_count+1) ;

    print_to_file();
    
    int finish_time = GetCurrentUsec();

    printf("\n---------------------------------\n");
    printf("whole searching took %d sec\n", finish_time - start_time);
    printf("---------------------------------\n");
    
    return 0;
}