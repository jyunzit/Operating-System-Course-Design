#include <pthread.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define MAX_PROCESS 100
#define MAX_TASK 20
#define MAX_TIME 300
#define MAX_THREADS 16

typedef struct {
    int pid;
    int arrival;
    int burst;
    int remaining;
    int priority;
    int deadline;
    int start;
    int finish;
    int waiting;
    int turnaround;
    int response;
    int completed;
} Process;

typedef struct {
    int tid;
    int period;
    int exec_time;
    int relative_deadline;
    int next_release;
    int remaining;
    int absolute_deadline;
    int jobs;
    int misses;
} Task;

typedef struct {
    int64_t begin;
    int64_t end;
    int64_t local_sum;
} ThreadArg;

static pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;
static int64_t global_sum = 0;

static int read_int(const char *prompt, int *value) {
    printf("%s", prompt);
    if (scanf("%d", value) != 1) {
        printf("输入错误：需要输入整数。\n");
        return 0;
    }
    return 1;
}

static int read_positive_int(const char *prompt, int *value, int max_value) {
    if (!read_int(prompt, value)) {
        return 0;
    }
    if (*value <= 0 || *value > max_value) {
        printf("输入错误：数值范围应为 1 到 %d。\n", max_value);
        return 0;
    }
    return 1;
}

static double now_seconds(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

static void copy_processes(Process dst[], const Process src[], int n) {
    for (int i = 0; i < n; i++) {
        dst[i] = src[i];
        dst[i].remaining = src[i].burst;
        dst[i].start = -1;
        dst[i].finish = 0;
        dst[i].waiting = 0;
        dst[i].turnaround = 0;
        dst[i].response = 0;
        dst[i].completed = 0;
    }
}

static int earliest_unfinished_arrival(const Process p[], int n) {
    int earliest = 1000000000;
    for (int i = 0; i < n; i++) {
        if (!p[i].completed && p[i].arrival < earliest) {
            earliest = p[i].arrival;
        }
    }
    return earliest == 1000000000 ? 0 : earliest;
}

static void append_sequence(char *sequence, size_t size, int pid, int start, int end) {
    char temp[64];
    snprintf(temp, sizeof(temp), "[%d-%d:P%d] ", start, end, pid);
    strncat(sequence, temp, size - strlen(sequence) - 1);
}

static void finish_process(Process *p, int finish_time) {
    p->finish = finish_time;
    p->turnaround = p->finish - p->arrival;
    p->waiting = p->turnaround - p->burst;
    p->response = p->start - p->arrival;
    p->completed = 1;
}

static void print_schedule_result(const char *name, Process p[], int n, const char *sequence, double cpu_time) {
    double total_waiting = 0.0;
    double total_turnaround = 0.0;
    double total_response = 0.0;
    int first_arrival = p[0].arrival;
    int last_finish = p[0].finish;
    int miss_count = 0;

    for (int i = 0; i < n; i++) {
        if (p[i].arrival < first_arrival) {
            first_arrival = p[i].arrival;
        }
        if (p[i].finish > last_finish) {
            last_finish = p[i].finish;
        }
    }

    printf("\n========== %s ==========\n", name);
    printf("运行顺序：%s\n", sequence);
    printf("PID\t到达\t运行\t优先级\t截止期\t开始\t完成\t等待\t周转\t响应\t是否超期\n");
    for (int i = 0; i < n; i++) {
        int missed = p[i].finish > p[i].deadline;
        miss_count += missed;
        total_waiting += p[i].waiting;
        total_turnaround += p[i].turnaround;
        total_response += p[i].response;
        printf("P%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%s\n",
               p[i].pid, p[i].arrival, p[i].burst, p[i].priority, p[i].deadline,
               p[i].start, p[i].finish, p[i].waiting, p[i].turnaround, p[i].response,
               missed ? "是" : "否");
    }

    printf("平均等待时间：%.2f\n", total_waiting / n);
    printf("平均周转时间：%.2f\n", total_turnaround / n);
    printf("平均响应时间：%.2f\n", total_response / n);
    printf("吞吐率：%.2f 个进程/时间单位\n", n * 1.0 / (last_finish - first_arrival));
    printf("截止期违约数：%d\n", miss_count);
    printf("算法模拟耗时：%.6f 秒\n", cpu_time);
}

static void fcfs(Process input[], int n) {
    Process p[MAX_PROCESS];
    char sequence[8192] = "";
    int time = 0;
    int done = 0;
    double begin = now_seconds();

    copy_processes(p, input, n);
    while (done < n) {
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
            time = earliest_unfinished_arrival(p, n);
            continue;
        }

        p[selected].start = time;
        append_sequence(sequence, sizeof(sequence), p[selected].pid, time, time + p[selected].burst);
        time += p[selected].burst;
        finish_process(&p[selected], time);
        done++;
    }

    print_schedule_result("基础算法：FCFS", p, n, sequence, now_seconds() - begin);
}

