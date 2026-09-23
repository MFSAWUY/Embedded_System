// pipe_ipc.cpp -- 管道：IPC（进程间通信）
// 编译：g++ -std=c++17 pipe_ipc.cpp -o pipe_ipc
//
// 学习重点：
//        1. pipe(fd) 是一种“半双工”的通信方式，数据只能单向流动：fd[0] 用于读，fd[1] 用于写，数据从 fd[1] 写入，从 fd[0] 读取
//        2. 管道本质是一个内核中的缓冲区，进程通过 read() 和 write() 操作管道，写入的数据从另一端按顺序读出
//        3. 管道只能在具有公共祖先的进程间使用，通常是父子进程，即进程见内存是隔离的，想通信必须借助内核提供的机制（管道/共享内存/信号等）
//        4. 管道有容量限制，当管道满时，write() 会阻塞；当管道空时，read() 会阻塞
//        5. 常见用法：父进程创建管道，fork() 创建子进程，父进程写入数据，子进程读取数据，即 fork 后父子进程各管一端，一个写一个读
#include <iostream>
#include <cstring>      // strlen()
#include <unistd.h>     // pipe(), read(), write()
#include <sys/wait.h>   // waitpid()

int main() {
    int pipefd[2]; // 管道文件描述符数组

    if (pipe(pipefd) <= 0) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();

    if (pid == 0) {
        // ---- 子进程：读 ----
        close(pipefd[1]); // 关闭自己已用不到的写端

        char buffer[128];
        ssize_t bytes_read = read(pipefd[0], buffer, sizeof(buffer));
        while (bytes_read > 0) {
            buffer[bytes_read] = '\0'; // 确保字符串以 '\0' 结尾
            std::cout << "[Child] received: " << buffer << std::endl;
        }

        close(pipefd[0]); // 关闭读端
    } else {
        // ---- 父进程 ----
        close(pipefd[0]); // 关闭读端

        const char* message = "Hello from parent process!";
        write(pipefd[1], message, strlen(message));

        close(pipefd[1]); // 关闭写端后，子进程才能读到 EOF(0)，read() 才会返回 0

        waitpid(pid, nullptr, 0); // 等待子进程结束
    }

    return 0;
}