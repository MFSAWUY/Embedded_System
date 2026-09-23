// fork_demo.cpp -- 进程：fork() 创建子进程
// 编译：g++ -std=c++17 fork_demo.cpp -o fork_demo
//
// 学习重点：
//        1. fork 一次调用，返回两次 -- 这是 fork 的关键；
//           父进程里返回：子进程的 PID （>0）
//           子进程里返回： 0
//        2. 子进程从 fork 之后继续执行，拥有父进程的内存空间的副本（虚拟内存），但父子进程的内存空间是独立的，互不影响
//        3. 父进程必须 wait 子进程结束，否则子进程会成为僵尸进程（Zombie Process），占用系统资源
#include <iostream>
#include <unistd.h>     // fork(), getpid(), getppid()
#include <sys/wait.h>   // waitpid()

int main() {
    // getpid() 获取当前进程的 PID，getppid() 获取父进程的 PID
    std::cout << "[before fork] PID= " << getpid() << std::endl;

    // fork() 创建子进程，得到父子进程的 PID
    // 返回值是区分父子进程的依据
    pid_t pid = fork(); 

    if (pid < 0) {
        perror("fork");
        return 1;
    } else if (pid == 0) {
        // ---- 子进程 ----
        std::cout << "[Child] PID= " << getpid() 
                  << ", Parent PID= " << getppid() << std::endl;  // 打印父进程的 PID
        // 子进程可以执行自己的任务，这里可以 exec 成别的程序，见 exec_demo.cpp
    } else {
        // ---- 父进程 ---- 父进程里 PID 的值是子进程的 PID ，子进程里 PID 是 0
        std::cout << "[Parent] child PID= " << pid << std::endl;

        // 父进程等待子进程结束，避免僵尸进程
        int status = 0;
        waitpid(pid, &status, 0); // 等待子进程结束，避免僵尸进程

        // WIFEXITED(status) 判断子进程是否正常退出(还是被信号终止)。WIFEXITED = "Wait IF EXITEed"。
        // WEXITSTATUS(status) 获取子进程的退出状态码（就是子进程 return 的值）。
        if (WIFEXITED(status)) {
            std::cout << "[Parent] child exited with status= " << WEXITSTATUS(status) << std::endl;
        } else {
            std::cout << "[Parent] child terminated abnormally." << std::endl;
        }
    }

    return 0;
}