static void sjf(Process input[], int n) {
    Process p[MAX_PROCESS];
    char sequence[8192] = "";
    int time = 0;
    int done = 0;
    double begin = now_seconds();

    copy_processes(p, input, n);
    while (done < n) {
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
            time = earliest_unfinished_arrival(p, n);
            continue;
        }

        p[selected].start = time;
        append_sequence(sequence, sizeof(sequence), p[selected].pid, time, time + p[selected].burst);
        time += p[selected].burst;
        finish_process(&p[selected], time);
        done++;
    }

    print_schedule_result("基础算法：SJF", p, n, sequence, now_seconds() - begin);
}

static void hrrn(Process input[], int n) {
    Process p[MAX_PROCESS];
    char sequence[8192] = "";
    int time = 0;
    int done = 0;
    double begin = now_seconds();

    copy_processes(p, input, n);
    while (done < n) {
        int selected = -1;
        double best_ratio = -1.0;

        for (int i = 0; i < n; i++) {
            if (!p[i].completed && p[i].arrival <= time) {
                int wait = time - p[i].arrival;
                double ratio = (wait + p[i].burst) * 1.0 / p[i].burst;
                if (ratio > best_ratio ||
                    (ratio == best_ratio && selected != -1 && p[i].arrival < p[selected].arrival)) {
                    selected = i;
                    best_ratio = ratio;
                }
            }
        }

        if (selected == -1) {
            time = earliest_unfinished_arrival(p, n);
            continue;
        }

        p[selected].start = time;
        append_sequence(sequence, sizeof(sequence), p[selected].pid, time, time + p[selected].burst);
        time += p[selected].burst;
        finish_process(&p[selected], time);
        done++;
    }

    print_schedule_result("经典算法：HRRN 高响应比优先", p, n, sequence, now_seconds() - begin);
}

static double adrs_score(const Process *p, int time) {
    int waiting = time - p->arrival;
    int slack = p->deadline - time - p->burst;
    double response_ratio = (waiting + p->burst) * 1.0 / p->burst;
    double deadline_factor = slack < 0 ? -1.0 / (-slack + 1) : 3.0 / (slack + 1);
    double priority_bonus = 1.0 / p->priority;

    return response_ratio + deadline_factor + priority_bonus;
}

static void adrs(Process input[], int n) {
    Process p[MAX_PROCESS];
    char sequence[8192] = "";
    int time = 0;
    int done = 0;
    double begin = now_seconds();

    copy_processes(p, input, n);
    while (done < n) {
        int selected = -1;
        double best_score = -1.0;

        for (int i = 0; i < n; i++) {
            if (!p[i].completed && p[i].arrival <= time) {
                double score = adrs_score(&p[i], time);
                if (score > best_score ||
                    (score == best_score && selected != -1 && p[i].deadline < p[selected].deadline)) {
                    selected = i;
                    best_score = score;
                }
            }
        }

        if (selected == -1) {
            time = earliest_unfinished_arrival(p, n);
            continue;
        }

        p[selected].start = time;
        append_sequence(sequence, sizeof(sequence), p[selected].pid, time, time + p[selected].burst);
        time += p[selected].burst;
        finish_process(&p[selected], time);
        done++;
    }

    print_schedule_result("自定义创新算法：ADRS 自适应老化截止期响应调度", p, n, sequence, now_seconds() - begin);
}

static void edf_process_schedule(Process input[], int n) {
    Process p[MAX_PROCESS];
    char sequence[8192] = "";
    int time = 0;
    int done = 0;
    double begin = now_seconds();

    copy_processes(p, input, n);
    while (done < n) {
        int selected = -1;
        for (int i = 0; i < n; i++) {
            if (!p[i].completed && p[i].arrival <= time) {
                if (selected == -1 ||
                    p[i].deadline < p[selected].deadline ||
                    (p[i].deadline == p[selected].deadline && p[i].arrival < p[selected].arrival)) {
                    selected = i;
                }
            }
        }

        if (selected == -1) {
            time = earliest_unfinished_arrival(p, n);
            continue;
        }

        p[selected].start = time;
        append_sequence(sequence, sizeof(sequence), p[selected].pid, time, time + p[selected].burst);
        time += p[selected].burst;
        finish_process(&p[selected], time);
        done++;
    }

    print_schedule_result("实时调度：EDF 最早截止期优先", p, n, sequence, now_seconds() - begin);
}

