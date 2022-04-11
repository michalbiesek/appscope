#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "dbg.h"
#include "evtformat.h"
#include "scopestdlib.h"


static void
evtFormatMetricRateLimitCanBeTurnedOff(void)
{
    const unsigned ratelimit = 0; // 0 means "no limit"

    evt_fmt_t* evt = evtFormatCreate();
    evtFormatSourceEnabledSet(evt, CFG_SRC_METRIC, 1);
    evtFormatRateLimitSet(evt, ratelimit);

    event_t e = INT_EVENT("Hey", 1, DELTA, NULL);
    proc_id_t proc = {.pid = 4848,
                      .ppid = 4847,
                      .hostname = "host",
                      .procname = "evttest",
                      .cmd = "cmd-4",
                      .id = "host-evttest-cmd-4"};
    cJSON* json, *data;

    int i;
    for (i=0; i<=500000; i++) {  // 1/2 million is arbitrary
        json = evtFormatMetric(evt, &e, 12345, &proc);

        //printf("i=%d %s\n", i, msg);
        data = cJSON_GetObjectItem(json, "data");

        // Verify that data contains _metric, and not "Truncated"
        cJSON_HasObjectItem(data, "_metric");
        cJSON_IsString(data);
        cJSON_Delete(json);
    }

    evtFormatDestroy(&evt);
}

int
main(int argc, char* argv[])
{
    printf("running %s\n", argv[0]);
    evtFormatMetricRateLimitCanBeTurnedOff();
}
