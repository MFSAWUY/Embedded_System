// exec_demo.cpp -- fork + exec：在子进程中运行另一个程序
// 编译：g++ -std=c++17 exec_demo.cpp -o exec_demo
//
// exec 替换当前进程的程序映像，不创建新进程；PID 和部分进程属性保持不变。
// 成功的 exec 不返回；只有失败时才返回 -1 并设置 errno。
// execvp 的 v 表示参数数组，p 表示按 PATH 查找不含 '/' 的程序名。
#include <cerrno>
#include <cstdio>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

int main() {
    const pid_t pid = fork();
    if (pid == -1) {
        std::perror("fork");
        return 1;
    }
    if (pid == 0) {
        // 使用可写字符数组满足接口类型，避免强制去除字符串字面量的 const。
        char program[] = "ls";
        char long_option[] = "-l";
        char all_option[] = "-a";
        char* args[] = {program, long_option, all_option, nullptr};
        execvp(program, args);  // 列出当前工作目录，而不是可执行文件所在目录。

        std::perror("execvp");
        _exit(1);  // exec 失败；不刷新继承的 stdio 缓冲区，也不执行 atexit 回调。
    }

    int status = 0;
    pid_t waited;
    do {
        waited = waitpid(pid, &status, 0);
    } while (waited == -1 && errno == EINTR);

    if (waited == -1) {
        std::perror("waitpid");
        return 1;
    }
    if (WIFEXITED(status)) {
        const int exit_code = WEXITSTATUS(status);
        std::cout << "[Parent] child (running ls) finished with status= "
                  << exit_code << std::endl;
        return exit_code;  // 让 shell 和测试也能观察到子进程的失败。
    }
    if (WIFSIGNALED(status)) {
        const int signal_number = WTERMSIG(status);
        std::cerr << "[Parent] child terminated by signal= " << signal_number << std::endl;
        return 128 + signal_number;
    }
    std::cerr << "[Parent] unexpected child status" << std::endl;
    return 1;
}
