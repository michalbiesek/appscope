/*
 * ipcclient - IPC client
 *
 * A simple program to test communication between scoped process using 
 * gcc -g test/manual/ipcclient.c -lrt -o ipcclient
 * ./ipcclient <scoped_PID>
 */

#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <string.h>
#include <sys/stat.h>

#define MAX_MESSAGES 10
#define MSG_BUFFER_SIZE 8192

static mqd_t readMqDesc;
#define READ_MQ_NAME  "/ScopeCLI"

void cleanupReadDesc(void) {
    mq_close(readMqDesc);
    mq_unlink(READ_MQ_NAME);
}

int main(int argc, char **argv) {
    struct mq_attr attr = {.mq_flags = 0, 
                               .mq_maxmsg = MAX_MESSAGES,
                               .mq_msgsize = MSG_BUFFER_SIZE,
                               .mq_curmsgs = 0};

    int res = EXIT_FAILURE;
    char WriteMqName[4096] = {0};
    if (argc != 2) {
        printf("Usage: %s <pid_scope_process>\n", argv[0]);
        return res;
    }

    snprintf(WriteMqName, sizeof(WriteMqName), "/ScopeIPC.%s", argv[1]);

    mqd_t writeMqDesc;

    // Ugly hack disable umask to handle run as a root
    mode_t oldMask = umask(0);
    readMqDesc = mq_open(READ_MQ_NAME, O_RDONLY | O_CREAT, 0666, &attr);
    if (readMqDesc == (mqd_t)-1) {
        perror("!mq_open readMqDesc failed");
        return res;
    }
    umask(oldMask);

    atexit(cleanupReadDesc);

    writeMqDesc = mq_open(WriteMqName, O_WRONLY);
    if (writeMqDesc == (mqd_t)-1) {
        perror("!mq_open writeMqDesc failed");
        return res;
    }

    char Txbuf[MSG_BUFFER_SIZE] = {0};
    char RxBuf[MSG_BUFFER_SIZE] = {0};

    printf("\nPass example message to stdin [type 'quit' to stop]\n");
    while (fgets(Txbuf, MSG_BUFFER_SIZE, stdin) != NULL) {
        if(strcmp("quit\n", Txbuf) == 0) {
            break;
        }

        // Send message to scoped process
        if (mq_send(writeMqDesc, Txbuf, strlen(Txbuf) + 1, 0) == -1) {
            perror("!mq_send writeMqDesc failed");
            goto end_iteration;
        }


        // Read response
        if (mq_receive(readMqDesc, RxBuf, MSG_BUFFER_SIZE, NULL) == -1) {
            perror("!mq_receive readMqDesc failed");
            goto end_iteration;
        }

        printf("Response from pid process %s : %s", argv[1], RxBuf);

        end_iteration:
            printf("\nPass example message to stdin [type 'quit' to stop]\n");
            memset(Txbuf, 0, MSG_BUFFER_SIZE);
            memset(RxBuf, 0, MSG_BUFFER_SIZE);
    }

    if (mq_close(writeMqDesc) == -1) {
        perror ("!mq_close writeMqDesc failed");
    }

    return EXIT_SUCCESS;
}
