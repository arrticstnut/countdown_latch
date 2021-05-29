/**
 * @file thread_utils.h
 *
 **/
#ifndef __CC_COUNT_DOWN_LATCH_H__
#define __CC_COUNT_DOWN_LATCH_H__

#include <atomic>
#include <mutex>
#include <condition_variable>

namespace cc {
class CountDownLatch {
public:

    CountDownLatch(int32_t cnt) {
        m_cnt.store(cnt);
    }

    CountDownLatch() {
        m_cnt.store(0);
    }

    void operator--() {
        --m_cnt;
    }
    void operator++() {
        ++m_cnt;
    }
    void reset(int32_t cnt) {
        m_cnt.store(cnt);
    }
    void wait() {
        std::unique_lock<std::mutex> lk(m_mtx);
        if (m_cnt.load() == 0) {
            lk.unlock();
            return;
        }
        m_cv.wait(lk);
        lk.unlock();
    }
    void down() {
        std::unique_lock<std::mutex> lk(m_mtx);
        --m_cnt;
        lk.unlock();
        if (m_cnt.load() == 0) {
            m_cv.notify_one();
        }
    }
private:
    std::atomic<int32_t> m_cnt {0};
    std::condition_variable m_cv;
    std::mutex m_mtx;
};

} // end of namespace

#endif
