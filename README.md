# Linux 线程与进程基础

这是一个面向嵌入式 Linux 用户态开发的 C++17 学习项目。六个示例分别演示线程、同步、进程和管道通信；每个源码都有独立的 main，分别生成一个可执行文件。

目标环境是 Ubuntu/Debian，不需要开发板。当前没有外设驱动、裸机/RTOS 支持或交叉编译配置。Windows 可以构建两个标准 C++ 线程示例，完整的六个示例和 Sanitizer 验收在 Linux 上进行。

## 学习路线与目录

| 顺序 | 示例 | 观察重点 |
| --- | --- | --- |
| 1 | [std_thread.cpp](01_thread/std_thread.cpp) | 函数参数、lambda、线程启动与 join、输出交错 |
| 2 | [pthread_demo.cpp](01_thread/pthread_demo.cpp) | POSIX 线程入口、结构体参数、返回指针、错误编号 |
| 3 | [thread_sync.cpp](01_thread/thread_sync.cpp) | 两个生产者、两个消费者、容量为 5 的共享队列 |
| 4 | [fork_demo.cpp](02_process/fork_demo.cpp) | 父子 PID、fork 三种返回值、waitpid 回收 |
| 5 | [exec_demo.cpp](02_process/exec_demo.cpp) | fork + execvp 运行 ls，向调用方传递子进程退出状态 |
| 6 | [pipe_ipc.cpp](02_process/pipe_ipc.cpp) | 父进程写入、子进程分块读取、关闭写端产生 EOF |

深入说明见[线程总结](01_thread/线程总结.md)和[进程总结](02_process/进程总结.md)。自动化验收入口是 [tests/check_examples.py](tests/check_examples.py)，仅依赖 Python 标准库。已执行和待执行的检查见[验收记录](VALIDATION.md)。

## Ubuntu/Debian 环境

在 Linux 终端执行：

~~~bash
cat /etc/os-release
uname -m
sudo apt update
sudo apt install build-essential cmake python3 gdb strace manpages-dev
g++ --version
cmake --version
python3 --version
~~~

需要支持 C++17 的编译器、CMake 3.16 或更新版本、Python 3.8 或更新版本。Python 仅用于测试，关闭 BUILD_TESTING 后不需要它。GDB 和 strace 用于学习与调试。

