#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define MAX_PROCESS 100

typedef struct {
    int pid;
    int arrival;
    int burst;
    int remaining;
    int priority;
    int start;
    int finish;
    int waiting;
    int turnaround;
    int completed;
} Process;

static void copy_processes(Process dst[], const Process src[], int n) {
    for (int i = 0; i < n; i++) {
        dst[i] = src[i];
        dst[i].remaining = src[i].burst;
        dst[i].start = -1;
        dst[i].finish = 0;
        dst[i].waiting = 0;
        dst[i].turnaround = 0;
        dst[i].completed = 0;
    }
}

static void print_input(const Process p[], int n) {
    printf("\n进程参数：\n");
    printf("PID\t到达时间\t运行时间\t优先级\n");
    for (int i = 0; i < n; i++) {
        printf("P%d\t%d\t\t%d\t\t%d\n", p[i].pid, p[i].arrival, p[i].burst, p[i].priority);
    }
}

static void print_result(const Process p[], int n, const char *name, const char *sequence) {
    double total_turnaround = 0.0;
    double total_waiting = 0.0;

    printf("\n========== %s ==========\n", name);
    printf("运行顺序：%s\n", sequence);
    printf("PID\t到达\t运行\t完成\t周转\t等待\t优先级\n");
    for (int i = 0; i < n; i++) {
        total_turnaround += p[i].turnaround;
        total_waiting += p[i].waiting;
        printf("P%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
               p[i].pid, p[i].arrival, p[i].burst, p[i].finish,
               p[i].turnaround, p[i].waiting, p[i].priority);
    }
    printf("平均周转时间：%.2f\n", total_turnaround / n);
    printf("平均等待时间：%.2f\n", total_waiting / n);
}

static void append_sequence(char *sequence, size_t size, int pid, int start, int end) {
    char item[64];
    snprintf(item, sizeof(item), "[%d-%d:P%d] ", start, end, pid);
    strncat(sequence, item, size - strlen(sequence) - 1);
}

static int all_done(const Process p[], int n) {
    for (int i = 0; i < n; i++) {
        if (!p[i].completed) {
            return 0;
        }
    }
    return 1;
}

static int earliest_arrival(const Process p[], int n) {
    int t = INT_MAX;
    for (int i = 0; i < n; i++) {
        if (!p[i].completed && p[i].arrival < t) {
            t = p[i].arrival;
        }
    }
    return t;
}

static void finish_process(Process *p, int finish_time) {
    p->finish = finish_time;
    p->turnaround = p->finish - p->arrival;
    p->waiting = p->turnaround - p->burst;
    p->completed = 1;
}

static void fcfs(const Process origin[], int n) {
    Process p[MAX_PROCESS];
    char sequence[4096] = "";
    int time = 0;
    int completed = 0;

    copy_processes(p, origin, n);
    while (completed < n) {
        int selected = -1;
        for (int i = 0; i < n; i++) {
            if (!p[i].completed && p[i].arrival <= time) {
                if (selected == -1 ||
                    p[i].arrival < p[selected].arrival ||
                    (p[i].arrival == p[selected].arrival && p[i].pid < p[selected].pid)) {
                    selected = i;
                }
            }
        }

        if (selected == -1) {
            time = earliest_arrival(p, n);
            continue;
        }

        p[selected].start = time;
        append_sequence(sequence, sizeof(sequence), p[selected].pid, time, time + p[selected].burst);
        time += p[selected].burst;
        finish_process(&p[selected], time);
        completed++;
    }

    print_result(p, n, "先来先服务 FCFS", sequence);
}

static void sjf(const Process origin[], int n) {
    Process p[MAX_PROCESS];
    char sequence[4096] = "";
    int time = 0;
    int completed = 0;

    copy_processes(p, origin, n);
    while (completed < n) {
        int selected = -1;
        for (int i = 0; i < n; i++) {
            if (!p[i].completed && p[i].arrival <= time) {
                if (selected == -1 ||
                    p[i].burst < p[selected].burst ||
                    (p[i].burst == p[selected].burst && p[i].arrival < p[selected].arrival)) {
                    selected = i;
                }
            }
        }

        if (selected == -1) {
            time = earliest_arrival(p, n);
            continue;
        }

        p[selected].start = time;
        append_sequence(sequence, sizeof(sequence), p[selected].pid, time, time + p[selected].burst);
        time += p[selected].burst;
        finish_process(&p[selected], time);
        completed++;
    }

    print_result(p, n, "短作业优先 SJF（非抢占）", sequence);
}

