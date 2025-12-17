#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

pthread_mutex_t log_lock;       // TO PROTECT SHARED DATA

typedef struct {
    float cpu;                  // FIRST THREE WILL TAKE USAGE FROM SYSTEM
    float memory;
    float disk;
    int cpuAlert;               //NOW OTHER THREE WILL HELP TO SHOW ALERT
    int memAlert;
    int diskAlert;
} ResourceData;

ResourceData data = {0.0, 0.0, 0.0, 0, 0, 0};

// -------------------- System Info --------------------
double getCPUUsage() {
    static FILETIME prevIdle, prevKernel, prevUser;  //STORES PREVIOUS CPU % INCLUDING KERNEL, USER SPACE
    FILETIME idleTime, kernelTime, userTime;

    GetSystemTimes(&idleTime, &kernelTime, &userTime);  //NOW GIVES CPU, KERNEL AND USER SPACE USAGE


        //FINDING DIFFERENCE BETWEEN PREVIOUS USAGE AND CURRENT(NEW) USAGE

    ULONGLONG idleDiff = ((ULONGLONG)idleTime.dwLowDateTime | ((ULONGLONG)idleTime.dwHighDateTime << 32)) -
                         ((ULONGLONG)prevIdle.dwLowDateTime | ((ULONGLONG)prevIdle.dwHighDateTime << 32));
    ULONGLONG kernelDiff = ((ULONGLONG)kernelTime.dwLowDateTime | ((ULONGLONG)kernelTime.dwHighDateTime << 32)) -
                           ((ULONGLONG)prevKernel.dwLowDateTime | ((ULONGLONG)prevKernel.dwHighDateTime << 32));
    ULONGLONG userDiff = ((ULONGLONG)userTime.dwLowDateTime | ((ULONGLONG)userTime.dwHighDateTime << 32)) -
                         ((ULONGLONG)prevUser.dwLowDateTime | ((ULONGLONG)prevUser.dwHighDateTime << 32));


            //STORING CURRENT VALUES.

    prevIdle = idleTime;
    prevKernel = kernelTime;
    prevUser = userTime;
                                                //ULONGLONG (Unsigned Long Long)used to convert usage in human understandable format.
    ULONGLONG total = kernelDiff + userDiff;   //SAVING TOTAL USAGE OF CPU BY ADDING KERNEL SPACE AND USER SPACE
    if (total == 0) return 0.0;                //CHECHING THAT USAGE IS 0 OR NOT.

    return (double)(total - idleDiff) * 100.0 / total;  //STANDARD FORMULA TO CALCULATE PERCENTAGE:
                                                        //(BUSY TIME OF CPU / TOTAL TIME) * 100
}


            //GETTING MEMORY USAGE:
            // DIDN'T USE ANY FORMULA. JUST GETTING THE MEMORY USAGE FROM THE ORIGINAL SIZE OF MEMORY IN PERCENTAGE

double getMemoryUsage() {
    MEMORYSTATUSEX mem;
    mem.dwLength = sizeof(mem);
    GlobalMemoryStatusEx(&mem);
    return (double)mem.dwMemoryLoad;
}

        //GETTING DISK USAGE(C DRIVE)

double getDiskUsage() {

                //GETING TOTAL AND FREE SPACE OF C DRIVE
    ULARGE_INTEGER freeBytes, totalBytes, totalFree;
    GetDiskFreeSpaceEx("C:\\", &freeBytes, &totalBytes, &totalFree);

                //CONVERTING THE POINTS VALUE IN PERCENTAGE BY: (USED/TOTAL)*100
    return (double)(totalBytes.QuadPart - freeBytes.QuadPart) * 100.0 / totalBytes.QuadPart;
}

// -------------------- Pop-up Alert --------------------

            //SHOWING HOW THE ALERT WILL SHOW
