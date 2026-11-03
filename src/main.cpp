#include <windows.h>
#include <iostream>
#include <vector>
#include <string>

int num_philosophers = 0;
int num_cycles = 0;
int think_time_ms = 0;

DWORD start_time = 0;
HANDLE log_mutex;

void LogEvent(int id, const std::string& state) {
    WaitForSingleObject(log_mutex, INFINITE);
    DWORD current_time = GetTickCount() - start_time;
    printf("[%06lu ms] WORKER %d | TID=%lu | STATE=%s\n", current_time, id, GetCurrentThreadId(), state.c_str());
    ReleaseMutex(log_mutex);
}

DWORD WINAPI Philosopher(LPVOID param) {
    int id = (int)(uintptr_t)param;
    
    for (int i = 0; i < num_cycles; ++i) {
        LogEvent(id, "THINKING");
        Sleep(think_time_ms); 
    }
    
    LogEvent(id, "FINISHED");
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Usage: main.exe <philosophers> <cycles> <think_time_ms>\n");
        return 1;
    }
    num_philosophers = atoi(argv[1]);
    num_cycles = atoi(argv[2]);
    think_time_ms = atoi(argv[3]);
    
    start_time = GetTickCount();
    
    log_mutex = CreateMutex(NULL, FALSE, NULL);
    if (log_mutex == NULL) {
        printf("Error creating log mutex\n");
        return 1;
    }
    
    std::vector<HANDLE> threads(num_philosophers);
    for (int i = 0; i < num_philosophers; ++i) {
        threads[i] = CreateThread(NULL, 0, Philosopher, (LPVOID)(uintptr_t)i, 0, NULL);
        if (threads[i] == NULL) {
            printf("Error creating thread %d\n", i);
            return 1;
        }
    }
    
    WaitForMultipleObjects(num_philosophers, threads.data(), TRUE, INFINITE);
    
    for (int i = 0; i < num_philosophers; ++i) {
        CloseHandle(threads[i]);
    }
    CloseHandle(log_mutex);
    
    return 0;
}