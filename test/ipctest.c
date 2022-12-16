#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ipc.h"
#include "test.h"
#include "runtimecfg.h"
#include "cJSON.h"

rtconfig g_cfg = {0};

static void
ipcInactiveDesc(void **state) {
    size_t mqSize;
    mqd_t mqdes = (mqd_t)-1;
    bool res = ipcIsActive(mqdes, &mqSize);
    assert_false(res);
}

static void
ipcInfoMsgCountNonExisting(void **state) {
    long msgCount = ipcInfoMsgCount((mqd_t)-1);
    assert_int_equal(msgCount, -1);
}

static void
ipcOpenNonExistingConnection(void **state) {
    mqd_t mqDes = ipcOpenWriteConnection("/NonExistingConnection");
    assert_int_equal(mqDes, -1);
}

static void
ipcCommunicationTest(void **state) {
    const char *ipcConnName = "/testConnection";
    int status;
    bool res;
    mqd_t mqWriteDes, mqReadDes;
    size_t mqWriteSize, mqReadSize;
    long msgCount;
    struct mq_attr attr;
    void *buf;
    ssize_t dataLen;

    // Setup read-only IPC
    mqReadDes = scope_mq_open(ipcConnName, O_RDONLY | O_CREAT | O_CLOEXEC | O_NONBLOCK, 0666, NULL);
    assert_int_not_equal(mqReadDes, -1);
    res = ipcIsActive(mqReadDes, &mqReadSize);
    assert_true(res);
    msgCount = ipcInfoMsgCount(mqReadDes);
    assert_int_equal(msgCount, 0);

    // Read-only IPC verify that is impossible to send msg to IPC
    scope_errno = 0;
    status = scope_mq_send(mqReadDes, "test", sizeof("test"), 0);
    assert_int_equal(scope_errno, EBADF);
    assert_int_equal(status, -1);

    // Setup write-only IPC
    mqWriteDes = ipcOpenWriteConnection(ipcConnName);
    assert_int_not_equal(mqWriteDes, -1);
    res = ipcIsActive(mqWriteDes, &mqWriteSize);
    assert_true(res);

    // Write-only IPC verify that it is possible to send msg to IPC
    status = scope_mq_send(mqWriteDes, "test", sizeof("test"), 0);
    assert_int_not_equal(status, -1);

    status = scope_mq_getattr(mqWriteDes, &attr);
    assert_int_equal(status, 0);

    buf = scope_malloc(attr.mq_msgsize);
    assert_non_null(buf);

    // Write-only IPC verify that is impossible to read msg from IPC
    scope_errno = 0;
    dataLen = scope_mq_receive(mqWriteDes, buf, attr.mq_msgsize, 0);
    assert_int_equal(scope_errno, EBADF);
    assert_int_equal(dataLen, -1);

    scope_free(buf);

    msgCount = ipcInfoMsgCount(mqWriteDes);
    assert_int_equal(msgCount, 1);
    msgCount = ipcInfoMsgCount(mqReadDes);
    assert_int_equal(msgCount, 1);

    status = scope_mq_getattr(mqReadDes, &attr);
    assert_int_equal(status, 0);

    buf = scope_malloc(attr.mq_msgsize);
    assert_non_null(buf);

    // Read-only IPC verify that it is possible to read msg from IPC
    dataLen = scope_mq_receive(mqReadDes, buf, attr.mq_msgsize, 0);
    assert_int_equal(dataLen, sizeof("test"));

    scope_free(buf);

    msgCount = ipcInfoMsgCount(mqWriteDes);
    assert_int_equal(msgCount, 0);
    msgCount = ipcInfoMsgCount(mqReadDes);
    assert_int_equal(msgCount, 0);

    // Teardown IPC(s)
    status = ipcCloseConnection(mqWriteDes);
    assert_int_equal(status, 0);

    status = ipcCloseConnection(mqReadDes);
    assert_int_equal(status, 0);
}

static void
ipcHandlerRequestEmptyQueue(void **state) {
    const char *ipcConnName = "/testConnection";
    int status;
    mqd_t mqReadWriteDes;
    ipc_cmd_t cmd;
    struct mq_attr attr;
    ipc_req_status_t res;

    mqReadWriteDes = scope_mq_open(ipcConnName, O_RDWR | O_CREAT | O_CLOEXEC | O_NONBLOCK, 0666, NULL);
    assert_int_not_equal(mqReadWriteDes, -1);

    status = scope_mq_getattr(mqReadWriteDes, &attr);
    assert_int_equal(status, 0);

    // Empty Message queue
    res = ipcRequestMsgHandler(mqReadWriteDes, attr.mq_msgsize, &cmd);
    assert_int_equal(res, IPC_REQ_INTERNAL_ERROR);

    status = scope_mq_close(mqReadWriteDes);
    assert_int_equal(status, 0);
    status = scope_mq_unlink(ipcConnName);
    assert_int_equal(status, 0);
}