void showAlert(const char* message) {
    // Non-blocking alert, always on top

            //TAKING THE DEFAULT SOUND OF WINDOWS ALERT
    MessageBeep(MB_ICONEXCLAMATION); 

            //SHOWING THE ORIGINAL BOX OF ALERT AND WILL SHOW ON THE TOP OF ALL WINDOWS.
    MessageBoxA(NULL, message, "System Health Alert", MB_OK | MB_ICONWARNING | MB_TOPMOST);
}


// -------------------- Monitoring Thread --------------------

   // FUNCTION OF BACKGROUND THREAD THAT MONITOR/CALCULATE SYSTEM HEALTH
void* monitorResources(void* arg) {
    char buffer[256];      //USED TO SAVE THE MEMORY IN BUFFER OF EVERY USAGE

            //LOOP TO CALCULATE AGAIN AND AGAIN:
    while (1) {
        pthread_mutex_lock(&log_lock);    //Lock shared data and logfile so NO INTERACTION BETWEEN THEM



                //THROUGH THREE FUNCTIONS MENTIONED ABOVE SAVING THE CURRENT USAGES.
        data.cpu = (float)getCPUUsage();
        data.memory = (float)getMemoryUsage();
        data.disk = (float)getDiskUsage();


            //FORMATTING THE CONSOLE WINDOWS: %.2F%% MEANS ONLY TWO VALUES AFTER POINT.
        sprintf(buffer, "CPU: %.2f%% | MEM: %.2f%% | DISK: %.2f%%\n",
                data.cpu, data.memory, data.disk);
        printf("%s", buffer);


        //OPENING THE LOG FILE.
        FILE* log = fopen("system_log.txt", "a");

        //WRITING THE HISTORY OF USAGE IN LOG FILE THEN CLOSE IT.
        if (log) {
            fprintf(log, "%s", buffer);
            fclose(log);
        }

        // Alerts logic: only trigger if value exceeds threshold and not already alerted

        //IF CPU USAGE GOES UP TO 50% SHOW THE ALERT 
        if (data.cpu > 50 && !data.cpuAlert) {
            showAlert("High CPU Usage Detected!");
            data.cpuAlert = 1;

            //IF LESS THAN 50 THEN DON'T SHOW IT.
        } else if (data.cpu <= 50) data.cpuAlert = 0;


            //IF MEMORY USAGE GOES UP TO 50% SHOW THE ALERT 
        if (data.memory > 50 && !data.memAlert) {
            showAlert("High Memory Usage Detected!");
            data.memAlert = 1;

                    //IF LESS THAN 50 THEN DON'T SHOW IT.
        } else if (data.memory <= 50) data.memAlert = 0;


                    //IF DISK USAGE GOES UP TO 70% SHOW THE ALERT 
        if (data.disk > 70 && !data.diskAlert) {
            showAlert("Low Disk Space Detected!");
            data.diskAlert = 1;

                                //IF LESS THAN 70 THEN DON'T SHOW IT
        } else if (data.disk <= 70) data.diskAlert = 0;

        pthread_mutex_unlock(&log_lock);
        Sleep(15000); // check every 15 seconds to avoid COLLISION BETWEEN ALERTS
    }
    return NULL;
}

// -------------------- Main --------------------
int main() {
    pthread_t monitorThread;  //CREATING THREAD VAIABLE
    pthread_mutex_init(&log_lock, NULL);  // INITIALIZING THE MUTEX THREAD


    //FORMATTING THE CONSOLE WINDOW
    printf("=== Multithreaded System Health Monitor ===\n");
    printf("Monitoring started. Close via Task Manager when needed.\n");



    //STARTING THE MONITOR THREAD VARIABLE
    pthread_create(&monitorThread, NULL, monitorResources, NULL);


    //IF USER DON'T STOP IT WILL RUN FOREVER
    pthread_join(monitorThread, NULL);


    //TO END THE THREAD.
    pthread_mutex_destroy(&log_lock);
    return 0;
}
