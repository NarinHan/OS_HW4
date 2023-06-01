#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>

void spin()
{
    int i ;
    for (i = 0 ; i < 50000 ; i++) ;
}

void * print_message_function(void *ptr) 
{
    char *message ;
    message = strdup((char *) ptr) ;

    int i = 0 ;
    for (i = 0 ; i < 10 ; i++) {
        printf("%s\n", message) ;
        spin() ;
        sched_yield() ; // yield the processor
    }

    return (void *) message ;
}

int main()
{
    pthread_t thread1, thread2 ;
    
    char *message1 = "A" ;
    char *message2 = "B" ;

    // int pthread_create(pthread_t *restrict thread,
    //                    const pthread_attr_t *restrict attr,
    //                    void *(*start_routine)(void *),
    //                    void *restrict arg);
    pthread_create(&thread1, NULL, print_message_function, (void *) message1) ;
    pthread_create(&thread2, NULL, print_message_function, (void *) message2) ;

    char *r1, *r2 ;

    // The pthread_join() function waits for the thread specified by thread to terminate.
    // If retval is not NULL, then pthread_join() copies the exit status of the target thread 
    //    (i.e., the value that the target thread supplied to pthread_exit(3)) into the location pointed to by retval.
    
    // int pthread_join(pthread_t thread, void **retval);
    pthread_join(thread1, (void *) &r1) ;
    pthread_join(thread2, (void *) &r2) ;

    printf("Join return value r1 : %s\n", r1) ;
    printf("Join return value r2 : %s\n", r2) ;

    exit(0) ;
}