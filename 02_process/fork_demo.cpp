// fork_demo.cpp -- fork 创建子进程，waitpid 等待并回收子进程
// 编译：g++ -std=c++17 fork_demo.cpp -o fork_demo
//
// fork 成功后：父进程得到子进程 PID，子进程得到 0；失败时只有父进程返回 -1。
// 父子进程通常拥有独立的地址空间；继承的文件描述符仍可能引用同一个内核对象。
// 在本例的默认信号处理方式下，父进程通过 waitpid 回收子进程，避免留下僵尸。
#include <cerrno>
#include <cstdio>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

int main() {
    // std::endl 在 fork 前刷新输出，避免缓冲内容被父子进程重复输出。
    std::cout << "[before fork] PID= " << getpid() << std::endl;
    const pid_t pid = fork();

    if (pid == -1) {
        std::perror("fork");
        return 1;
    }
    if (pid == 0) {
        std::cout << "[Child] PID= " << getpid()
                  << ", Parent PID= " << getppid() << std::endl;
        return 0;
    }

    std::cout << "[Parent] child PID= " << pid << std::endl;
    int status = 0;
    pid_t waited;
    do {
        waited = waitpid(pid, &status, 0);
    } while (waited == -1 && errno == EINTR);

    if (waited == -1) {
        std::perror("waitpid");
        return 1;
    }

    // 仅在等待成功后解释 status，且先判断退出类别，再读取对应字段。
    if (WIFEXITED(status)) {
        const int exit_code = WEXITSTATUS(status);
        std::cout << "[Parent] child exited with status= " << exit_code << std::endl;
        return exit_code;
    }
    if (WIFSIGNALED(status)) {
        const int signal_number = WTERMSIG(status);
        std::cerr << "[Parent] child terminated by signal= " << signal_number << std::endl;
        return 128 + signal_number;
    }
    std::cerr << "[Parent] unexpected child status" << std::endl;
    return 1;
}