static void priority_schedule(const Process origin[], int n) {
    Process p[MAX_PROCESS];
    char sequence[4096] = "";
    int time = 0;
    int completed = 0;

    copy_processes(p, origin, n);
    while (completed < n) {
        int selected = -1;
        for (int i = 0; i < n; i++) {
            if (!p[i].completed && p[i].arrival <= time) {
                if (selected == -1 ||
                    p[i].priority < p[selected].priority ||
                    (p[i].priority == p[selected].priority && p[i].arrival < p[selected].arrival)) {
                    selected = i;
                }
            }
        }

        if (selected == -1) {
            time = earliest_arrival(p, n);
            continue;
        }

        p[selected].start = time;
        append_sequence(sequence, sizeof(sequence), p[selected].pid, time, time + p[selected].burst);
        time += p[selected].burst;
        finish_process(&p[selected], time);
        completed++;
    }

    print_result(p, n, "优先级调度（非抢占，数字越小优先级越高）", sequence);
}

static void rr(const Process origin[], int n, int quantum) {
    Process p[MAX_PROCESS];
    char sequence[8192] = "";
    int queue[MAX_PROCESS * 100];
    int in_queue[MAX_PROCESS] = {0};
    int front = 0, rear = 0;
    int time = 0;
    int completed = 0;

    copy_processes(p, origin, n);

    while (!all_done(p, n)) {
        for (int i = 0; i < n; i++) {
            if (!p[i].completed && !in_queue[i] && p[i].arrival <= time) {
                queue[rear++] = i;
                in_queue[i] = 1;
            }
        }

        if (front == rear) {
            time = earliest_arrival(p, n);
            continue;
        }

        int idx = queue[front++];
        int run = p[idx].remaining < quantum ? p[idx].remaining : quantum;
        if (p[idx].start == -1) {
            p[idx].start = time;
        }
        append_sequence(sequence, sizeof(sequence), p[idx].pid, time, time + run);
        time += run;
        p[idx].remaining -= run;

        for (int i = 0; i < n; i++) {
            if (!p[i].completed && !in_queue[i] && p[i].arrival <= time) {
                queue[rear++] = i;
                in_queue[i] = 1;
            }
        }

        if (p[idx].remaining > 0) {
            queue[rear++] = idx;
        } else {
            finish_process(&p[idx], time);
            completed++;
        }
    }

    (void)completed;
    print_result(p, n, "时间片轮转 RR", sequence);
}

static void run_selected(int choice, const Process processes[], int n, int quantum) {
    switch (choice) {
        case 1:
            fcfs(processes, n);
            break;
        case 2:
            sjf(processes, n);
            break;
        case 3:
            rr(processes, n, quantum);
            break;
        case 4:
            priority_schedule(processes, n);
            break;
        case 5:
            fcfs(processes, n);
            sjf(processes, n);
            rr(processes, n, quantum);
            priority_schedule(processes, n);
            break;
        default:
            printf("无效选择。\n");
    }
}

int main(void) {
    Process processes[MAX_PROCESS];
    int n, choice, quantum;

    printf("处理机调度算法模拟\n");
    printf("请输入进程数量（1-%d）：", MAX_PROCESS);
    if (scanf("%d", &n) != 1 || n <= 0 || n > MAX_PROCESS) {
        printf("进程数量非法。\n");
        return 1;
    }

    for (int i = 0; i < n; i++) {
        processes[i].pid = i + 1;
        printf("请输入 P%d 的 到达时间 运行时间 优先级：", i + 1);
        if (scanf("%d%d%d", &processes[i].arrival, &processes[i].burst, &processes[i].priority) != 3 ||
            processes[i].arrival < 0 || processes[i].burst <= 0) {
            printf("进程参数非法。\n");
            return 1;
        }
        processes[i].remaining = processes[i].burst;
    }

    printf("请输入时间片大小（RR 使用，建议 2 或 3）：");
    if (scanf("%d", &quantum) != 1 || quantum <= 0) {
        printf("时间片非法。\n");
        return 1;
    }

    print_input(processes, n);
    printf("\n请选择算法：\n");
    printf("1. FCFS\n2. SJF\n3. RR\n4. 优先级调度\n5. 全部运行并比较\n");
    printf("选择：");
    if (scanf("%d", &choice) != 1) {
        printf("选择非法。\n");
        return 1;
    }

    run_selected(choice, processes, n, quantum);
    return 0;
}
