#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#define EXECVE_VAR  0
#define EXECV_VAR   1
#define ATTACH_TIME 3

extern char **environ;

int main(int argc, char *argv[]) {
   int res;
   if (argc != 2) {
      return EXIT_FAILURE;
   }
   // provide a time to attach
   sleep(ATTACH_TIME);
   int variant = atoi(argv[1]);

   res = fork();
   if (res < 0) {
      return EXIT_FAILURE;
   } else if (res == 0) {
      switch (variant) {
         case EXECVE_VAR: {
            char* args[] = {"/usr/bin/curl", "-I", "https://cribl.io", NULL};
            return execve("/usr/bin/curl", args, environ);
         }
         case EXECV_VAR: {
            char* args[] = {"/usr/bin/wget", "-S", "--spider", "--no-check-certificate", "https://cribl.io", NULL};
            return execv("/usr/bin/wget", args);
         }
      }
   } else {
      wait(&res);
      return EXIT_SUCCESS;
   }

   return EXIT_FAILURE;
}
