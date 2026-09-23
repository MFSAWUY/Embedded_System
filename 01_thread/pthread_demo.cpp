// pthread_demo.cpp -- POSIX 线程：参数、返回值和错误处理
// 编译：g++ -std=c++17 pthread_demo.cpp -o pthread_demo -pthread
//
// std::thread 是跨平台标准接口；pthread 是本例直接使用的 POSIX 接口。
// 入口函数的签名固定为 void* (*)(void*)，可通过指针传递一个结构体。
// pthread_create/pthread_join 直接返回错误编号，不能用 errno 代替它。
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <pthread.h>
#include <unistd.h>

struct ThreadData {
    int id;
    int result;
};

void* worker(void* arg) {
    auto* data = static_cast<ThreadData*>(arg);
    // pthread_t 是不透明类型，不能跨平台假定它是可打印的整数。
    std::cout << "pthread " << data->id << " running" << std::endl;
    sleep(1);  // 仅模拟工作耗时，不用于同步。
    data->result = data->id * 100;
    return data;  // 返回 main 中仍然存活的结构体地址，不把整数伪装成指针。
}

int main() {
    constexpr int THREAD_COUNT = 2;
    pthread_t threads[THREAD_COUNT]{};
    ThreadData data[THREAD_COUNT]{{1, 0}, {2, 0}};
    ThreadData* results[THREAD_COUNT]{};
    int created = 0;
    int exit_code = EXIT_SUCCESS;

    for (int i = 0; i < THREAD_COUNT; ++i) {
        const int error = pthread_create(&threads[i], nullptr, worker, &data[i]);
        if (error != 0) {
            std::cerr << "pthread_create: " << std::strerror(error) << std::endl;
            exit_code = EXIT_FAILURE;
            break;
        }
        ++created;
    }

    // 即使后续创建失败，也要等待已经启动的线程，保证 data 的生命周期。
    for (int i = 0; i < created; ++i) {
        void* returned = nullptr;
        const int error = pthread_join(threads[i], &returned);
        if (error != 0) {
            std::cerr << "pthread_join: " << std::strerror(error) << std::endl;
            // 无法确认线程已结束时，不能销毁它仍可能访问的栈上参数。
            // 诊断已刷新，直接结束整个进程；pthread_join 不返回 EINTR。
            std::_Exit(EXIT_FAILURE);
        }
        results[i] = static_cast<ThreadData*>(returned);
    }

    if (exit_code != EXIT_SUCCESS) {
        return exit_code;
    }

    // join 建立同步关系；这里读取工作线程写入的 result 无需额外的锁。
    std::cout << "Thread " << results[0]->id << " returned: " << results[0]->result
              << ", Thread " << results[1]->id << " returned: " << results[1]->result
              << std::endl;
    std::cout << "All threads finished." << std::endl;
    return EXIT_SUCCESS;
}