static void
ipcHandlerRequestNotJson(void **state) {
    const char *ipcConnName = "/testConnection";
    int status;
    mqd_t mqReadWriteDes;
    ipc_cmd_t cmd;
    struct mq_attr attr;
    ipc_req_status_t res;

    mqReadWriteDes = scope_mq_open(ipcConnName, O_RDWR | O_CREAT | O_CLOEXEC | O_NONBLOCK, 0666, NULL);
    assert_int_not_equal(mqReadWriteDes, -1);

    // Not Json message on queue
    char msg[] = "test";
    status = scope_mq_send(mqReadWriteDes, msg, sizeof(msg), 0);
    assert_int_equal(status, 0);

    status = scope_mq_getattr(mqReadWriteDes, &attr);
    assert_int_equal(status, 0);

    res = ipcRequestMsgHandler(mqReadWriteDes, attr.mq_msgsize, &cmd);
    assert_int_equal(res, IPC_REQ_NOT_JSON_ERROR);

    status = scope_mq_close(mqReadWriteDes);
    assert_int_equal(status, 0);
    status = scope_mq_unlink(ipcConnName);
    assert_int_equal(status, 0);
}

static void
ipcHandlerRequestMissingMandatory(void **state) {
    const char *ipcConnName = "/testConnection";
    int status;
    mqd_t mqReadWriteDes;
    ipc_cmd_t cmd;
    struct mq_attr attr;
    ipc_req_status_t res;

    mqReadWriteDes = scope_mq_open(ipcConnName, O_RDWR | O_CREAT | O_CLOEXEC | O_NONBLOCK, 0666, NULL);
    assert_int_not_equal(mqReadWriteDes, -1);

    // Valid message on queue
    char msg[] = "{}";
    status = scope_mq_send(mqReadWriteDes, msg, sizeof(msg), 0);
    assert_int_equal(status, 0);

    status = scope_mq_getattr(mqReadWriteDes, &attr);
    assert_int_equal(status, 0);

    res = ipcRequestMsgHandler(mqReadWriteDes, attr.mq_msgsize, &cmd);
    assert_int_equal(res, IPC_REQ_MANDATORY_FIELD_ERROR);

    status = scope_mq_close(mqReadWriteDes);
    assert_int_equal(status, 0);
    status = scope_mq_unlink(ipcConnName);
    assert_int_equal(status, 0);
}

static void
ipcHandlerRequestKnown(void **state) {
    const char *ipcConnName = "/testConnection";
    int status;
    mqd_t mqReadWriteDes;
    ipc_cmd_t cmd;
    struct mq_attr attr;
    ipc_req_status_t res;
    char msgBuf[1024] = {0};

    mqReadWriteDes = scope_mq_open(ipcConnName, O_RDWR | O_CREAT | O_CLOEXEC | O_NONBLOCK, 0666, NULL);
    assert_int_not_equal(mqReadWriteDes, -1);

    // Valid message on queue
    scope_sprintf(msgBuf, "{\"req\": %d}", IPC_CMD_GET_SCOPE_STATUS);
    size_t msgLen = scope_strlen(msgBuf);
    status = scope_mq_send(mqReadWriteDes, msgBuf, msgLen, 0);
    assert_int_equal(status, 0);

    status = scope_mq_getattr(mqReadWriteDes, &attr);
    assert_int_equal(status, 0);

    res = ipcRequestMsgHandler(mqReadWriteDes, attr.mq_msgsize, &cmd);
    assert_int_equal(res, IPC_REQ_OK);
    assert_int_equal(cmd, IPC_CMD_GET_SCOPE_STATUS);

    status = scope_mq_close(mqReadWriteDes);
    assert_int_equal(status, 0);
    status = scope_mq_unlink(ipcConnName);
    assert_int_equal(status, 0);
}

static void
ipcHandlerRequestUnknown(void **state) {
    const char *ipcConnName = "/testConnection";
    int status;
    mqd_t mqReadWriteDes;
    ipc_cmd_t cmd;
    struct mq_attr attr;
    ipc_req_status_t res;

    mqReadWriteDes = scope_mq_open(ipcConnName, O_RDWR | O_CREAT | O_CLOEXEC | O_NONBLOCK, 0666, NULL);
    assert_int_not_equal(mqReadWriteDes, -1);

    // Unkonwn message on queue
    char msg[] = "{\"req\": 100000}";
    status = scope_mq_send(mqReadWriteDes, msg, sizeof(msg), 0);
    assert_int_equal(status, 0);

    status = scope_mq_getattr(mqReadWriteDes, &attr);
    assert_int_equal(status, 0);

    res = ipcRequestMsgHandler(mqReadWriteDes, attr.mq_msgsize, &cmd);
    assert_int_equal(res, IPC_REQ_OK);
    assert_int_equal(cmd, IPC_CMD_UNKNOWN);

    status = scope_mq_close(mqReadWriteDes);
    assert_int_equal(status, 0);
    status = scope_mq_unlink(ipcConnName);
    assert_int_equal(status, 0);
}

