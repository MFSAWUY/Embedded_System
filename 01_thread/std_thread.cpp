// std_thread.cpp -- C++11 std::thread 基本用法
// 编译：g++ -std=c++17 std_thread.cpp -o std_thread -pthread
//
// 学习重点：
//        1. 线程是独立的执行流：共享进程的地址空间，但各自拥有栈和寄存器状态
//        2. std::thread 是标准 C++ 接口；Linux 上的实现通常使用 pthread，其他平台不一定如此
//        3. 仍可 join 的线程对象在析构前必须 join 或 detach，否则会调用 std::terminate()
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

    // 也可以使用 lambda 表达式作为线程入口（匿名函数作为入口）
    std::thread t3([]{
        std::cout << "lambda thread running" << std::endl;
    });

    // 三个线程已经启动；依次 join 只是在等待，不会让它们串行执行
    t1.join(); // 等待线程 t1 结束
    t2.join(); // 等待线程 t2 结束
    t3.join(); // 等待线程 t3 结束

    // 获取硬件并发数提示；不能保证等于可用 CPU 数，无法确定时可能返回 0
    std::cout << "Hardware concurrency: " << std::thread::hardware_concurrency() << std::endl;

    std::cout << "All threads finished." << std::endl;
    return 0;
}
