#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cstdlib>

int num_philosophers = 0;
int num_cycles = 0;
int think_time_ms = 0;
int eat_time_ms = 0;
int max_wait_ms = 0; 
bool quiet_mode = false;

DWORD start_time = 0;
HANDLE log_mutex;
std::vector<HANDLE> forks;
std::vector<int> fork_owners;

long invariant_violations = 0;
long total_success = 0;
long total_timeouts = 0;
std::vector<DWORD> wait_times;

void LogEvent(int id, const std::string& state) {
    if (quiet_mode) return;
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
        int jitter = rand() % 5;
        LogEvent(id, "THINKING");
        Sleep(think_time_ms + jitter);
        
        std::string forks_str = std::to_string(first_fork) + "," + std::to_string(second_fork);
        LogEvent(id, "WAITING FORKS " + forks_str);
        DWORD wait_start = GetTickCount();
        
        DWORD res1 = WaitForSingleObject(forks[first_fork], max_wait_ms);
        if (res1 == WAIT_TIMEOUT) {
            LogEvent(id, "TIMEOUT WAITING FORK " + std::to_string(first_fork));
            InterlockedIncrement(&total_timeouts);
            continue; 
        } else if (res1 == WAIT_FAILED) {
            break; 
        }
        
        DWORD res2 = WaitForSingleObject(forks[second_fork], max_wait_ms);
        if (res2 == WAIT_TIMEOUT) {
            LogEvent(id, "TIMEOUT WAITING FORK " + std::to_string(second_fork));
            ReleaseMutex(forks[first_fork]); // ОСВОБОЖДАЕМ первую вилку, чтобы не было дедлока!
            InterlockedIncrement(&total_timeouts);
            continue;
        } else if (res2 == WAIT_FAILED) {
            ReleaseMutex(forks[first_fork]);
            break;
        }
        
        wait_times[id] += (GetTickCount() - wait_start);
        
        if (fork_owners[first_fork] != -1 || fork_owners[second_fork] != -1) {
            InterlockedIncrement(&invariant_violations);
        }
        fork_owners[first_fork] = id;
        fork_owners[second_fork] = id;
        
        LogEvent(id, "ACQUIRED FORKS " + forks_str + " (EATING)");
        Sleep(eat_time_ms);
        InterlockedIncrement(&total_success);
        
        fork_owners[first_fork] = -1;
        fork_owners[second_fork] = -1;
        
        LogEvent(id, "RELEASED FORKS " + forks_str);
        ReleaseMutex(forks[second_fork]);
        ReleaseMutex(forks[first_fork]);
    }
    LogEvent(id, "FINISHED");
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 6) {
        printf("Usage: main.exe <philosophers> <cycles> <think_time> <eat_time> <max_wait_ms> [quiet_mode: 0 or 1]\n");
        return 1;
    }
    num_philosophers = atoi(argv[1]);
    num_cycles = atoi(argv[2]);
    think_time_ms = atoi(argv[3]);
    eat_time_ms = atoi(argv[4]);
    max_wait_ms = atoi(argv[5]);
    if (argc >= 7) quiet_mode = (atoi(argv[6]) != 0);
    
    srand(2552);
    
    forks.resize(num_philosophers);
    fork_owners.resize(num_philosophers, -1);
    wait_times.resize(num_philosophers, 0);
    
    log_mutex = CreateMutex(NULL, FALSE, NULL);
    if (log_mutex == NULL) {
        printf("Error creating log mutex\n");
        return 1;
    }
    
    for (int i = 0; i < num_philosophers; ++i) {
        forks[i] = CreateMutex(NULL, FALSE, NULL);
        if (forks[i] == NULL) return 1;
    }
    
    start_time = GetTickCount();
    std::vector<HANDLE> threads(num_philosophers);
    
    for (int i = 0; i < num_philosophers; ++i) {
        threads[i] = CreateThread(NULL, 0, Philosopher, (LPVOID)(uintptr_t)i, 0, NULL);
        if (threads[i] == NULL) return 1;
    }
    
    DWORD wait_res = WaitForMultipleObjects(num_philosophers, threads.data(), TRUE, 60000);
    if (wait_res == WAIT_TIMEOUT) {
        printf("\nGLOBAL TIMEOUT: Experiment took too long and was aborted.\n");
    }
    
    DWORD total_time = GetTickCount() - start_time;
    DWORD total_wait = 0;
    for (DWORD w : wait_times) total_wait += w;
    DWORD avg_wait = num_philosophers > 0 ? total_wait / num_philosophers : 0;
    
    printf("\nSimulation finished. Violations: %ld\n", invariant_violations);
    printf("Total execution time: %lu ms\n", total_time);
    printf("Average wait time: %lu ms\n", avg_wait);
    printf("Successful meals: %ld, Timeouts: %ld\n", total_success, total_timeouts);
    
    std::ofstream csv_file("data/results.csv", std::ios::app);
    if (csv_file.is_open()) {
        csv_file << num_philosophers << "," << num_cycles << "," 
                 << think_time_ms << "," << eat_time_ms << "," 
                 << total_time << "," << avg_wait << "," 
                 << total_success << "," << total_timeouts << "\n";
        csv_file.close();
    }
    
    for (int i = 0; i < num_philosophers; ++i) {
        if (threads[i]) CloseHandle(threads[i]);
        if (forks[i]) CloseHandle(forks[i]);
    }
    CloseHandle(log_mutex);
    return 0;
}