static void
ipcHandlerResponseQueueTooSmallForResponse(void **state) {
    const char *ipcConnName = "/testConnection";
    int status;
    mqd_t mqReadWriteDes;
    bool res;
    long msgCount;

    // Limit the parameter so small that it is too small to handle any response
    const long maxMsgSize = 3;
    struct mq_attr attr = {.mq_flags = 0, 
                           .mq_maxmsg = 10,
                           .mq_msgsize = maxMsgSize,
                           .mq_curmsgs = 0};


    mqReadWriteDes = scope_mq_open(ipcConnName, O_RDWR | O_CREAT | O_CLOEXEC | O_NONBLOCK, 0666, &attr);
    assert_int_not_equal(mqReadWriteDes, -1);

    status = scope_mq_getattr(mqReadWriteDes, &attr);
    assert_int_equal(status, 0);

    res = ipcResponseMsgHandler(mqReadWriteDes, attr.mq_msgsize, IPC_CMD_GET_SCOPE_STATUS, IPC_REQ_OK);
    assert_false(res);
    msgCount = ipcInfoMsgCount(mqReadWriteDes);
    assert_int_equal(msgCount, 0);

    status = scope_mq_close(mqReadWriteDes);
    assert_int_equal(status, 0);
    status = scope_mq_unlink(ipcConnName);
    assert_int_equal(status, 0);
}

static void
ipcHandlerResponseFail(void **state) {
    const char *ipcConnName = "/testConnection";
    int status;
    mqd_t mqReadDes;
    bool res;
    struct mq_attr attr;
    long msgCount;

    mqReadDes = scope_mq_open(ipcConnName, O_RDONLY | O_CREAT | O_CLOEXEC | O_NONBLOCK, 0666, NULL);
    assert_int_not_equal(mqReadDes, -1);

    status = scope_mq_getattr(mqReadDes, &attr);
    assert_int_equal(status, 0);

    res = ipcResponseMsgHandler(mqReadDes, attr.mq_msgsize, IPC_CMD_GET_SCOPE_STATUS, IPC_REQ_OK);
    assert_false(res);
    msgCount = ipcInfoMsgCount(mqReadDes);
    assert_int_equal(msgCount, 0);

    status = scope_mq_close(mqReadDes);
    assert_int_equal(status, 0);
    status = scope_mq_unlink(ipcConnName);
    assert_int_equal(status, 0);
}

static void
ipcHandlerResponseValid(void **state) {
    const char *ipcConnName = "/testConnection";
    int status;
    mqd_t mqReadWriteDes;
    bool res;
    long msgCount;
    void *buf;
    struct mq_attr attr;
    ssize_t dataLen;

    mqReadWriteDes = scope_mq_open(ipcConnName, O_RDWR | O_CREAT | O_CLOEXEC | O_NONBLOCK, 0666, NULL);
    assert_int_not_equal(mqReadWriteDes, -1);

    status = scope_mq_getattr(mqReadWriteDes, &attr);
    assert_int_equal(status, 0);

    res = ipcResponseMsgHandler(mqReadWriteDes, attr.mq_msgsize, IPC_CMD_GET_SCOPE_STATUS, IPC_REQ_OK);
    assert_true(res);
    msgCount = ipcInfoMsgCount(mqReadWriteDes);
    assert_int_equal(msgCount, 1);


    status = scope_mq_getattr(mqReadWriteDes, &attr);
    assert_int_equal(status, 0);

    buf = scope_malloc(attr.mq_msgsize);
    assert_non_null(buf);

    dataLen = scope_mq_receive(mqReadWriteDes, buf, attr.mq_msgsize, 0);
    assert_int_not_equal(dataLen, -1);

    char resp[] = "{\"status\":200,\"scoped\":false}";
    assert_int_equal(dataLen, sizeof(resp)-1);

    status = scope_memcmp(buf, resp, dataLen);
    assert_int_equal(status, 0);

    scope_free(buf);

    status = scope_mq_close(mqReadWriteDes);
    assert_int_equal(status, 0);
    status = scope_mq_unlink(ipcConnName);
    assert_int_equal(status, 0);
}

