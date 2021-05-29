
#include <unistd.h>
#include <iostream>
#include <random>
#include <thread>
#include <vector>
#include "taskqueue.h"
#include "countdownlatch.h"

struct Task;

std::vector<std::thread> g_task_threads;
cc::TaskQueue<Task> g_task_queue;
bool g_running_flag = true;

struct AsyncContext {
    cc::CountDownLatch* count_latch;
    unsigned result;
    int32_t task_id;
};

struct Task {
    std::shared_ptr<AsyncContext> real_task;
};

void* task_proc(Task* task) {
    auto context = task->real_task;

    // 随机数, 序列是一致的，所以使用static, 让不同的线程之间共用状态
    static std::uniform_int_distribution<unsigned> u(0,9);
    static std::default_random_engine e;
    context->result = u(e);

    std::cout << "task:" << context->task_id << ", sleep:" << context->result << std::endl;;
    sleep(context->result);
    std::cout << "task:" << context->task_id << ", sleep end" << std::endl;;
    context->count_latch->down();
    return nullptr;
}

void* task_dispatcher(void*) {
    Task task;
    while (g_running_flag || !g_task_queue.empty()) {
        auto status = g_task_queue.pop(&task, 1000000);
        if (status == ::cc::PopStatus::kOk) {
            task_proc(&task);
        }
    }
    return nullptr;
}

int main() {
    uint32_t task_thread_num = 10;
    for (int i = 0; i < task_thread_num; ++i) {
        g_task_threads.push_back(std::thread(task_dispatcher, nullptr));
    }

    int task_num = 5;
    cc::CountDownLatch count_latch(5);
    std::vector<Task> task_list;
    for (int i = 0; i < task_num; ++i) {
        Task task;
        auto real_task = std::make_shared<AsyncContext>();
        real_task->count_latch = &count_latch;
        real_task->result = 0;
        real_task->task_id= i;
        task.real_task = real_task;
        task_list.push_back(task);
        g_task_queue.push(task);
    }

    count_latch.wait();

    for (auto& task: task_list) {
        auto& real_task = task.real_task;
        std::cout << "taskid=" << real_task->task_id << ", result=" << real_task->result << std::endl;
    }

    g_running_flag = false;
    for (auto& t: g_task_threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    return 0;
}
