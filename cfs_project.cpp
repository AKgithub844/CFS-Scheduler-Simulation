// cfs_scheduler.cpp
#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <stdexcept>

// --------------------------------------------------------------------------------
// Data Structures
// --------------------------------------------------------------------------------

enum PROCESS_NATURE { CPU_BOUND, IO_BOUND };

struct ProcessState {
    long long counter;
};

struct Process {
    int pid;
    long long vruntime;
    int cpu_burst_time;
    int priority;
    ProcessState processState;
    PROCESS_NATURE processNature;
    Process(int p=0, long long vr=0, int cbt=0, int pr=0, PROCESS_NATURE pn=CPU_BOUND)
      : pid(p), vruntime(vr), cpu_burst_time(cbt), priority(pr), processNature(pn) {
        processState.counter = 0;
    }
};

struct ProcessLog {
    int pid;
    long long startTime;
    long long endTime;
    ProcessLog(int p=0, long long st=0, long long et=0)
      : pid(p), startTime(st), endTime(et) {}
};

// --------------------------------------------------------------------------------
// QueueService
// --------------------------------------------------------------------------------

class QueueService {
private:
    struct Compare {
        bool operator()(Process* a, Process* b) {
            return a->vruntime > b->vruntime;
        }
    };
    std::priority_queue<Process*, std::vector<Process*>, Compare> q;

public:
    void push_element(Process* p) {
        if (p) q.push(p);
    }
    void pop_element() {
        if (!q.empty()) q.pop();
    }
    bool is_empty() const {
        return q.empty();
    }
    Process* top_element() const {
        return q.empty() ? nullptr : q.top();
    }
};

// --------------------------------------------------------------------------------
// CFSScheduler
// --------------------------------------------------------------------------------

class CFSScheduler {
private:
    std::vector<ProcessLog*> logs;
    static constexpr int NICE_0_LOAD    = 1024;
    static constexpr int CPU_TIME_SLICE = 1;   // ms
    static constexpr int IO_WAIT_TIME  = 10;  // ms

    double weightFunction(int priority) {
        return static_cast<double>(NICE_0_LOAD) / (priority + 1);
    }

    long long getCurrentTimeNs() {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
                   now.time_since_epoch()).count();
    }

    void createProcessLog(long long st, long long et, int pid) {
        logs.push_back(new ProcessLog(pid, st, et));
    }

    void executeCpuBoundProcess(Process* proc, int timeSlice, QueueService& q) {
        if (!proc) return;
        int exec = std::min(timeSlice, proc->cpu_burst_time);
        proc->cpu_burst_time -= exec;
        double w = weightFunction(proc->priority);
        proc->vruntime += static_cast<long long>((exec * NICE_0_LOAD) / w);
        std::this_thread::sleep_for(std::chrono::milliseconds(exec));
        if (proc->cpu_burst_time > 0) q.push_element(proc);
    }

    void handleIoBoundProcess(Process* proc, int ioWait, QueueService& q) {
        if (!proc) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(ioWait));
        double w = weightFunction(proc->priority);
        proc->vruntime += static_cast<long long>((ioWait * NICE_0_LOAD) / w);
        int exec = 1;
        proc->cpu_burst_time -= exec;
        proc->vruntime += static_cast<long long>((exec * NICE_0_LOAD) / w);
        if (proc->cpu_burst_time > 0) q.push_element(proc);
    }

public:
    CFSScheduler() = default;
    ~CFSScheduler() {
        for (auto log : logs) delete log;
    }

    std::vector<ProcessLog*> schedule(std::vector<Process*> processList) {
        QueueService queue;
        for (auto p : processList) if (p) queue.push_element(p);

        while (!queue.is_empty()) {
            Process* cur = queue.top_element();
            queue.pop_element();
            if (!cur) continue;

            long long st = getCurrentTimeNs();
            if (cur->processNature == CPU_BOUND)
                executeCpuBoundProcess(cur, CPU_TIME_SLICE, queue);
            else
                handleIoBoundProcess(cur, IO_WAIT_TIME, queue);
            long long et = getCurrentTimeNs();
            createProcessLog(st, et, cur->pid);
        }
        return logs;
    }

    static void displayProcessInfo(const std::vector<Process*>& procs) {
        std::cout << std::setw(5) << "PID"
                  << std::setw(10) << "Prio"
                  << std::setw(12) << "Burst"
                  << std::setw(12) << "VRun"
                  << std::setw(12) << "Type\n";
        std::cout << std::string(50,'-') << "\n";
        for (auto p : procs) {
            if (!p) continue;
            std::cout << std::setw(5) << p->pid<<" "
                      << std::setw(10) << p->priority<<" "
                      << std::setw(12) << p->cpu_burst_time<<" "
                      << std::setw(12) << p->vruntime<<" "
                      << std::setw(12)<<" "
                      << (p->processNature==CPU_BOUND?"CPU":"IO")
                      << "\n";
        }
    }

    void displayLogs() const {
        std::cout << "\nPID  Start(ns)          End(ns)            Duration(ns)\n";
        std::cout << std::string(60,'-') << "\n";
        for (auto log : logs) {
            long long d = log->endTime - log->startTime;
            std::cout << std::setw(5) << log->pid<<" "
                      << std::setw(18) << log->startTime<<" "
                      << std::setw(18) << log->endTime<<" "
                      << std::setw(15) << d << "\n";
        }
    }
};

// --------------------------------------------------------------------------------
// Helpers & Main
// --------------------------------------------------------------------------------

static std::vector<Process*> createSampleProcesses() {
    return {
        new Process(1,0,15,0,CPU_BOUND),
        new Process(2,0,20,5,IO_BOUND),
        new Process(3,0,10,2,CPU_BOUND),
        new Process(4,0,25,1,IO_BOUND),
        new Process(5,0,12,3,CPU_BOUND)
    };
}

static void cleanupProcesses(std::vector<Process*>& v) {
    for (auto p : v) delete p;
    v.clear();
}

int main() {
    std::cout << "=== CFS Scheduler Demo ===\n\n";

    auto processes = createSampleProcesses();
    CFSScheduler::displayProcessInfo(processes);

    CFSScheduler scheduler;
    auto logs = scheduler.schedule(processes);

    scheduler.displayLogs();

    std::cout << "\n=== Summary ===\n"
              << "Processes scheduled: " << processes.size() << "\n"
              << "Execution slices  : " << logs.size()    << "\n";

    cleanupProcesses(processes);
    for (auto lg : logs) delete lg;
    return 0;
}
