#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>

bool lid_opened()
{
    FILE* f = fopen("/proc/acpi/button/lid/LID/state", "r");
    if (f == 0) return true;

    char buf[1024];
    size_t len = fread(buf, 1, sizeof(buf), f);
    if (len == 0)
    {
        fclose(f);
        return true;
    }
    if (len == sizeof(buf)) len--;
    buf[len] = 0;
    if (strstr(buf, "closed") != 0) return false;
    if (strstr(buf, "opened") != 0) return true;
    return true;
}

void sleep_now()
{
    system("pm-suspend");
}

int main()
{
    openlog ("unwake", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    syslog (LOG_INFO, "Unwake started");

    unsigned int closedLidDuration = 0;
    time_t oldTime = time(0);

    for(;;)
    {
        sleep(1);
        time_t newTime = time(0);
        if ((newTime - oldTime) > 2)
        {
            // Wakeup
            closedLidDuration = 0;
            syslog (LOG_INFO, "Wakeup detected");
        }
        else
        {
            if (lid_opened())
            {
                closedLidDuration = 0;
            }
            else
            {
                closedLidDuration++;
            }
        }
        oldTime = newTime;

        if (closedLidDuration > 5)
        {
            syslog (LOG_WARNING, "Closed lid detected");
            sleep_now();
        }
    }

    return 0;
}

