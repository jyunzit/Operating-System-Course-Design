#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_REF 200
#define MAX_FRAME 20
#define MAX_BLOCK 100

typedef struct {
    int start;
    int size;
    int free;
    int pid;
} Block;

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

static void print_frames(const int frames[], int frame_count) {
    printf("内存块：");
    for (int i = 0; i < frame_count; i++) {
        if (frames[i] == -1) {
            printf("[ ] ");
        } else {
            printf("[%d] ", frames[i]);
        }
    }
}

static int find_page(const int frames[], int frame_count, int page) {
    for (int i = 0; i < frame_count; i++) {
        if (frames[i] == page) {
            return i;
        }
    }
    return -1;
}

static int read_page_data(int refs[], int *n, int *frame_count) {
    if (!read_positive_int("请输入页面访问序列长度：", n, MAX_REF)) {
        return 0;
    }

    printf("请输入页面访问序列：");
    for (int i = 0; i < *n; i++) {
        if (scanf("%d", &refs[i]) != 1) {
            printf("输入错误：页面号必须是整数。\n");
            return 0;
        }
    }

    if (!read_positive_int("请输入物理块数量：", frame_count, MAX_FRAME)) {
        return 0;
    }
    return 1;
}

static void fifo_page(void) {
    int refs[MAX_REF], n, frame_count;
    int frames[MAX_FRAME];
    int pointer = 0;
    int faults = 0;

    if (!read_page_data(refs, &n, &frame_count)) {
        return;
    }

    for (int i = 0; i < frame_count; i++) {
        frames[i] = -1;
    }

    printf("\nFIFO 页面置换过程：\n");
    for (int i = 0; i < n; i++) {
        int page = refs[i];
        int hit = find_page(frames, frame_count, page) != -1;
        if (!hit) {
            frames[pointer] = page;
            pointer = (pointer + 1) % frame_count;
            faults++;
        }
        printf("访问页面 %d：", page);
        print_frames(frames, frame_count);
        printf("%s\n", hit ? "命中" : "缺页");
    }

    printf("缺页次数：%d\n", faults);
    printf("缺页率：%.2f%%\n", faults * 100.0 / n);
}

static void lru_page(void) {
    int refs[MAX_REF], n, frame_count;
    int frames[MAX_FRAME], last_used[MAX_FRAME];
    int faults = 0;

    if (!read_page_data(refs, &n, &frame_count)) {
        return;
    }

    for (int i = 0; i < frame_count; i++) {
        frames[i] = -1;
        last_used[i] = -1;
    }

    printf("\nLRU 页面置换过程：\n");
    for (int time = 0; time < n; time++) {
        int page = refs[time];
        int pos = find_page(frames, frame_count, page);
        int hit = pos != -1;

        if (hit) {
            last_used[pos] = time;
        } else {
            int replace = -1;
            for (int i = 0; i < frame_count; i++) {
                if (frames[i] == -1) {
                    replace = i;
                    break;
                }
            }
            if (replace == -1) {
                int oldest = INT_MAX;
                for (int i = 0; i < frame_count; i++) {
                    if (last_used[i] < oldest) {
                        oldest = last_used[i];
                        replace = i;
                    }
                }
            }
            frames[replace] = page;
            last_used[replace] = time;
            faults++;
        }

        printf("访问页面 %d：", page);
        print_frames(frames, frame_count);
        printf("%s\n", hit ? "命中" : "缺页");
    }

    printf("缺页次数：%d\n", faults);
    printf("缺页率：%.2f%%\n", faults * 100.0 / n);
}

static void print_blocks(const Block blocks[], int count) {
    printf("\n当前动态分区表：\n");
    printf("序号\t起址\t大小\t状态\t作业\n");
    for (int i = 0; i < count; i++) {
        printf("%d\t%d\t%d\t%s\t", i, blocks[i].start, blocks[i].size,
               blocks[i].free ? "空闲" : "占用");
        if (blocks[i].free) {
            printf("-\n");
        } else {
            printf("J%d\n", blocks[i].pid);
        }
    }
}