static void fill_demo_processes(Process p[], int *n) {
    int data[][5] = {
        {0, 8, 3, 20, 1},
        {1, 4, 1, 9, 2},
        {2, 9, 4, 25, 3},
        {3, 5, 2, 16, 4},
        {5, 2, 1, 12, 5},
        {6, 6, 3, 22, 6}
    };

    *n = 6;
    for (int i = 0; i < *n; i++) {
        p[i].arrival = data[i][0];
        p[i].burst = data[i][1];
        p[i].priority = data[i][2];
        p[i].deadline = data[i][3];
        p[i].pid = data[i][4];
        p[i].remaining = p[i].burst;
    }
}

static int input_processes(Process p[], int *n) {
    if (!read_positive_int("请输入进程数量：", n, MAX_PROCESS)) {
        return 0;
    }

    for (int i = 0; i < *n; i++) {
        p[i].pid = i + 1;
        if (!read_int("到达时间：", &p[i].arrival)) {
            return 0;
        }
        if (!read_positive_int("运行时间：", &p[i].burst, 1000000)) {
            return 0;
        }
        if (!read_positive_int("优先级（数字越小优先级越高）：", &p[i].priority, 1000000)) {
            return 0;
        }
        if (!read_positive_int("绝对截止期：", &p[i].deadline, 1000000)) {
            return 0;
        }
        p[i].remaining = p[i].burst;
    }
    return 1;
}

static void compare_process_schedules(Process p[], int n) {
    printf("\n进程输入表：\n");
    printf("PID\t到达\t运行\t优先级\t截止期\n");
    for (int i = 0; i < n; i++) {
        printf("P%d\t%d\t%d\t%d\t%d\n", p[i].pid, p[i].arrival, p[i].burst, p[i].priority, p[i].deadline);
    }

    fcfs(p, n);
    sjf(p, n);
    hrrn(p, n);
    adrs(p, n);
    edf_process_schedule(p, n);
}

static int gcd_int(int a, int b) {
    while (b != 0) {
        int t = a % b;
        a = b;
        b = t;
    }
    return a;
}

static int lcm_int(int a, int b) {
    return a / gcd_int(a, b) * b;
}

static void fill_demo_tasks(Task tasks[], int *n, int *duration) {
    *n = 3;
    tasks[0] = (Task){1, 4, 1, 4, 0, 0, 0, 0, 0};
    tasks[1] = (Task){2, 5, 2, 5, 0, 0, 0, 0, 0};
    tasks[2] = (Task){3, 10, 2, 10, 0, 0, 0, 0, 0};
    *duration = 40;
}

static int input_tasks(Task tasks[], int *n, int *duration) {
    if (!read_positive_int("请输入实时任务数量：", n, MAX_TASK)) {
        return 0;
    }

    for (int i = 0; i < *n; i++) {
        tasks[i].tid = i + 1;
        if (!read_positive_int("任务周期 period：", &tasks[i].period, 1000)) {
            return 0;
        }
        if (!read_positive_int("每周期执行时间 execution：", &tasks[i].exec_time, 1000)) {
            return 0;
        }
        if (!read_positive_int("相对截止期 deadline：", &tasks[i].relative_deadline, 1000)) {
            return 0;
        }
        tasks[i].next_release = 0;
        tasks[i].remaining = 0;
        tasks[i].absolute_deadline = 0;
        tasks[i].jobs = 0;
        tasks[i].misses = 0;
    }

    if (!read_positive_int("请输入模拟总时长：", duration, MAX_TIME)) {
        return 0;
    }
    return 1;
}

static void reset_tasks(Task dst[], const Task src[], int n) {
    for (int i = 0; i < n; i++) {
        dst[i] = src[i];
        dst[i].next_release = 0;
        dst[i].remaining = 0;
        dst[i].absolute_deadline = 0;
        dst[i].jobs = 0;
        dst[i].misses = 0;
    }
}

