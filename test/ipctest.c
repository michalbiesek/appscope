#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ipc.h"
#include "test.h"
#include "runtimecfg.h"
#include "cJSON.h"

#define TEST_MQ_GET_SCOPE_STATUS_REQ ("{\"req\":1,\"uniq\":1234,\"id\":1,\"max\":1,\"data\":\"test\"}")
#define TEST_SCOPE_GET_STATUS_REQ ("{\"req\":0"}")
#define TEST_SCOPE_GET_STATUS_RESPONSE  ("{\"status\":200,\"scoped\":false}")
#define TEST_MQ_UNKNOWN_REQ ("{\"req\":99999,\"uniq\":4567,\"id\":1,\"max\":1,\"data\":\"loremIpsum\"}")
#define TEST_SCOPE_UNKNOWN_REQ ("{\"req\":9999}")

rtconfig g_cfg = {0};

static void
ipcInactiveDesc(void **state) {
    size_t mqSize = 0;
    long msgCount = -1;
    mqd_t mqdes = (mqd_t)-1;
    bool res = ipcIsActive(mqdes, &mqSize, &msgCount);
    assert_false(res);
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
    long msgCount = -1;
    struct mq_attr attr;
    void *buf;
    ssize_t dataLen;

    // Setup read-only IPC
    mqReadDes = scope_mq_open(ipcConnName, O_RDONLY | O_CREAT | O_CLOEXEC | O_NONBLOCK, 0666, NULL);
    assert_int_not_equal(mqReadDes, -1);
    res = ipcIsActive(mqReadDes, &mqReadSize, &msgCount);
    assert_true(res);
    assert_int_equal(msgCount, 0);

    // Read-only IPC verify that is impossible to send msg to IPC
    scope_errno = 0;
    status = scope_mq_send(mqReadDes, "test", sizeof("test"), 0);
    assert_int_equal(scope_errno, EBADF);
    assert_int_equal(status, -1);

    // Setup write-only IPC
    mqWriteDes = ipcOpenWriteConnection(ipcConnName);
    assert_int_not_equal(mqWriteDes, -1);
    res = ipcIsActive(mqWriteDes, &mqWriteSize, &msgCount);
    assert_true(res);
    assert_int_equal(msgCount, 0);

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

    status = scope_mq_getattr(mqWriteDes, &attr);
    assert_int_equal(status, 0);
    assert_int_equal(attr.mq_curmsgs, 1);

    status = scope_mq_getattr(mqReadDes, &attr);
    assert_int_equal(status, 0);
    assert_int_equal(attr.mq_curmsgs, 1);

    buf = scope_malloc(attr.mq_msgsize);
    assert_non_null(buf);

    // Read-only IPC verify that it is possible to read msg from IPC
    dataLen = scope_mq_receive(mqReadDes, buf, attr.mq_msgsize, 0);
    assert_int_equal(dataLen, sizeof("test"));

    scope_free(buf);

    status = scope_mq_getattr(mqWriteDes, &attr);
    assert_int_equal(status, 0);
    assert_int_equal(attr.mq_curmsgs, 0);
    status = scope_mq_getattr(mqReadDes, &attr);
    assert_int_equal(status, 0);
    assert_int_equal(attr.mq_curmsgs, 0);

    // Closing IPC(s)
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
    req_parse_status_t parseStatus = REQ_PARSE_GENERIC_ERROR;
    int uniqueId = -1;
    struct mq_attr attr;
    char *scopeReq;

    mqReadWriteDes = scope_mq_open(ipcConnName, O_RDWR | O_CREAT | O_CLOEXEC | O_NONBLOCK, 0666, NULL);
    assert_int_not_equal(mqReadWriteDes, -1);

    status = scope_mq_getattr(mqReadWriteDes, &attr);
    assert_int_equal(status, 0);

    // Empty Message queue
    scopeReq = ipcMQRequestHandler(mqReadWriteDes, attr.mq_msgsize, &parseStatus, &uniqueId);
    assert_null(scopeReq);
    assert_int_equal(parseStatus, REQ_PARSE_INTERNAL_ERROR);
    assert_int_equal(uniqueId, -1);

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
    req_parse_status_t parseStatus = REQ_PARSE_GENERIC_ERROR;
    int uniqueId = -1;
    struct mq_attr attr;
    char *scopeReq;

    mqReadWriteDes = scope_mq_open(ipcConnName, O_RDWR | O_CREAT | O_CLOEXEC | O_NONBLOCK, 0666, NULL);
    assert_int_not_equal(mqReadWriteDes, -1);

    // Not JSON message on queue
    char msg[] = "test";
    status = scope_mq_send(mqReadWriteDes, msg, sizeof(msg), 0);
    assert_int_equal(status, 0);

    status = scope_mq_getattr(mqReadWriteDes, &attr);
    assert_int_equal(status, 0);

    scopeReq = ipcMQRequestHandler(mqReadWriteDes, attr.mq_msgsize, &parseStatus, &uniqueId);
    assert_null(scopeReq);
    assert_int_equal(parseStatus, REQ_PARSE_JSON_ERROR);
    assert_int_equal(uniqueId, -1);

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
    struct mq_attr attr;
    req_parse_status_t parseStatus = REQ_PARSE_GENERIC_ERROR;
    int uniqueId = -1;
    char *scopeReq;

    mqReadWriteDes = scope_mq_open(ipcConnName, O_RDWR | O_CREAT | O_CLOEXEC | O_NONBLOCK, 0666, NULL);
    assert_int_not_equal(mqReadWriteDes, -1);

    // Empty JSON message on queue
    char msg[] = "{}";
    status = scope_mq_send(mqReadWriteDes, msg, sizeof(msg), 0);
    assert_int_equal(status, 0);

    status = scope_mq_getattr(mqReadWriteDes, &attr);
    assert_int_equal(status, 0);

    scopeReq = ipcMQRequestHandler(mqReadWriteDes, attr.mq_msgsize, &parseStatus, &uniqueId);
    assert_null(scopeReq);
    assert_int_equal(parseStatus, REQ_PARSE_MISS_MANDATORY_ERROR);
    assert_int_equal(uniqueId, -1);

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
    req_parse_status_t parseStatus = REQ_PARSE_GENERIC_ERROR;
    int uniqueId = -1;
    char *scopeReq;
    struct mq_attr attr;

    mqReadWriteDes = scope_mq_open(ipcConnName, O_RDWR | O_CREAT | O_CLOEXEC | O_NONBLOCK, 0666, NULL);
    assert_int_not_equal(mqReadWriteDes, -1);

    // Get Scope Status message on queue
    size_t msgLen = scope_strlen(TEST_MQ_GET_SCOPE_STATUS_REQ);
    status = scope_mq_send(mqReadWriteDes, TEST_MQ_GET_SCOPE_STATUS_REQ, msgLen, 0);
    assert_int_equal(status, 0);

    status = scope_mq_getattr(mqReadWriteDes, &attr);
    assert_int_equal(status, 0);

    scopeReq = ipcMQRequestHandler(mqReadWriteDes, attr.mq_msgsize, &parseStatus, &uniqueId);
    assert_non_null(scopeReq);
    assert_string_equal(scopeReq, "test");
    assert_int_equal(parseStatus, REQ_PARSE_OK);
    assert_int_equal(uniqueId, 1234);

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
    req_parse_status_t parseStatus = REQ_PARSE_GENERIC_ERROR;
    int uniqueId = -1;
    struct mq_attr attr;
    char *scopeReq;

    mqReadWriteDes = scope_mq_open(ipcConnName, O_RDWR | O_CREAT | O_CLOEXEC | O_NONBLOCK, 0666, NULL);
    assert_int_not_equal(mqReadWriteDes, -1);

    status = scope_mq_send(mqReadWriteDes, TEST_MQ_UNKNOWN_REQ, sizeof(TEST_MQ_UNKNOWN_REQ), 0);
    assert_int_equal(status, 0);

    status = scope_mq_getattr(mqReadWriteDes, &attr);
    assert_int_equal(status, 0);

    scopeReq = ipcMQRequestHandler(mqReadWriteDes, attr.mq_msgsize, &parseStatus, &uniqueId);
    assert_non_null(scopeReq);
    assert_string_equal(scopeReq, "loremIpsum");
    assert_int_equal(parseStatus, REQ_PARSE_OK);
    assert_int_equal(uniqueId, 4567);

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
    char *res;
    int uniqueId;
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

    // Get Scope Status Scope message
    res = ipcResponseMQHandler(mqReadWriteDes, attr.mq_msgsize, TEST_MQ_GET_SCOPE_STATUS_REQ, REQ_PARSE_OK, uniqueId);
    assert_null(res);
    status = scope_mq_getattr(mqReadWriteDes, &attr);
    assert_int_equal(status, 0);
    assert_int_equal(attr.mq_curmsgs, 0);

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
    char *res;
    struct mq_attr attr;
    long msgCount;
    int uniqueId = 9999;

    mqReadDes = scope_mq_open(ipcConnName, O_RDONLY | O_CREAT | O_CLOEXEC | O_NONBLOCK, 0666, NULL);
    assert_int_not_equal(mqReadDes, -1);

    status = scope_mq_getattr(mqReadDes, &attr);
    assert_int_equal(status, 0);

    res = ipcResponseMQHandler(mqReadDes, attr.mq_msgsize, TEST_SCOPE_GET_STATUS_RESPONSE, REQ_PARSE_OK, uniqueId);
    assert_null(res);
    status = scope_mq_getattr(mqReadDes, &attr);
    assert_int_equal(status, 0);
    assert_int_equal(attr.mq_curmsgs, 0);

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
    char *res;
    long msgCount;
    void *buf;
    struct mq_attr attr;
    ssize_t dataLen;
    int uniqueId = -1;

    mqReadWriteDes = scope_mq_open(ipcConnName, O_RDWR | O_CREAT | O_CLOEXEC | O_NONBLOCK, 0666, NULL);
    assert_int_not_equal(mqReadWriteDes, -1);

    status = scope_mq_getattr(mqReadWriteDes, &attr);
    assert_int_equal(status, 0);

    res = ipcResponseMQHandler(mqReadWriteDes, attr.mq_msgsize, TEST_MQ_GET_SCOPE_STATUS_REQ, REQ_PARSE_OK, uniqueId);
    assert_null(res);
    status = scope_mq_getattr(mqReadWriteDes, &attr);
    assert_int_equal(status, 0);
    assert_int_equal(attr.mq_curmsgs, 1);


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
ipcHandlerResponseUnknown(void **state) {
    const char *ipcConnName = "/testConnection";
    int status;
    mqd_t mqReadWriteDes;
    char *res;
    void *buf;
    struct mq_attr attr;
    ssize_t dataLen;
    int uniqueId = -1;

    mqReadWriteDes = scope_mq_open(ipcConnName, O_RDWR | O_CREAT | O_CLOEXEC | O_NONBLOCK, 0666, NULL);
    assert_int_not_equal(mqReadWriteDes, -1);

    status = scope_mq_getattr(mqReadWriteDes, &attr);
    assert_int_equal(status, 0);

    // Handle Scope response unknown
    res = ipcResponseMQHandler(mqReadWriteDes, attr.mq_msgsize, TEST_SCOPE_UNKNOWN_REQ, REQ_PARSE_OK, uniqueId);
    assert_non_null(res);
    status = scope_mq_getattr(mqReadWriteDes, &attr);
    assert_int_equal(status, 0);
    assert_int_equal(attr.mq_curmsgs, 1);

    status = scope_mq_getattr(mqReadWriteDes, &attr);
    assert_int_equal(status, 0);

    buf = scope_malloc(attr.mq_msgsize);
    assert_non_null(buf);

    dataLen = scope_mq_receive(mqReadWriteDes, buf, attr.mq_msgsize, 0);
    assert_int_not_equal(dataLen, -1);

    // Handle Single Mq Response (client side)
    cJSON *item;
    cJSON *mqResp = cJSON_Parse(buf);
    assert_non_null(mqResp);
    item = cJSON_GetObjectItemCaseSensitive(mqResp, "status");
    assert_non_null(item);
    assert_true(cJSON_IsNumber(item));
    item = cJSON_GetObjectItemCaseSensitive(mqResp, "uniq");
    assert_non_null(item);
    assert_true(cJSON_IsNumber(item));
    assert_true(cJSON_HasObjectItem(mqResp, "id"));
    item = cJSON_GetObjectItemCaseSensitive(mqResp, "id");
    assert_non_null(item);
    assert_true(cJSON_IsNumber(item));
    assert_true(cJSON_HasObjectItem(mqResp, "max"));
    item = cJSON_GetObjectItemCaseSensitive(mqResp, "max");
    assert_non_null(item);
    assert_true(cJSON_IsNumber(item));
    item = cJSON_GetObjectItemCaseSensitive(mqResp, "data");
    assert_non_null(item);
    assert_true(cJSON_IsString(item));

    cJSON *scopeResp = cJSON_Parse(item);
    item = cJSON_GetObjectItemCaseSensitive(mqResp, "status");
    assert_non_null(item);
    assert_true(cJSON_IsNumber(item));
    //Method not implemented
    assert_int_equal(item->valueint, 501);
    cJSON_Delete(scopeResp);
    cJSON_Delete(mqResp);

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
        cmocka_unit_test(ipcOpenNonExistingConnection),
        cmocka_unit_test(ipcCommunicationTest),
        cmocka_unit_test(ipcHandlerRequestEmptyQueue),
        cmocka_unit_test(ipcHandlerRequestNotJson),
        cmocka_unit_test(ipcHandlerRequestMissingMandatory),
        cmocka_unit_test(ipcHandlerRequestKnown),
        cmocka_unit_test(ipcHandlerRequestUnknown),
        // cmocka_unit_test(ipcHandlerResponseQueueTooSmallForResponse),
        // cmocka_unit_test(ipcHandlerResponseFail),
        // cmocka_unit_test(ipcHandlerResponseValid),
        cmocka_unit_test(ipcHandlerResponseUnknown),
        cmocka_unit_test(dbgHasNoUnexpectedFailures),
    };
    return cmocka_run_group_tests(tests, groupSetup, groupTeardown);
}
