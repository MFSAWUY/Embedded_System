// exec_demo.cpp -- exec：用新程序替换当前进程的映像（fork + exec 经典组合）
// 编译：g++ -std=c++17 exec_demo.cpp -o exec_demo
//
// 学习重点：
//        1. exec 不会创建新进程，而是“替换”当前进程的映像，当前进程的代码、数据、堆栈都会被新程序替换掉，只有 PID 不变
//        2. exec() 函数在调用成功后不会执行并返回，只有在调用失败时才会返回 -1
//        3. 典型组合：父进程 fork() 创建子进程，子进程 exec() 执行新程序，父进程 wait() 等待子进程结束
//                    （shell 执行命令、网络服务器 fork() 子进程处理请求，都是这个套路）
//        4. exec() 函数有多个变体，如 execl(), execv(), execle(), execve(), execvp(), execvpe()
#include <iostream>
#include <unistd.h>     // exec()
#include <sys/wait.h>   // waitpid()

int main() {
    pid_t pid = fork(); // fork() 创建子进程
    if (pid == 0) {
        // ---- 子进程：用 execvp 执行 /bin/ls -l -a命令 ----
        // 参数形式：程序名 + 参数列表（以 nullptr 结尾）
        char* args[] = {(char*)"ls", (char*)"-l", (char*)"-a", nullptr};
        execvp("ls", args); // execvp() 执行 ls 命令，替换当前进程映像

        // 走到这里说明 exec() 调用失败，打印错误信息（比如找不到 ls）
        perror("execvp");
        _exit(1); // 退出子进程，避免继续执行父进程的代码，注意用 _exit()，不刷新 stdio 缓冲区
    } else {
        // ---- 父进程 ---
        // 父进程等待子进程结束，避免僵尸进程
        int status;
        waitpid(pid, &status, 0); // 等待子进程结束，避免僵尸进程

        if (WIFEXITED(status)) {
            std::cout << "[Parent] child (running ls) finished with status= " << WEXITSTATUS(status) << std::endl;
        } else {
            std::cout << "[Parent] child terminated abnormally." << std::endl;
        }
    }

    return 0;

    // 使用 execl() 替换当前进程的映像，执行 ls 命令
    // execl("/bin/ls", "ls", "-l", nullptr);
    // 使用 execv() 替换当前进程的映像，执行 ls 命令
    // char *argv[] = {"/bin/ls", "-l", nullptr};
    // execv("/bin/ls", argv);
    // 使用 execle() 替换当前进程的映像，执行 ls 命令
    // char *env[] = {nullptr};
    // execle("/bin/ls", "ls", "-l", nullptr, env);
    // 使用 execve() 替换当前进程的映像，执行 ls 命令
    // char *argv[] = {"/bin/ls", "-l", nullptr};
    // char *env[] = {nullptr};
    // execve("/bin/ls", argv, env);
    // 使用 execvp() 替换当前进程的映像，执行 ls 命令
    // char *argv[] = {"ls", "-l", nullptr};
    // execvp("ls", argv);

    // 如果exec()调用失败，会执行到这里
    // perror("exec");
    // return 1;
}