static int select_edf_task(Task tasks[], int n) {
    int selected = -1;
    for (int i = 0; i < n; i++) {
        if (tasks[i].remaining > 0) {
            if (selected == -1 ||
                tasks[i].absolute_deadline < tasks[selected].absolute_deadline ||
                (tasks[i].absolute_deadline == tasks[selected].absolute_deadline && tasks[i].tid < tasks[selected].tid)) {
                selected = i;
            }
        }
    }
    return selected;
}

static int select_rms_task(Task tasks[], int n) {
    int selected = -1;
    for (int i = 0; i < n; i++) {
        if (tasks[i].remaining > 0) {
            if (selected == -1 ||
                tasks[i].period < tasks[selected].period ||
                (tasks[i].period == tasks[selected].period && tasks[i].tid < tasks[selected].tid)) {
                selected = i;
            }
        }
    }
    return selected;
}

static void release_jobs(Task tasks[], int n, int time) {
    for (int i = 0; i < n; i++) {
        if (time == tasks[i].next_release) {
            if (tasks[i].remaining > 0) {
                tasks[i].misses++;
                tasks[i].remaining = 0;
            }
            tasks[i].remaining = tasks[i].exec_time;
            tasks[i].absolute_deadline = time + tasks[i].relative_deadline;
            tasks[i].next_release += tasks[i].period;
            tasks[i].jobs++;
        }
    }
}

static void check_deadline_miss(Task tasks[], int n, int time) {
    for (int i = 0; i < n; i++) {
        if (tasks[i].remaining > 0 && time > 0 && time == tasks[i].absolute_deadline) {
            tasks[i].misses++;
            tasks[i].remaining = 0;
        }
    }
}

static void simulate_realtime(const char *name, const Task input[], int n, int duration, int use_edf) {
    Task tasks[MAX_TASK];
    char timeline[MAX_TIME * 8] = "";
    int busy = 0;

    reset_tasks(tasks, input, n);
    for (int time = 0; time < duration; time++) {
        char item[16];
        int selected;

        release_jobs(tasks, n, time);
        selected = use_edf ? select_edf_task(tasks, n) : select_rms_task(tasks, n);

        if (selected == -1) {
            snprintf(item, sizeof(item), "%d:- ", time);
        } else {
            snprintf(item, sizeof(item), "%d:T%d ", time, tasks[selected].tid);
            tasks[selected].remaining--;
            busy++;
        }
        strncat(timeline, item, sizeof(timeline) - strlen(timeline) - 1);
        check_deadline_miss(tasks, n, time + 1);
    }

    printf("\n========== %s ==========\n", name);
    printf("时间线：%s\n", timeline);
    printf("任务\t周期\t执行\t相对截止期\t释放作业数\t错过截止期\n");
    for (int i = 0; i < n; i++) {
        printf("T%d\t%d\t%d\t%d\t\t%d\t\t%d\n",
               tasks[i].tid, tasks[i].period, tasks[i].exec_time,
               tasks[i].relative_deadline, tasks[i].jobs, tasks[i].misses);
    }
    printf("CPU 利用率：%.2f%%\n", busy * 100.0 / duration);
}

static void realtime_study(Task tasks[], int n, int duration) {
    int hyper_period = tasks[0].period;
    double utilization = 0.0;

    for (int i = 0; i < n; i++) {
        hyper_period = lcm_int(hyper_period, tasks[i].period);
        utilization += tasks[i].exec_time * 1.0 / tasks[i].period;
    }

    printf("\n实时任务参数：\n");
    printf("任务\t周期\t执行时间\t相对截止期\n");
    for (int i = 0; i < n; i++) {
        printf("T%d\t%d\t%d\t\t%d\n", tasks[i].tid, tasks[i].period, tasks[i].exec_time, tasks[i].relative_deadline);
    }
    printf("理论利用率 U = %.3f\n", utilization);
    printf("任务超周期 LCM = %d，本次模拟时长 = %d\n", hyper_period, duration);
    printf("说明：EDF 按最早绝对截止期动态选择任务，RMS 按周期越短优先级越高静态选择任务。\n");

    simulate_realtime("EDF 实时调度模拟", tasks, n, duration, 1);
    simulate_realtime("RMS 实时调度模拟", tasks, n, duration, 0);
}

static void *mutex_sum_worker(void *arg) {
    ThreadArg *a = (ThreadArg *)arg;
    for (long long i = a->begin; i <= a->end; i++) {
        pthread_mutex_lock(&global_mutex);
        global_sum += i % 97;
        pthread_mutex_unlock(&global_mutex);
    }
    return NULL;
}

