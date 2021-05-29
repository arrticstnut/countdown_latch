/*
 *
 * @file task_queue.h
 *
*/
#ifndef _CC_TASK_QUEUE_H__
#define _CC_TASK_QUEUE_H__

#include <assert.h>
#include <stdint.h>
#include <mutex>
#include <condition_variable>
#include <queue>

namespace cc {

enum class PushStatus {
    kOk = 0,
    kFull
};
enum class PopStatus {
    kOk = 0,
    kEmpty,
    kTimeout
};
template<typename T>
class TaskQueue {
public:
    /**
     * brief: 有锁任务队列
     * param:
     *      max_num : -int 任务队列的最大长度, -1(默认) 为不限制
     */
    explicit TaskQueue(int max_num = -1);
    ~TaskQueue();
    /**
     * brief: 向队列末尾添加任务
     * return:
     *  PushStatus::kOk  - 添加成功
     *  PushStatus::kFull  - 添加失败,队列满
     */
    PushStatus push(T);
    /**
     * brief: 从队列中获取任务,如无任务则阻塞等待
     * param:
     *  T*  - output
     *  timout_us  - 阻塞等待的最大时间，-1(默认)表示永久阻塞,直到有新任务
     * return:
     *  PopStatus::kOk  - 获取任务成功
     *  PopStatus::kTimeout  - 等待新任务超时
     */
    PopStatus pop(T*, int64_t timout_us=-1);
    /**
     * brief: 从队列中获取任务, 如无任务则直接返回
     * param:
     *  T*  - output
     * return:
     *  PopStatus::kOk  - 获取任务成功
     *  PopStatus::kEmpty  - 任务队列为空
     */
    PopStatus pop_nonblock(T*);
    /**
     * brief: 队列是否为空
     */
    bool empty() { return _task_count == 0; }
    /**
     * brief: 获取队列中任务数量
     */
    int size() { return static_cast<int>(_task_count); }
private:
    TaskQueue(const TaskQueue<T>&) = delete;
    TaskQueue<T>& operator=(const TaskQueue<T>&) = delete;
protected:
    volatile int _task_count = 0;
    int _max_task_num;
    std::queue<T> _queue;
    std::mutex _lock;
    std::condition_variable _cond;
};
template<typename T>
TaskQueue<T>::TaskQueue(int max_num) : _max_task_num(max_num) {}
template<typename T>
TaskQueue<T>::~TaskQueue() {}
template<typename T>
inline PushStatus TaskQueue<T>::push(T task) {
    std::unique_lock<std::mutex> locker(_lock);
    if (_max_task_num > 0 && _task_count >= _max_task_num) {
        return PushStatus::kFull;
    }
    _queue.push(task);
    ++_task_count;
    _cond.notify_one();
    return PushStatus::kOk;
}
template<typename T>
inline PopStatus TaskQueue<T>::pop(T* task, int64_t timeout_us) {
    std::unique_lock<std::mutex> locker(_lock);
    if (timeout_us > 0) {
        bool ok = _cond.wait_for(locker,
                std::chrono::microseconds(timeout_us),
                [this]{return !_queue.empty();});
        if (!ok) {
            return PopStatus::kTimeout;
        }
    } else {
        _cond.wait(locker, [this]{return !_queue.empty();});
    }
    *task = _queue.front();
    _queue.pop();
    --_task_count;
    return PopStatus::kOk;
}
template<typename T>
inline PopStatus TaskQueue<T>::pop_nonblock(T* task) {
    std::unique_lock<std::mutex> locker(_lock);
    if (_queue.empty()) {
        return PopStatus::kEmpty;
    }
    *task = _queue.front();
    _queue.pop();
    --_task_count;
    return PopStatus::kOk;
}

} // end of namespace

#endif
