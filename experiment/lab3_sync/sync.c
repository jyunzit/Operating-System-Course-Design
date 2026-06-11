#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BUFFER_SIZE 5
#define PRODUCER_COUNT 2
#define CONSUMER_COUNT 2
#define PRODUCE_TIMES 5
#define READER_COUNT 3
#define WRITER_COUNT 2
#define PHIL_COUNT 5
#define EAT_TIMES 3

static void short_pause(int unit) {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = unit * 100000000L;
    nanosleep(&ts, NULL);
}

/* ===================== 生产者-消费者 ===================== */

static int buffer[BUFFER_SIZE];
static int in_pos = 0;
static int out_pos = 0;
static sem_t empty_slots;
static sem_t full_slots;
static pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *producer(void *arg) {
    int id = *(int *)arg;
    for (int i = 1; i <= PRODUCE_TIMES; i++) {
        int item = id * 100 + i;
        sem_wait(&empty_slots);
        pthread_mutex_lock(&buffer_mutex);

        buffer[in_pos] = item;
        printf("生产者 %d 生产产品 %d，放入位置 %d\n", id, item, in_pos);
        in_pos = (in_pos + 1) % BUFFER_SIZE;

        pthread_mutex_unlock(&buffer_mutex);
        sem_post(&full_slots);
        short_pause(1);
    }
    return NULL;
}

static void *consumer(void *arg) {
    int id = *(int *)arg;
    for (int i = 1; i <= PRODUCE_TIMES; i++) {
        sem_wait(&full_slots);
        pthread_mutex_lock(&buffer_mutex);

        int item = buffer[out_pos];
        printf("消费者 %d 消费产品 %d，取出位置 %d\n", id, item, out_pos);
        out_pos = (out_pos + 1) % BUFFER_SIZE;

        pthread_mutex_unlock(&buffer_mutex);
        sem_post(&empty_slots);
        short_pause(2);
    }
    return NULL;
}

static void run_producer_consumer(void) {
    pthread_t producers[PRODUCER_COUNT], consumers[CONSUMER_COUNT];
    int producer_ids[PRODUCER_COUNT], consumer_ids[CONSUMER_COUNT];

    in_pos = 0;
    out_pos = 0;
    sem_init(&empty_slots, 0, BUFFER_SIZE);
    sem_init(&full_slots, 0, 0);

    printf("\n========== 生产者-消费者问题 ==========\n");
    for (int i = 0; i < PRODUCER_COUNT; i++) {
        producer_ids[i] = i + 1;
        pthread_create(&producers[i], NULL, producer, &producer_ids[i]);
    }
    for (int i = 0; i < CONSUMER_COUNT; i++) {
        consumer_ids[i] = i + 1;
        pthread_create(&consumers[i], NULL, consumer, &consumer_ids[i]);
    }

    for (int i = 0; i < PRODUCER_COUNT; i++) {
        pthread_join(producers[i], NULL);
    }
    for (int i = 0; i < CONSUMER_COUNT; i++) {
        pthread_join(consumers[i], NULL);
    }

    sem_destroy(&empty_slots);
    sem_destroy(&full_slots);
    printf("生产者-消费者模拟结束。\n");
}

/* ===================== 读者-写者 ===================== */

static int shared_data = 0;
static int reader_count = 0;
static pthread_mutex_t reader_count_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t rw_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *reader(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < 3; i++) {
        pthread_mutex_lock(&reader_count_mutex);
        reader_count++;
        if (reader_count == 1) {
            pthread_mutex_lock(&rw_mutex);
        }
        pthread_mutex_unlock(&reader_count_mutex);

        printf("读者 %d 正在读，shared_data = %d\n", id, shared_data);
        short_pause(1);

        pthread_mutex_lock(&reader_count_mutex);
        reader_count--;
        if (reader_count == 0) {
            pthread_mutex_unlock(&rw_mutex);
        }
        pthread_mutex_unlock(&reader_count_mutex);

        short_pause(1);
    }
    return NULL;
}

