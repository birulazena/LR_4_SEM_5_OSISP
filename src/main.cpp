#include <windows.h>
#include <iostream>
#include <vector>
#include <string>

int num_philosophers = 0;
int num_cycles = 0;
int think_time_ms = 0;
int eat_time_ms = 0;

DWORD start_time = 0;
HANDLE log_mutex;
std::vector<HANDLE> forks;

std::vector<int> fork_owners;
int invariant_violations = 0;

void LogEvent(int id, const std::string& state) {
    WaitForSingleObject(log_mutex, INFINITE);
    DWORD current_time = GetTickCount() - start_time;
    printf("[%06lu ms] WORKER %d | TID=%lu | STATE=%s\n", current_time, id, GetCurrentThreadId(), state.c_str());
    ReleaseMutex(log_mutex);
}

DWORD WINAPI Philosopher(LPVOID param) {
    int id = (int)(uintptr_t)param;
    
    int left_fork = id;
    int right_fork = (id + 1) % num_philosophers;
    
    int first_fork = (id % 2 == 0) ? left_fork : right_fork;
    int second_fork = (id % 2 == 0) ? right_fork : left_fork;
    
    for (int i = 0; i < num_cycles; ++i) {
        LogEvent(id, "THINKING");
        Sleep(think_time_ms);
        
        LogEvent(id, "WAITING");
        WaitForSingleObject(forks[first_fork], INFINITE);
        WaitForSingleObject(forks[second_fork], INFINITE);
        
        if (fork_owners[first_fork] != -1 || fork_owners[second_fork] != -1) {
            InterlockedIncrement((LONG*)&invariant_violations);
        }
        fork_owners[first_fork] = id;
        fork_owners[second_fork] = id;
        
        LogEvent(id, "EATING");
        Sleep(eat_time_ms);
        
        fork_owners[first_fork] = -1;
        fork_owners[second_fork] = -1;
        
        ReleaseMutex(forks[second_fork]);
        ReleaseMutex(forks[first_fork]);
    }
    
    LogEvent(id, "FINISHED");
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        printf("Usage: main.exe <philosophers> <cycles> <think_time_ms> <eat_time_ms>\n");
        return 1;
    }
    num_philosophers = atoi(argv[1]);
    num_cycles = atoi(argv[2]);
    think_time_ms = atoi(argv[3]);
    eat_time_ms = atoi(argv[4]);
    
    start_time = GetTickCount();
    
    log_mutex = CreateMutex(NULL, FALSE, NULL);
    
    forks.resize(num_philosophers);
    fork_owners.resize(num_philosophers, -1);
    
    for (int i = 0; i < num_philosophers; ++i) {
        forks[i] = CreateMutex(NULL, FALSE, NULL);
    }
    
    std::vector<HANDLE> threads(num_philosophers);
    for (int i = 0; i < num_philosophers; ++i) {
        threads[i] = CreateThread(NULL, 0, Philosopher, (LPVOID)(uintptr_t)i, 0, NULL);
    }
    
    WaitForMultipleObjects(num_philosophers, threads.data(), TRUE, INFINITE);
    
    printf("\nSimulation finished.\n");
    printf("Invariant violations: %d\n", invariant_violations);
    
    for (int i = 0; i < num_philosophers; ++i) {
        CloseHandle(threads[i]);
        CloseHandle(forks[i]);
    }
    CloseHandle(log_mutex);
    
    return 0;
}