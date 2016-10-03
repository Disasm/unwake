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

bool screen_locked()
{
    int rc = system("DISPLAY=:0.0 dbus-send --print-reply --dest=org.mate.ScreenSaver / org.mate.ScreenSaver.GetActive | grep boolean | grep true &> /dev/null");
    return rc == 0;
}

void sleep_now()
{
    //system("pm-suspend");
    system("systemctl suspend -i");
}

int main()
{
    openlog ("unwake", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    syslog (LOG_INFO, "Unwake started");

    unsigned int lockedScreenDuration = 0;
    bool enableMonitoring = false;
    time_t oldTime = time(0);

    for(;;)
    {
        sleep(1);
        time_t newTime = time(0);
        if ((newTime - oldTime) > 3)
        {
            // Wakeup
            enableMonitoring = true;
            lockedScreenDuration = 0;
            syslog (LOG_INFO, "Wakeup detected");
        }
        else if (enableMonitoring)
        {
            if (screen_locked())
            {
                lockedScreenDuration++;
            }
            else
            {
                lockedScreenDuration = 0;
                enableMonitoring = false;
            }
        }
        oldTime = newTime;

        if ((lockedScreenDuration > 10) && enableMonitoring)
        {
            syslog (LOG_WARNING, "Locked screen detected");
            sleep_now();
        }
    }

    return 0;
}

