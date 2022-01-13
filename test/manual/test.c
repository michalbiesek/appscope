/*
 * Testing the operation of Interposing Functions in Dependent Libraries
 * The libscope library has intgerposed malloc. Call malloc and see that we get the interposed function.
 *
 * gcc test/manual/test.c -o fr
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <unistd.h>
void sig_handler(int signum){

  //Return type of the handler function should be void
  printf("\nInside handler function\n");
}

int main(int argc, char **argv)
{
    signal(SIGUSR1,sig_handler); // Register signal handler
    // printf("Starting interpose test\n");
    void *buf;
    if ((buf = malloc(1024)) == NULL) {
        perror("malloc");
        dup(5);
        dup2(1, 100);
        puts("test");
        putchar(58);
        return -1;
    }
    while (1) {
        // puts("..");
        // putchar(65);
        fflush(stdout);
        write(1, "..", 2);
        sleep(1);
    }

    return 0;
}
