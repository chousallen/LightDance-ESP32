// TODO:
// 1. implement play(), pause(), stop() api's
// 2. be careful of the return value

#include <stdio.h>  // sscanf
#include <string.h> // strncmp, strcmp, strncpy, strtok
#include <stdlib.h> // atoi, strtoll
#include <stdint.h> // int64_t
#include <sys/socket.h>

#include "esp_log.h"   // ESP_LOGI, ESP_LOGW
#include "esp_timer.h" // esp_timer_get_time

#include "parsing_component.h" // (if you expose process_line or related)

static const char *TAG = "PARSING_COMPONENT";
static int64_t offset = 0;

void send_sync_with_t1(int sock)
{
    int64_t t_1 = esp_timer_get_time();
    char line[128];
    snprintf(line, sizeof(line), "sync -t_1 %lld\n", (long long)t_1);
    send(sock, line, strlen(line), 0);
}

void process_line(int sock, char *line)
{
    // line is null terminated without trailing \n
    if (strncmp(line, "syncresp", 8) == 0)
    {
        // Expected: syncresp -t1 <t1> -t2 <t2> -t3 <t3>
        int64_t t1 = 0, t2 = 0, t3 = 0;
        char *p = line + 8;
        while (*p)
        {
            while (*p == ' ')
                p++;
            if (strncmp(p, "-t1", 3) == 0)
            {
                p += 3;
                while (*p == ' ')
                    p++;
                if (*p)
                {
                    t1 = strtoll(p, &p, 10);
                }
            }
            else if (strncmp(p, "-t2", 3) == 0)
            {
                p += 3;
                while (*p == ' ')
                    p++;
                if (*p)
                {
                    t2 = strtoll(p, &p, 10);
                }
            }
            else if (strncmp(p, "-t3", 3) == 0)
            {
                p += 3;
                while (*p == ' ')
                    p++;
                if (*p)
                {
                    t3 = strtoll(p, &p, 10);
                }
            }
            else
            {
                while (*p && *p != ' ')
                    p++; // skip unknown token
            }
        }
        int64_t t4 = esp_timer_get_time();
        int64_t new_offset = ((t2 - t1) + (t3 - t4)) / 2;
        int64_t rtt = (t4 - t1) - (t3 - t2);
        offset = new_offset;
        ESP_LOGI(TAG, "SYNCRESP: t1=%lld t2=%lld t3=%lld t4=%lld offset=%lld rtt=%lld",
                 (long long)t1, (long long)t2, (long long)t3, (long long)t4, (long long)offset, (long long)rtt);
        return;
    }
    if (strncmp(line, "sync", 4) == 0)
    {
        // Server asked sync without times: respond with sync -t_1 <t1>
        send_sync_with_t1(sock);
        ESP_LOGI(TAG, "SYNC");
        return;
    }
    if (strncmp(line, "play", 4) == 0)
    {
        int starttime = 0, endtime = 0, d = 0, dd = 0;
        sscanf(line, "play -ss%d -end%d -d%d -dd%d", &starttime, &endtime, &d, &dd);
        ESP_LOGI(TAG, "PLAY: ss=%d end=%d d=%d dd=%d", starttime, endtime, d, dd);
        return;
    }
    if (strcmp(line, "pause") == 0)
    {
        ESP_LOGI(TAG, "PAUSE");
        return;
    }
    if (strcmp(line, "stop") == 0)
    {
        ESP_LOGI(TAG, "STOP");
        return;
    }
    if (strncmp(line, "parttest", 8) == 0)
    {
        int channel = 0;
        int r = 255, g = 255, b = 255;
        char local[256];
        strncpy(local, line, sizeof(local) - 1);
        local[sizeof(local) - 1] = '\0';
        char *tok = strtok(local, " "); // parttest
        tok = strtok(NULL, " ");
        while (tok)
        {
            if (strcmp(tok, "-c") == 0)
            {
                char *v = strtok(NULL, " ");
                if (v)
                    channel = atoi(v);
            }
            else if (strcmp(tok, "-rgb") == 0)
            {
                char *sr = strtok(NULL, " ");
                char *sg = strtok(NULL, " ");
                char *sb = strtok(NULL, " ");
                if (sr && sg && sb)
                {
                    r = atoi(sr);
                    g = atoi(sg);
                    b = atoi(sb);
                }
            }
            tok = strtok(NULL, " ");
        }
        ESP_LOGI(TAG, "PARTTEST: c=%d rgb=(%d,%d,%d)", channel, r, g, b);
        return;
    }
    ESP_LOGW(TAG, "Unknown line: %s", line);
}
