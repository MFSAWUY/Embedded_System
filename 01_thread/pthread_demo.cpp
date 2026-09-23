// pthread_demo.cpp -- POSIX 线程底层 API
// 编译：g++ -std=c++17 pthread_demo.cpp -o pthread_demo -pthread
//
// 学习重点：
//        1.std::thread 底层就是封装了 pthread， 学习这个能看清线程的本质
//        2.线程函数签名固定为 void* (*)(void*), 也就是返回值为 void*，参数为 void* 的函数指针，参数/返回值都靠 void* 来传递
//        3.pthread_create() 创建线程，pthread_join() 等待线程结束并回收资源、获取线程返回值
#include <iostream>
#include <pthread.h>
#include <unistd.h>

// 线程入口函数：必须返回 void*，参数也为 void*，参数/返回值都靠 void* 来传递（使用 pthread 创建线程入口函数参数固定）
void* worker(void* arg){
    int id = *(int*)arg;                        // 传入的参数是 void*，需要强制类型转换为 int*，再解引用获取值，从 void* 里取出参数
    std::cout << "pthread " << id << " running, tid="
              << pthread_self() << std::endl;   // pthread_self() 获取线程 ID
    sleep(1); // 模拟线程工作，休眠 1 秒
    return (void*)(long)(id * 100);             // 返回值也为 void*，这里返回 id*100，强制类型转换为 void*
}

int main() {
    std::cout << "Main thread id: " << pthread_self() << std::endl;

    pthread_t t1, t2; // 线程 ID
    int arg1 = 1, arg2 = 2; // 线程参数

    // 创建两个线程，分别执行 worker 函数，传递函数 + 参数
    // 参数：线程句柄 / 属性（默认 nullptr）/ 线程入口函数 / 线程入口函数参数
    pthread_create(&t1, nullptr, worker, &arg1); // 创建线程 t1，执行 worker(&arg1)
    pthread_create(&t2, nullptr, worker, &arg2); // 创建线程 t2，执行 worker(&arg2)

    // pthread_join 阻塞当前线程，等待指定线程结束，并通过第二参数获取返回值
    void* ret1;
    void* ret2;
    // join 阻塞当前线程，等待指定线程结束，并获取返回值
    pthread_join(t1, &ret1); // 等待线程 t1 结束，并获取返回值
    pthread_join(t2, &ret2); // 等待线程 t2 结束，并获取返回值

    std::cout << "Thread " << arg1 << " returned: " << (long)ret1
              << ", Thread " << arg2 << " returned: " << (long)ret2 << std::endl;

    std::cout << "All threads finished." << std::endl;
    return 0;
}