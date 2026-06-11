#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILES 32
#define MAX_BLOCKS 64
#define BLOCK_SIZE 32
#define MAX_NAME 32
#define MAX_CONTENT 1024
#define MAX_FILE_BLOCKS 32

typedef struct {
    int used;
    char name[MAX_NAME];
    int size;
    int block_count;
    int blocks[MAX_FILE_BLOCKS];
} FCB;

static FCB files[MAX_FILES];
static char disk[MAX_BLOCKS][BLOCK_SIZE];
static int bitmap[MAX_BLOCKS];

static void clear_input_line(void) {
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {
    }
}

static int read_int(const char *prompt, int *value) {
    printf("%s", prompt);
    if (scanf("%d", value) != 1) {
        printf("输入错误：需要输入整数。\n");
        clear_input_line();
        return 0;
    }
    return 1;
}

static int read_name(const char *prompt, char name[]) {
    char temp[128];

    printf("%s", prompt);
    if (scanf("%127s", temp) != 1) {
        printf("输入错误：文件名读取失败。\n");
        clear_input_line();
        return 0;
    }

    if (strlen(temp) >= MAX_NAME) {
        printf("输入错误：文件名过长，最多 %d 个字符。\n", MAX_NAME - 1);
        return 0;
    }

    strcpy(name, temp);
    return 1;
}

static void init_filesystem(void) {
    for (int i = 0; i < MAX_FILES; i++) {
        files[i].used = 0;
        files[i].name[0] = '\0';
        files[i].size = 0;
        files[i].block_count = 0;
    }
    for (int i = 0; i < MAX_BLOCKS; i++) {
        bitmap[i] = 0;
        memset(disk[i], 0, BLOCK_SIZE);
    }
}

static int find_file(const char *name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

static int find_empty_fcb(void) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].used) {
            return i;
        }
    }
    return -1;
}

static int count_free_blocks(void) {
    int count = 0;
    for (int i = 0; i < MAX_BLOCKS; i++) {
        if (!bitmap[i]) {
            count++;
        }
    }
    return count;
}

static int allocate_blocks(int need, int out_blocks[]) {
    int allocated = 0;

    for (int i = 0; i < MAX_BLOCKS && allocated < need; i++) {
        if (!bitmap[i]) {
            bitmap[i] = 1;
            out_blocks[allocated++] = i;
        }
    }

    if (allocated == need) {
        return 1;
    }

    for (int i = 0; i < allocated; i++) {
        bitmap[out_blocks[i]] = 0;
    }
    return 0;
}

static void release_file_blocks(FCB *file) {
    for (int i = 0; i < file->block_count; i++) {
        int block = file->blocks[i];
        if (block >= 0 && block < MAX_BLOCKS) {
            bitmap[block] = 0;
            memset(disk[block], 0, BLOCK_SIZE);
        }
    }
    file->size = 0;
    file->block_count = 0;
}

static void create_file(void) {
    char name[MAX_NAME];
    int idx;

    if (!read_name("请输入文件名：", name)) {
        return;
    }

    if (find_file(name) != -1) {
        printf("创建失败：文件 %s 已存在。\n", name);
        return;
    }

    idx = find_empty_fcb();
    if (idx == -1) {
        printf("创建失败：文件控制块已满。\n");
        return;
    }

    files[idx].used = 1;
    strcpy(files[idx].name, name);
    files[idx].size = 0;
    files[idx].block_count = 0;

    printf("文件 %s 创建成功。\n", name);
}

