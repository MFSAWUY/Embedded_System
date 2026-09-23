// std_thread.cpp -- C++11 std::thread 基本用法
// 编译：g++ -std=c++17 std_thread.cpp -o std_thread -pthread
//
// 学习重点：
//        1. 线程的本质 = 内核是一个独立的执行流，共享同一进程的内存，线程是内核调度的最小单位，线程之间共享进程的资源（如内存、文件描述符等），线程之间可以并发执行
//        2. std::thread 是 C++ 对底层 pthread 的封装，提供了更高层次的线程管理接口，使用起来更方便，用法更加现代
//        3. 必须 join(等待线程结束) 或 detach(分离线程) 否则线程对象析构时 std::terminate() 会被调用，程序会异常终止
#include <iostream>
#include <thread>
#include <chrono>

// 线程入口函数：
void worker(int id, int times){
    for(int i = 0; i < times; ++i){
        std::cout << "std::thread " << id << " working " << i+1 << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 模拟线程工作，休眠 100 毫秒
    }
}

int main() {
    std::cout << "Main thread id: " << std::this_thread::get_id() << std::endl;

    // 创建两个线程，分别执行 worker 函数，传递函数 + 参数
    std::thread t1(worker, 1, 3); // 创建线程 t1，执行 worker(1, 3)
    std::thread t2(worker, 2, 3); // 创建线程 t2，执行 worker(2, 3)

    // 也可以使用 lammbda 表达式作为线程入口（匿名函数作为入口）
    std::thread t3([]{
        std::cout << "lambda thread running" << std::endl;
    });

    // join 阻塞当前线程，等待指定线程结束
    t1.join(); // 等待线程 t1 结束
    t2.join(); // 等待线程 t2 结束
    t3.join(); // 等待线程 t3 结束

    // 获取硬件并发数（逻辑 CPU 核数，常用于决定线程池大小）
    std::cout << "Hardware concurrency: " << std::thread::hardware_concurrency() << std::endl;

    std::cout << "All threads finished." << std::endl;
    return 0;
}