static void *writer(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < 3; i++) {
        pthread_mutex_lock(&rw_mutex);

        shared_data += 10;
        printf("写者 %d 正在写，shared_data 修改为 %d\n", id, shared_data);
        short_pause(2);

        pthread_mutex_unlock(&rw_mutex);
        short_pause(2);
    }
    return NULL;
}

static void run_reader_writer(void) {
    pthread_t readers[READER_COUNT], writers[WRITER_COUNT];
    int reader_ids[READER_COUNT], writer_ids[WRITER_COUNT];

    shared_data = 0;
    reader_count = 0;
    printf("\n========== 读者-写者问题 ==========\n");

    for (int i = 0; i < WRITER_COUNT; i++) {
        writer_ids[i] = i + 1;
        pthread_create(&writers[i], NULL, writer, &writer_ids[i]);
    }
    for (int i = 0; i < READER_COUNT; i++) {
        reader_ids[i] = i + 1;
        pthread_create(&readers[i], NULL, reader, &reader_ids[i]);
    }

    for (int i = 0; i < WRITER_COUNT; i++) {
        pthread_join(writers[i], NULL);
    }
    for (int i = 0; i < READER_COUNT; i++) {
        pthread_join(readers[i], NULL);
    }
    printf("读者-写者模拟结束。\n");
}

/* ===================== 哲学家进餐 ===================== */

static pthread_mutex_t forks[PHIL_COUNT];

static void *philosopher(void *arg) {
    int id = *(int *)arg;
    int left = id;
    int right = (id + 1) % PHIL_COUNT;

    for (int i = 0; i < EAT_TIMES; i++) {
        printf("哲学家 %d 正在思考。\n", id + 1);
        short_pause(1);

        if (id % 2 == 0) {
            pthread_mutex_lock(&forks[left]);
            pthread_mutex_lock(&forks[right]);
        } else {
            pthread_mutex_lock(&forks[right]);
            pthread_mutex_lock(&forks[left]);
        }

        printf("哲学家 %d 拿到筷子 %d 和 %d，正在进餐。\n", id + 1, left + 1, right + 1);
        short_pause(2);

        pthread_mutex_unlock(&forks[left]);
        pthread_mutex_unlock(&forks[right]);
        printf("哲学家 %d 放下筷子，结束本次进餐。\n", id + 1);
        short_pause(1);
    }
    return NULL;
}

static void run_dining_philosophers(void) {
    pthread_t philosophers[PHIL_COUNT];
    int ids[PHIL_COUNT];

    printf("\n========== 哲学家进餐问题 ==========\n");
    for (int i = 0; i < PHIL_COUNT; i++) {
        pthread_mutex_init(&forks[i], NULL);
    }
    for (int i = 0; i < PHIL_COUNT; i++) {
        ids[i] = i;
        pthread_create(&philosophers[i], NULL, philosopher, &ids[i]);
    }
    for (int i = 0; i < PHIL_COUNT; i++) {
        pthread_join(philosophers[i], NULL);
    }
    for (int i = 0; i < PHIL_COUNT; i++) {
        pthread_mutex_destroy(&forks[i]);
    }
    printf("哲学家进餐模拟结束。\n");
}

int main(void) {
    int choice;

    while (1) {
        printf("\n========== 进程同步与并发控制 ==========\n");
        printf("1. 生产者-消费者问题\n");
        printf("2. 读者-写者问题\n");
        printf("3. 哲学家进餐问题\n");
        printf("4. 全部运行\n");
        printf("0. 退出\n");
        printf("请选择：");
        if (scanf("%d", &choice) != 1) {
            printf("输入错误。\n");
            return 1;
        }

        switch (choice) {
            case 1:
                run_producer_consumer();
                break;
            case 2:
                run_reader_writer();
                break;
            case 3:
                run_dining_philosophers();
                break;
            case 4:
                run_producer_consumer();
                run_reader_writer();
                run_dining_philosophers();
                break;
            case 0:
                return 0;
            default:
                printf("无效选择。\n");
        }
    }
}
