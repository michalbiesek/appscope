/*
 * Testing unshared and scoping the process
 *
 * Start docker 
 * docker run --privileged --name testcont --rm -it ubuntu:22.04 bash
 * In separate terminal
 * retrieve pid of docker process docker top testcont
 * gcc test/manual/namespace/nstest.c -o nstest
 * nc -l -p 9109 will scope the events from first stage
 * sudo SCOPE_CRIBL_ENABLE=FALSE ldscope -- ./nstest <docker_pid>
 */
#define _GNU_SOURCE
#include <linux/limits.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define TEST_FILE "file.txt"
#define EXAMPLE_STR "Lorem ipsum dolor sit amet, consectetur adipiscing elit"

int main(int argc, char *argv[], char *envp[]) {
    char path[PATH_MAX] = {0};
    if (argc != 2) {
        printf("Usage: %s <pid for different network ns>\n", argv[0]);
        return EXIT_FAILURE;
    }
    snprintf(path, PATH_MAX, "/proc/%s/ns/net", argv[1]);
    printf("Starting first stage test\n");

    // Dummy operations before switching ns 
    for (int i = 0 ; i < 3; ++i) {
        FILE *fp = fopen(TEST_FILE,"w");
        if (!fp)
            return EXIT_FAILURE;
        fwrite(EXAMPLE_STR, sizeof(char), sizeof(EXAMPLE_STR), fp);
        fclose(fp);
        unlink(TEST_FILE);
        sleep(1);
    }

    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd == -1 ) {
        fprintf(stderr, "open");
        return EXIT_FAILURE;
    }

    if (setns(fd, 0) == -1) {
        fprintf(stderr, "setns");
        return EXIT_FAILURE;
    }

    printf("Starting second stage test\n");
    char *cmd = "/usr/bin/top";
    char *argvc[2];
    argvc[0] = "/usr/bin/top";
    argvc[1] = NULL;

    execve(cmd, argvc, envp);

    return EXIT_SUCCESS;
}
