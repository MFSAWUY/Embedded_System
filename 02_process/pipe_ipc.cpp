// pipe_ipc.cpp -- 匿名管道：父进程写，子进程读
// 编译：g++ -std=c++17 pipe_ipc.cpp -o pipe_ipc
// 运行：./pipe_ipc [message]，无参数时发送默认消息；空字符串也是有效消息。
//
// pipefd[0] 是读端，pipefd[1] 是写端；本例通过 fork 继承这两个文件描述符。
// 管道是字节流，没有消息边界；一次 write 不一定对应一次 read。
// 阻塞模式下，空管道仍有写端时 read 等待；所有写端关闭且数据读完才返回 EOF。
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

bool close_fd(int fd) {
    if (close(fd) == -1) {
        std::perror("close");
        // Linux 即使在 close 返回 EINTR 时也已释放描述符，不要重试关闭。
        return false;
    }
    return true;
}

bool write_all(int fd, const char* message, std::size_t length) {
    std::size_t offset = 0;
    while (offset < length) {
        const ssize_t written = write(fd, message + offset, length - offset);
        if (written > 0) {
            offset += static_cast<std::size_t>(written);
        } else if (written == -1 && errno == EINTR) {
            continue;
        } else {
            if (written == -1) {
                std::perror("write");
            } else {
                std::cerr << "write: no progress" << std::endl;
            }
            return false;
        }
    }
    return true;
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [message]" << std::endl;
        return 2;
    }
    const char* message = argc == 2 ? argv[1] : "Hello from parent process!";

    // 读端意外关闭时，让 write 返回 EPIPE，便于报告错误并回收子进程。
    struct sigaction action {};
    action.sa_handler = SIG_IGN;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGPIPE, &action, nullptr) == -1) {
        std::perror("sigaction");
        return 1;
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        std::perror("pipe");
        return 1;
    }

    const pid_t pid = fork();
    if (pid == -1) {
        std::perror("fork");
        close_fd(pipefd[0]);
        close_fd(pipefd[1]);
        return 1;
    }

    if (pid == 0) {
        // 子进程不写入，必须关闭它继承的写端，否则自己也会阻止 EOF。
        if (!close_fd(pipefd[1])) {
            close_fd(pipefd[0]);
            return 1;
        }

        int exit_code = 0;
        char buffer[128];
        std::cout << "[Child] received: ";
        for (;;) {
            const ssize_t received = read(pipefd[0], buffer, sizeof(buffer));
            if (received > 0) {
                // 按长度输出，读满 128 字节也不需要额外写入 '\0'。
                std::cout.write(buffer, static_cast<std::streamsize>(received));
                if (!std::cout) {
                    std::cerr << "stdout: output failed" << std::endl;
                    exit_code = 1;
                    break;
                }
            } else if (received == 0) {
                std::cout << std::endl;
                if (!std::cout) {
                    std::cerr << "stdout: output failed" << std::endl;
                    exit_code = 1;
                }
                break;
            } else if (errno != EINTR) {
                std::perror("read");
                exit_code = 1;
                break;
            }
        }
        if (!close_fd(pipefd[0])) {
            exit_code = 1;
        }
        return exit_code;
    }

    int exit_code = close_fd(pipefd[0]) ? 0 : 1;
    if (!write_all(pipefd[1], message, std::strlen(message))) {
        exit_code = 1;
    }
    // 必须先关闭写端再 waitpid，否则子进程可能在等 EOF，而父进程在等子进程。
    if (!close_fd(pipefd[1])) {
        exit_code = 1;
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
        return exit_code != 0 ? exit_code : WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        const int signal_number = WTERMSIG(status);
        std::cerr << "[Parent] child terminated by signal= " << signal_number << std::endl;
        return 128 + signal_number;
    }
    std::cerr << "[Parent] unexpected child status" << std::endl;
    return 1;
}