static void
ipcHandlerResponseValidMultipleFrames(void **state) {
    const char *ipcConnName = "/testConnection";
    int status;
    mqd_t mqReadWriteDes;
    bool res;
    long msgCount;
    void *buf;
    ssize_t dataLen;
    char testBuf[4096] = {0};
    const long maxMsgSize = 52;
    struct mq_attr attr = {.mq_flags = 0, 
                           .mq_maxmsg = 10,
                           .mq_msgsize = maxMsgSize,
                           .mq_curmsgs = 0};

    mqReadWriteDes = scope_mq_open(ipcConnName, O_RDWR | O_CREAT | O_CLOEXEC | O_NONBLOCK, 0666, &attr);
    assert_int_not_equal(mqReadWriteDes, -1);

    status = scope_mq_getattr(mqReadWriteDes, &attr);
    assert_int_equal(status, 0);

    res = ipcResponseMsgHandler(mqReadWriteDes, attr.mq_msgsize, IPC_CMD_GET_SCOPE_CFG, IPC_REQ_OK);
    assert_true(res);
    msgCount = ipcInfoMsgCount(mqReadWriteDes);

    status = scope_mq_getattr(mqReadWriteDes, &attr);
    assert_int_equal(status, 0);

    buf = scope_malloc(attr.mq_msgsize);
    assert_non_null(buf);
    size_t testOffset = 0;
    for (int i = 0; i < msgCount; ++i) {
        dataLen = scope_mq_receive(mqReadWriteDes, buf, attr.mq_msgsize, 0);
        assert_int_not_equal(dataLen, -1);
        scope_memcpy(testBuf+testOffset, buf, dataLen);
    }

    scope_free(buf);

    status = scope_mq_close(mqReadWriteDes);
    assert_int_equal(status, 0);
    status = scope_mq_unlink(ipcConnName);
    assert_int_equal(status, 0);
}



static void
ipcHandlerResponseUnknown(void **state) {
    const char *ipcConnName = "/testConnection";
    int status;
    mqd_t mqReadWriteDes;
    bool res;
    long msgCount;
    void *buf;
    struct mq_attr attr;
    ssize_t dataLen;

    mqReadWriteDes = scope_mq_open(ipcConnName, O_RDWR | O_CREAT | O_CLOEXEC | O_NONBLOCK, 0666, NULL);
    assert_int_not_equal(mqReadWriteDes, -1);

    status = scope_mq_getattr(mqReadWriteDes, &attr);
    assert_int_equal(status, 0);

    res = ipcResponseMsgHandler(mqReadWriteDes, attr.mq_msgsize, IPC_CMD_UNKNOWN, IPC_REQ_OK);
    assert_true(res);
    msgCount = ipcInfoMsgCount(mqReadWriteDes);
    assert_int_equal(msgCount, 1);


    status = scope_mq_getattr(mqReadWriteDes, &attr);
    assert_int_equal(status, 0);

    buf = scope_malloc(attr.mq_msgsize);
    assert_non_null(buf);

    dataLen = scope_mq_receive(mqReadWriteDes, buf, attr.mq_msgsize, 0);
    assert_int_not_equal(dataLen, -1);

    char resp[] = "{\"status\":501}";
    assert_int_equal(dataLen, sizeof(resp)-1);

    status = scope_memcmp(buf, resp, dataLen);
    assert_int_equal(status, 0);

    scope_free(buf);

    status = scope_mq_close(mqReadWriteDes);
    assert_int_equal(status, 0);
    status = scope_mq_unlink(ipcConnName);
    assert_int_equal(status, 0);
}

int
main(int argc, char* argv[]) {
    printf("running %s\n", argv[0]);

    const struct CMUnitTest tests[] = {
        cmocka_unit_test(ipcInactiveDesc),
        cmocka_unit_test(ipcInfoMsgCountNonExisting),
        cmocka_unit_test(ipcOpenNonExistingConnection),
        cmocka_unit_test(ipcCommunicationTest),
        cmocka_unit_test(ipcHandlerRequestEmptyQueue),
        cmocka_unit_test(ipcHandlerRequestNotJson),
        cmocka_unit_test(ipcHandlerRequestMissingMandatory),
        cmocka_unit_test(ipcHandlerRequestKnown),
        cmocka_unit_test(ipcHandlerRequestUnknown),
        cmocka_unit_test(ipcHandlerResponseQueueTooSmallForResponse),
        cmocka_unit_test(ipcHandlerResponseFail),
        cmocka_unit_test(ipcHandlerResponseValid),
        // cmocka_unit_test(ipcHandlerResponseValidMultipleFrames),
        cmocka_unit_test(ipcHandlerResponseUnknown),
        cmocka_unit_test(dbgHasNoUnexpectedFailures),
    };
    return cmocka_run_group_tests(tests, groupSetup, groupTeardown);
}