static void write_file(void) {
    char name[MAX_NAME];
    char content[MAX_CONTENT];
    int idx, length, need_blocks;
    int new_blocks[MAX_FILE_BLOCKS];

    if (!read_name("请输入要写入的文件名：", name)) {
        return;
    }

    idx = find_file(name);
    if (idx == -1) {
        printf("写入失败：文件不存在。\n");
        return;
    }

    printf("请输入写入内容（不超过 %d 个字符）：", MAX_CONTENT - 1);
    clear_input_line();
    if (fgets(content, sizeof(content), stdin) == NULL) {
        printf("读取内容失败。\n");
        return;
    }
    content[strcspn(content, "\n")] = '\0';

    length = (int)strlen(content);
    need_blocks = (length + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (length == 0) {
        need_blocks = 0;
    }

    if (need_blocks > MAX_FILE_BLOCKS) {
        printf("写入失败：文件过大，最多占用 %d 个块。\n", MAX_FILE_BLOCKS);
        return;
    }

    if (count_free_blocks() < need_blocks || !allocate_blocks(need_blocks, new_blocks)) {
        printf("写入失败：磁盘空闲块不足。\n");
        return;
    }

    release_file_blocks(&files[idx]);
    files[idx].size = length;
    files[idx].block_count = need_blocks;

    for (int i = 0; i < need_blocks; i++) {
        int bytes = length - i * BLOCK_SIZE;
        if (bytes > BLOCK_SIZE) {
            bytes = BLOCK_SIZE;
        }

        files[idx].blocks[i] = new_blocks[i];
        memset(disk[new_blocks[i]], 0, BLOCK_SIZE);
        memcpy(disk[new_blocks[i]], content + i * BLOCK_SIZE, (size_t)bytes);
    }

    printf("写入成功：文件 %s 大小 %d 字节，占用 %d 个块。\n", name, length, need_blocks);
}

static void read_file(void) {
    char name[MAX_NAME];
    int idx;
    int remaining;

    if (!read_name("请输入要读取的文件名：", name)) {
        return;
    }

    idx = find_file(name);
    if (idx == -1) {
        printf("读取失败：文件不存在。\n");
        return;
    }

    printf("文件 %s 内容：", name);
    remaining = files[idx].size;
    for (int i = 0; i < files[idx].block_count; i++) {
        int block = files[idx].blocks[i];
        int bytes = remaining > BLOCK_SIZE ? BLOCK_SIZE : remaining;
        for (int j = 0; j < bytes; j++) {
            putchar(disk[block][j]);
        }
        remaining -= bytes;
    }
    printf("\n");
}

static void delete_file(void) {
    char name[MAX_NAME];
    int idx;

    if (!read_name("请输入要删除的文件名：", name)) {
        return;
    }

    idx = find_file(name);
    if (idx == -1) {
        printf("删除失败：文件不存在。\n");
        return;
    }

    release_file_blocks(&files[idx]);
    files[idx].used = 0;
    files[idx].name[0] = '\0';
    printf("文件 %s 删除成功，所占数据块已释放。\n", name);
}

static void list_files(void) {
    int has_file = 0;

    printf("\n目录列表：\n");
    printf("文件名\t大小\t块数\t占用块号\n");
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used) {
            has_file = 1;
            printf("%s\t%d\t%d\t", files[i].name, files[i].size, files[i].block_count);
            for (int j = 0; j < files[i].block_count; j++) {
                printf("%d ", files[i].blocks[j]);
            }
            printf("\n");
        }
    }
    if (!has_file) {
        printf("当前目录为空。\n");
    }
}

static void show_bitmap(void) {
    printf("\n空闲空间位示图（0 表示空闲，1 表示占用）：\n");
    for (int i = 0; i < MAX_BLOCKS; i++) {
        printf("%d", bitmap[i]);
        if ((i + 1) % 8 == 0) {
            printf(" ");
        }
        if ((i + 1) % 32 == 0) {
            printf("\n");
        }
    }
    printf("空闲块数量：%d / %d\n", count_free_blocks(), MAX_BLOCKS);
}

static void show_help(void) {
    printf("\n本模拟文件系统参数：\n");
    printf("最大文件数：%d\n", MAX_FILES);
    printf("磁盘块数量：%d\n", MAX_BLOCKS);
    printf("每块大小：%d 字节\n", BLOCK_SIZE);
    printf("空闲空间管理方式：位示图 bitmap\n");
    printf("文件组织方式：索引分配，FCB 中保存文件占用的数据块号\n");
}

int main(void) {
    int choice;

    init_filesystem();
    while (1) {
        printf("\n========== 简单文件系统模拟 ==========\n");
        printf("1. 创建文件\n");
        printf("2. 写文件\n");
        printf("3. 读文件\n");
        printf("4. 删除文件\n");
        printf("5. 显示目录\n");
        printf("6. 显示空闲空间位示图\n");
        printf("7. 显示系统参数\n");
        printf("0. 退出\n");

        if (!read_int("请选择：", &choice)) {
            return 1;
        }

        switch (choice) {
            case 1:
                create_file();
                break;
            case 2:
                write_file();
                break;
            case 3:
                read_file();
                break;
            case 4:
                delete_file();
                break;
            case 5:
                list_files();
                break;
            case 6:
                show_bitmap();
                break;
            case 7:
                show_help();
                break;
            case 0:
                return 0;
            default:
                printf("无效选择。\n");
        }
    }
}