static void merge_free_blocks(Block blocks[], int *count) {
    int i = 0;
    while (i < *count - 1) {
        if (blocks[i].free && blocks[i + 1].free) {
            blocks[i].size += blocks[i + 1].size;
            for (int j = i + 1; j < *count - 1; j++) {
                blocks[j] = blocks[j + 1];
            }
            (*count)--;
        } else {
            i++;
        }
    }
}

static int select_block(const Block blocks[], int count, int size, int algorithm) {
    int selected = -1;
    for (int i = 0; i < count; i++) {
        if (blocks[i].free && blocks[i].size >= size) {
            if (algorithm == 1) {
                return i;
            }
            if (selected == -1 || blocks[i].size < blocks[selected].size) {
                selected = i;
            }
        }
    }
    return selected;
}

static void allocate_block(Block blocks[], int *count, int pid, int size, int algorithm) {
    int idx;

    if (pid <= 0 || size <= 0) {
        printf("分配失败：作业号和申请大小必须为正数。\n");
        return;
    }

    idx = select_block(blocks, *count, size, algorithm);
    if (idx == -1) {
        printf("J%d 申请 %dKB 失败：没有足够连续空闲空间。\n", pid, size);
        return;
    }

    if (blocks[idx].size == size) {
        blocks[idx].free = 0;
        blocks[idx].pid = pid;
    } else {
        if (*count >= MAX_BLOCK) {
            printf("分配失败：分区表已满。\n");
            return;
        }
        for (int i = *count; i > idx + 1; i--) {
            blocks[i] = blocks[i - 1];
        }
        blocks[idx + 1].start = blocks[idx].start + size;
        blocks[idx + 1].size = blocks[idx].size - size;
        blocks[idx + 1].free = 1;
        blocks[idx + 1].pid = -1;
        blocks[idx].size = size;
        blocks[idx].free = 0;
        blocks[idx].pid = pid;
        (*count)++;
    }
    printf("J%d 申请 %dKB 成功。\n", pid, size);
}

static void free_block(Block blocks[], int *count, int pid) {
    for (int i = 0; i < *count; i++) {
        if (!blocks[i].free && blocks[i].pid == pid) {
            blocks[i].free = 1;
            blocks[i].pid = -1;
            printf("J%d 回收成功。\n", pid);
            merge_free_blocks(blocks, count);
            return;
        }
    }
    printf("J%d 不存在或已经释放。\n", pid);
}

static void partition_manage(void) {
    Block blocks[MAX_BLOCK];
    int count = 1;
    int total, algorithm, op;

    if (!read_positive_int("请输入总内存大小 KB：", &total, 1000000)) {
        return;
    }
    if (!read_int("选择分配算法：1. 首次适应 FF  2. 最佳适应 BF：", &algorithm)) {
        return;
    }
    if (algorithm != 1 && algorithm != 2) {
        printf("输入错误：分配算法只能选择 1 或 2。\n");
        return;
    }

    blocks[0].start = 0;
    blocks[0].size = total;
    blocks[0].free = 1;
    blocks[0].pid = -1;

    while (1) {
        print_blocks(blocks, count);
        printf("\n操作：1. 分配  2. 回收  0. 返回主菜单\n");
        if (!read_int("请选择：", &op)) {
            return;
        }

        if (op == 0) {
            break;
        } else if (op == 1) {
            int pid, size;
            if (!read_int("请输入作业号：", &pid)) {
                return;
            }
            if (!read_int("请输入申请大小 KB：", &size)) {
                return;
            }
            allocate_block(blocks, &count, pid, size, algorithm);
        } else if (op == 2) {
            int pid;
            if (!read_int("请输入要回收的作业号：", &pid)) {
                return;
            }
            free_block(blocks, &count, pid);
        } else {
            printf("无效操作。\n");
        }
    }
}

int main(void) {
    int choice;

    while (1) {
        printf("\n========== 内存管理模拟 ==========\n");
        printf("1. FIFO 页面置换\n");
        printf("2. LRU 页面置换\n");
        printf("3. 动态分区管理（FF/BF）\n");
        printf("0. 退出\n");
        if (!read_int("请选择：", &choice)) {
            return 1;
        }

        switch (choice) {
            case 1:
                fifo_page();
                break;
            case 2:
                lru_page();
                break;
            case 3:
                partition_manage();
                break;
            case 0:
                return 0;
            default:
                printf("无效选择。\n");
        }
    }
}
