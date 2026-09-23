// thread_sync.cpp -- C++11 线程同步：mutex + condition_variable (生产者-消费者模型)
// 编译：g++ -std=c++17 thread_sync.cpp -o thread_sync -pthread
//
// 学习重点：（这是线程池和所有同步逻辑的地基）
//        1. mutex 保证同一时刻只有一个线程访问临界区，即共享资源
//        2. condition_variable 让线程“等待某个条件成立”，而不是空转轮询，用于线程间的等待和通知
//        3. wait(lock, pred) 的语义：
//             1) 如果 pred() 返回 false：自动释放锁并挂起；线程进入等待状态，直到被 notify 唤醒，被唤醒后重新获取锁并检查 pred()，如果 pred() 仍然返回 false，则继续等待
//             2) 如果 pred() 返回 true：立即返回（已持锁），线程继续执行，不会进入等待状态
//        4. notify_one() 唤醒一个等待线程，notify_all() 唤醒所有等待线程
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>

std::queue<int> q;                               // 共享数据（队列）
std::mutex mtx;                                  // 互斥锁，保护共享数据，保护队列的锁
std::condition_variable cv;                      // 条件变量，用于线程间的等待和通知
constexpr int MAX_QUEUE_SIZE = 5;                // 队列容量上限

// 生产者线程函数：生产数据并放入队列，队列满时阻塞
void producer(int id) {
    for (int i = 0; i < 10; ++i) {
        std::unique_lock<std::mutex> lock(mtx);
        // 等待队列没满，满则自动解锁挂起，被 notify 唤醒后重新获取锁并检查条件
        cv.wait(lock, []{ return q.size() < MAX_QUEUE_SIZE; });
        
        q.push(i);
        std::cout << "[Producer " << id << "]: Push " << i 
                  << ", size=" << q.size() << std::endl;
        
        lock.unlock();
        cv.notify_all();                          // 通知等待中的消费者“有货了”
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

// 消费者线程函数：消费数据并从队列取出，队列空时阻塞
void consumer(int id) {
    for(int i = 0; i < 10; ++i) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, []{ return !q.empty(); });  // 等待队列非空，空则自动解锁挂起，被 notify 唤醒后重新获取锁并检查条件

        int value = q.front();
        q.pop();
        lock.unlock();
        std::cout << "[Consumer " << id << "]: pop " << value 
                  << ", size=" << q.size() << std::endl;
        
        lock.unlock();
        cv.notify_all();                          // 通知等待中的生产者“有空位了”
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
    }
}

int main() {
    std::thread p1(producer, 1), p2(producer, 2);
    std::thread c1(consumer, 1), c2(consumer, 2);

    p1.join();
    p2.join();
    c1.join();
    c2.join();

    return 0;
}