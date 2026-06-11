# Linux 操作系统课程实验

本仓库为 Linux / 操作系统课程实验代码集合，使用 C 语言实现，所有实验均可在 Linux 终端下通过 `gcc` 和 `make` 编译运行。
## 实验目录结构

```text
experiment/
├── lab1_scheduler/
│   ├── scheduler.c
│   ├── Makefile
│   └── sample_input.txt
├── lab2_memory/
│   ├── memory.c
│   ├── Makefile
│   ├── sample_fifo.txt
│   ├── sample_lru.txt
│   └── sample_partition.txt
├── lab3_sync/
│   ├── sync.c
│   ├── Makefile
│   └── sample_input.txt
├── lab4_filesystem/
│   ├── filesystem.c
│   ├── Makefile
│   └── sample_input.txt
└── lab5_sched_perf/
    ├── sched_perf.c
    ├── Makefile
    ├── sample_input.txt
    ├── manual_process_input.txt
    └── manual_realtime_input.txt
```

## 实验环境

推荐环境：

- 操作系统：Ubuntu / Debian / CentOS / Rocky Linux 等 Linux 系统
- 编译器：gcc
- 构建工具：make
- 线程库：pthread

Ubuntu / Debian 安装编译工具：

```bash
sudo apt update
sudo apt install build-essential
```

CentOS / Rocky Linux 安装编译工具：

```bash
sudo yum groupinstall "Development Tools"
```

检查安装情况：

```bash
gcc --version
make --version
```

## 实验一：处理机调度

目录：

```bash
lab1_scheduler
```

功能说明：

- 实现 FCFS 先来先服务调度。
- 实现 SJF 短作业优先调度。
- 实现 RR 时间片轮转调度。
- 实现优先级调度。
- 输出进程运行顺序、完成时间、周转时间、等待时间、平均等待时间和平均周转时间。

编译运行：

```bash
cd lab1_scheduler
make
./scheduler
```

使用示例输入测试：

```bash
./scheduler < sample_input.txt
```

如果没有 `make`，可以直接使用：

```bash
gcc -Wall -Wextra -std=c99 -O2 -o scheduler scheduler.c
./scheduler < sample_input.txt
```

清理可执行文件：

```bash
make clean
```

## 实验二：内存管理

目录：

```bash
lab2_memory
```

功能说明：

- 实现 FIFO 页面置换算法。
- 实现 LRU 页面置换算法。
- 实现首次适应 FF 动态分区分配。
- 实现最佳适应 BF 动态分区分配。
- 输出页面置换过程、缺页次数、缺页率和动态分区表变化。

编译运行：

```bash
cd lab2_memory
make
./memory
```

使用示例输入测试：

```bash
./memory < sample_fifo.txt
./memory < sample_lru.txt
./memory < sample_partition.txt
```

如果没有 `make`，可以直接使用：

```bash
gcc -Wall -Wextra -std=c99 -O2 -o memory memory.c
./memory < sample_fifo.txt
```

清理可执行文件：

```bash
make clean
```

## 实验三：进程同步与并发控制

目录：

```bash
lab3_sync
```

功能说明：

- 使用多线程模拟生产者-消费者问题。
- 使用多线程模拟读者-写者问题。
- 使用多线程模拟哲学家进餐问题。
- 使用互斥锁、信号量等同步机制。
- 避免死锁和数据竞争。

编译运行：

```bash
cd lab3_sync
make
./sync
```

使用示例输入测试：

```bash
./sync < sample_input.txt
```

如果没有 `make`，可以直接使用：

```bash
gcc -Wall -Wextra -std=c99 -O2 -pthread -o sync sync.c
./sync < sample_input.txt
```

注意：本实验使用 `pthread`，编译时必须加 `-pthread`。

清理可执行文件：

```bash
make clean
```

## 实验四：文件系统

目录：

```bash
lab4_filesystem
```

功能说明：

- 模拟简单文件系统。
- 支持文件创建、写入、读取、删除。
- 支持目录列表显示。
- 使用位示图 bitmap 管理空闲空间。
- 使用 FCB 模拟文件控制块。

编译运行：

```bash
cd lab4_filesystem
make
./filesystem
```

使用示例输入测试：

```bash
./filesystem < sample_input.txt
```

如果没有 `make`，可以直接使用：

```bash
gcc -Wall -Wextra -std=c99 -O2 -o filesystem filesystem.c
./filesystem < sample_input.txt
```

清理可执行文件：

```bash
make clean
```

## 实验五：调度与性能优化

目录：

```bash
lab5_sched_perf
```

功能说明：

- 比较 FCFS、SJF、HRRN、EDF 等调度算法。
- 实现自定义改进算法 ADRS 自适应老化截止期响应调度。
- 研究 EDF 和 RMS 实时调度机制。
- 统计平均等待时间、平均周转时间、平均响应时间、吞吐率、截止期违约数和 CPU 利用率。
- 进行并发性能优化实验，对比单线程、多线程全局锁累加和多线程局部累加优化。

编译运行：

```bash
cd lab5_sched_perf
make
./sched_perf
```

一键运行全部示例：

```bash
./sched_perf < sample_input.txt
```

单独测试进程调度算法：

```bash
./sched_perf < manual_process_input.txt
```

单独测试实时调度算法：

```bash
./sched_perf < manual_realtime_input.txt
```

如果没有 `make`，可以直接使用：

```bash
gcc -Wall -Wextra -std=c99 -O2 -pthread -o sched_perf sched_perf.c
./sched_perf < sample_input.txt
```

注意：本实验使用 `pthread`，编译时必须加 `-pthread`。

清理可执行文件：

```bash
make clean
```

## 一键编译全部实验

如果已经进入项目根目录，可以依次编译所有实验：

```bash
cd lab1_scheduler && make && cd ..
cd lab2_memory && make && cd ..
cd lab3_sync && make && cd ..
cd lab4_filesystem && make && cd ..
cd lab5_sched_perf && make && cd ..
```

## 一键运行全部示例

```bash
cd lab1_scheduler && ./scheduler < sample_input.txt && cd ..
cd lab2_memory && ./memory < sample_fifo.txt && ./memory < sample_lru.txt && ./memory < sample_partition.txt && cd ..
cd lab3_sync && ./sync < sample_input.txt && cd ..
cd lab4_filesystem && ./filesystem < sample_input.txt && cd ..
cd lab5_sched_perf && ./sched_perf < sample_input.txt && cd ..
```

## 常见问题

### 1. 提示 `gcc: command not found`

说明系统未安装 gcc。Ubuntu / Debian 下执行：

```bash
sudo apt install build-essential
```

### 2. 提示 `make: command not found`

说明系统未安装 make。可以安装 `build-essential`，也可以使用各实验说明中的 `gcc` 命令直接编译。

### 3. pthread 相关实验编译失败

实验三和实验五使用了 pthread，多线程程序编译时需要加 `-pthread`：

```bash
gcc -Wall -Wextra -std=c99 -O2 -pthread -o sync sync.c
gcc -Wall -Wextra -std=c99 -O2 -pthread -o sched_perf sched_perf.c
```

### 4. 如何证明程序运行正确

每个实验目录均提供示例输入文件，可以使用输入重定向快速测试。例如：

```bash
./scheduler < sample_input.txt
./memory < sample_fifo.txt
./sync < sample_input.txt
./filesystem < sample_input.txt
./sched_perf < sample_input.txt
```

运行后程序会输出调度顺序、缺页率、同步过程、文件系统状态或性能分析结果，可用于实验报告截图。