static void *local_sum_worker(void *arg) {
    ThreadArg *a = (ThreadArg *)arg;
    long long sum = 0;
    for (long long i = a->begin; i <= a->end; i++) {
        sum += i % 97;
    }
    a->local_sum = sum;
    return NULL;
}

static int64_t single_thread_sum(int64_t total) {
    int64_t sum = 0;
    for (int64_t i = 1; i <= total; i++) {
        sum += i % 97;
    }
    return sum;
}

static int64_t run_parallel_sum(int64_t total, int threads, int use_mutex) {
    pthread_t tids[MAX_THREADS];
    ThreadArg args[MAX_THREADS];
    int64_t chunk = total / threads;
    int64_t result = 0;

    global_sum = 0;
    for (int i = 0; i < threads; i++) {
        args[i].begin = i * chunk + 1;
        args[i].end = (i == threads - 1) ? total : (i + 1) * chunk;
        args[i].local_sum = 0;
        pthread_create(&tids[i], NULL, use_mutex ? mutex_sum_worker : local_sum_worker, &args[i]);
    }

    for (int i = 0; i < threads; i++) {
        pthread_join(tids[i], NULL);
    }

    if (use_mutex) {
        result = global_sum;
    } else {
        for (int i = 0; i < threads; i++) {
            result += args[i].local_sum;
        }
    }
    return result;
}

static void concurrency_optimization_demo(void) {
    int64_t total = 2000000;
    int threads = 4;
    double t1, t2, t3, begin;
    int64_t r1, r2, r3;

    printf("\n========== 并发性能优化实验 ==========\n");
    printf("任务：计算 1 到 %" PRId64 " 的 i %% 97 累加和。\n", total);
    printf("线程数：%d\n", threads);

    begin = now_seconds();
    r1 = single_thread_sum(total);
    t1 = now_seconds() - begin;

    begin = now_seconds();
    r2 = run_parallel_sum(total, threads, 1);
    t2 = now_seconds() - begin;

    begin = now_seconds();
    r3 = run_parallel_sum(total, threads, 0);
    t3 = now_seconds() - begin;

    printf("方案\t\t\t结果\t\t耗时/秒\t\t相对单线程加速比\n");
    printf("单线程\t\t\t%" PRId64 "\t%.6f\t%.2f\n", r1, t1, 1.0);
    printf("多线程全局锁累加\t%" PRId64 "\t%.6f\t%.2f\n", r2, t2, t2 > 0 ? t1 / t2 : 0.0);
    printf("多线程局部累加优化\t%" PRId64 "\t%.6f\t%.2f\n", r3, t3, t3 > 0 ? t1 / t3 : 0.0);
    printf("分析：全局锁方案每次累加都进入临界区，锁竞争严重；局部累加只在线程结束后合并结果，减少了同步开销。\n");
}

static void run_demo_all(void) {
    Process processes[MAX_PROCESS];
    Task tasks[MAX_TASK];
    int process_count;
    int task_count;
    int duration;

    fill_demo_processes(processes, &process_count);
    compare_process_schedules(processes, process_count);

    fill_demo_tasks(tasks, &task_count, &duration);
    realtime_study(tasks, task_count, duration);

    concurrency_optimization_demo();
}

int main(void) {
    int choice;

    while (1) {
        printf("\n========== 调度与性能优化实验 ==========\n");
        printf("1. 手动输入进程并比较调度算法\n");
        printf("2. 手动输入实时任务并比较 EDF/RMS\n");
        printf("3. 并发性能优化实验\n");
        printf("4. 一键运行全部示例\n");
        printf("0. 退出\n");

        if (!read_int("请选择：", &choice)) {
            return 1;
        }

        if (choice == 1) {
            Process processes[MAX_PROCESS];
            int n;
            if (input_processes(processes, &n)) {
                compare_process_schedules(processes, n);
            }
        } else if (choice == 2) {
            Task tasks[MAX_TASK];
            int n, duration;
            if (input_tasks(tasks, &n, &duration)) {
                realtime_study(tasks, n, duration);
            }
        } else if (choice == 3) {
            concurrency_optimization_demo();
        } else if (choice == 4) {
            run_demo_all();
        } else if (choice == 0) {
            return 0;
        } else {
            printf("无效选择。\n");
        }
    }
}