将整个目录复制到 Linux 的 ~/Embedded_System；本地 Windows 的盘符路径不能直接作为另一台 Linux 机器的路径。如果在 Windows 上编辑，可以用 [VS Code Remote-SSH](https://code.visualstudio.com/docs/remote/ssh) 打开 Linux 上的项目，使编译器和调试器也运行在 Linux 上。

## 一键构建与验收

~~~bash
cd ~/Embedded_System
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
cmake -E chdir build ctest --output-on-failure
~~~

Linux 上会生成六个目标，注册 17 项测试；Windows 上生成 std_thread 和 thread_sync，注册对应的 2 项测试。CMake 的配置输出会说明跳过了哪些 Linux 示例。

仅构建、不安装 Python 测试依赖时：

~~~bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=OFF
cmake --build build --parallel
~~~

之后要恢复测试，重新配置时传入 -DBUILD_TESTING=ON。

## 逐例运行

从项目根目录执行：

~~~bash
./build/std_thread
./build/pthread_demo
./build/thread_sync
./build/fork_demo
./build/exec_demo
./build/pipe_ipc
~~~

预期现象：

- std_thread 的两个 worker 各工作 3 次，另有一个 lambda 线程；最终输出 All threads finished.。创建后线程就可以开始运行，依次 join 不会把它们变成串行任务。不同线程的输出可能交错。
- pthread_demo 的两个线程各自填写结果，主线程 join 后输出 Thread 1 returned: 100, Thread 2 returned: 200。
- thread_sync 输出 20 次 Push 和 20 次 pop。0～9 各生产和消费两次，队列大小始终在 0～5 之间。哪个消费者取到哪个元素不固定。
- fork_demo 输出不同的父子 PID；父进程最后输出 child exited with status= 0。父子最初打印的先后顺序不固定。
- exec_demo 执行 PATH 中的 ls -l -a，列出当前工作目录，包括隐藏文件；父进程随后输出子进程状态。
- pipe_ipc 默认输出如下，随后父子进程均结束：

~~~text
[Child] received: Hello from parent process!
~~~

硬件并发数只是提示值，可能为 0。PID、线程 ID 和线程调度顺序都不应写死为测试预期。

### 管道消息参数与退出状态

~~~bash
./build/pipe_ipc "你好，Linux 管道"
./build/pipe_ipc ""
./build/pipe_ipc "带空格的消息"
echo $?
~~~

pipe_ipc 接受零个或一个消息参数。零参数使用默认消息；显式空字符串表示零字节消息，仍会输出前缀和换行。含空格或 shell 特殊字符时应正确引用参数。参数是文本，不能携带嵌入的 NUL 字节，并受操作系统命令行长度限制。

正常运行返回 0。系统调用或线程操作失败返回非零；pipe_ipc 参数过多返回 2。exec_demo 正常收集到子进程退出码时原样返回它；子进程被信号终止时，进程示例按 128 + 信号编号报告失败。

可以直接观察 exec 失败如何传播：

~~~bash
PATH=/nonexistent ./build/exec_demo
echo $?
~~~

应看到 execvp 错误和父进程报告的状态 1，最后的退出码也是 1。这行命令只修改这一次调用的 PATH。

## 不使用 CMake 的手动编译

从项目根目录执行：

~~~bash
mkdir -p build-manual
g++ -std=c++17 -Wall -Wextra -Wpedantic -g -O0 -pthread 01_thread/std_thread.cpp -o build-manual/std_thread
g++ -std=c++17 -Wall -Wextra -Wpedantic -g -O0 -pthread 01_thread/pthread_demo.cpp -o build-manual/pthread_demo
g++ -std=c++17 -Wall -Wextra -Wpedantic -g -O0 -pthread 01_thread/thread_sync.cpp -o build-manual/thread_sync
g++ -std=c++17 -Wall -Wextra -Wpedantic -g -O0 02_process/fork_demo.cpp -o build-manual/fork_demo
g++ -std=c++17 -Wall -Wextra -Wpedantic -g -O0 02_process/exec_demo.cpp -o build-manual/exec_demo
g++ -std=c++17 -Wall -Wextra -Wpedantic -g -O0 02_process/pipe_ipc.cpp -o build-manual/pipe_ipc
./build-manual/pipe_ipc
~~~

- -std=c++17 选择语言标准；-g -O0 便于逐行调试。
- -Wall -Wextra -Wpedantic 打开常用诊断，但不能保证发现数据竞争或业务逻辑错误。
- -pthread 同时处理目标平台需要的线程编译和链接设置；CMake 使用 Threads::Threads 表达同样的依赖。
- 六个文件各有 main，不能把它们一起链接为一个程序。

## 自动化测试检查什么

| 测试组 | 数量 | 验收内容 |
| --- | --- | --- |
| 标准线程与 pthread | 3 | 正常结束、返回值；同步队列的 FIFO、容量、40 次操作和逐线程数量 |
| fork | 1 | 父子 PID 关系、fork 前输出未重复、子进程退出状态 |
| exec | 4 | 当前目录和隐藏文件；找不到命令；子进程退出 7；被 SIGTERM 终止时返回 143 |
| pipe | 9 | 默认消息；0/127/128/129/4096/65537 字节；中文、空格、特殊字符与换行；错误参数 |

测试检查可观察行为，不依赖固定调度顺序。每个程序最多运行 8 秒，每项 CTest 有 10 秒总时限；超时属于失败。Linux 下测试驱动会终止该示例的整个进程组，避免留下 fork 出来的子进程。命令替身在临时目录中创建，不会替换系统的 ls。

重复检查不同线程交错时，可运行：

~~~bash
cmake -E chdir build ctest -R "^thread_sync$" --repeat-until-fail 20 --output-on-failure
~~~

### ThreadSanitizer

~~~bash
cmake -S . -B build-tsan -DCMAKE_BUILD_TYPE=Debug -DENABLE_TSAN=ON
cmake --build build-tsan --parallel
cmake -E chdir build-tsan ctest -L thread --output-on-failure
~~~

这会为三个线程示例启用 -fsanitize=thread；其他进程示例仍按普通方式构建。目标是观察执行期间是否发生数据竞争。

### AddressSanitizer 与 UndefinedBehaviorSanitizer

~~~bash
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN_UBSAN=ON
cmake --build build-asan --parallel
cmake -E chdir build-asan ctest --output-on-failure
~~~

这会为全部示例启用 -fsanitize=address,undefined，检查越界等内存问题和可检测的未定义行为。CTest 将 UBSan 配置为发现错误即失败。

两套 Sanitizer 必须使用分开的构建目录，不能同时开启两个选项。项目中的 Sanitizer 配置面向 Linux GCC/Clang；Windows 配置会明确拒绝这些选项。如果 Sanitizer 因运行环境不支持而无法启动，该次检查也不能记为通过。

## 调试与观察系统调用

~~~bash
gdb ./build/thread_sync
~~~

进入 GDB 后：

~~~text
break consumer
run
info threads
thread apply all bt
continue
~~~

进程与管道跟踪：

~~~bash
strace -f -e trace=%process ./build/exec_demo
strace -f -e trace=pipe,pipe2,read,write,close,wait4 ./build/pipe_ipc "hello"
~~~

-f 跟踪子进程；动态库加载也会产生 read/close 等调用，不是所有跟踪行都来自教学代码。

## Windows 上的有限验证

在配置好 MSVC、Windows SDK、CMake 和 Python 的开发者终端中：

~~~powershell
cmake -S . -B build-windows
cmake --build build-windows --config Debug
cmake -E chdir build-windows ctest -C Debug --output-on-failure
~~~

Windows 原生构建只验收两个标准 C++ 线程示例。若提示找不到 rc.exe 或 mt.exe，应使用 Visual Studio 开发者终端加载完整 Windows SDK 环境。Windows 测试通过不能代替 Linux 的 17 项测试和 Sanitizer 验收。

## 仍值得继续练习的内容

同步示例保留了固定工作量：两个生产者各生产 10 次，两个消费者各消费 10 次。修改数量时必须保持匹配，否则可能永久等待。下一步可以添加“生产结束”状态，让消费者在生产结束且队列为空时退出。

当前把同步示例的整行日志放在队列锁内，目的是让操作顺序清楚可验证；它会延长临界区，不适合作为吞吐量基准。进一步练习可以在锁内保存状态快照、锁外执行耗时工作，并分别设计日志顺序和队列同步。

## 参考资料

- [CMake FindThreads](https://cmake.org/cmake/help/latest/module/FindThreads.html)：跨平台线程依赖。
- [GCC 编译与链接选项](https://gcc.gnu.org/onlinedocs/gcc/Link-Options.html)：-pthread 等参数。
- [GCC Sanitizer 选项](https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html)：TSan、ASan 与 UBSan。
- [Linux man-pages](https://www.kernel.org/doc/man-pages/)：系统调用和 POSIX 接口语义。
