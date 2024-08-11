#pragma once

#include <condition_variable>
#include <mutex>

#include <tarp/cxxcommon.hxx>

namespace tarp {

/* Semaphore implementation -- only useful pre c++20, when semaphores are
 * introduced in the c++ stdlib.
 * NOTE: set max_count=1 if you need binary semaphore semantics (or better, use
 * the binary_semaphore alias).
 */
template<std::uint32_t max_count = std::numeric_limits<std::uint32_t>::max()>
class semaphore {
public:
    DISALLOW_COPY_AND_MOVE(semaphore<max_count>);

    /* counter initialized to 0 (i.e. semaphore=locked) by default. */
    semaphore() : m_counter(0), m_max_count(max_count) {}

public:
    // reset semaphore to 0
    void reset() {
        std::unique_lock l {m_mtx};
        m_counter = 0;
    }

    // Signal the semaphore. Iff the internal counter has not reached
    // the maximum set value, then the counter is incremented as well.
    void release() {
        std::unique_lock l {m_mtx};

        /* this has binary semaphore semantics when m_max_count=2 */
        if (m_counter < m_max_count) {
            m_counter++;
        }
        m_condvar.notify_all();
    }

    // Block until the semaphore is signaled AND the internal counter is
    // non-zero.
    void acquire() {
        std::unique_lock l(m_mtx);

        while (m_counter == 0) {
            m_condvar.wait(l);
        }
        --m_counter;
    }

    // Try to decrement the internal counter; return immediately.
    // True if successful, False if failed (==> counter is 0)
    bool try_acquire() {
        std::unique_lock l {m_mtx};

        if (m_counter > 0) {
            --m_counter;
            return true;
        }
        return false;
    }

    template<typename timepoint>
    bool try_acquire_until(const timepoint &abs_time) {
        std::unique_lock l(m_mtx);

        m_condvar.wait_until(l, abs_time, [this] {
            return m_counter > 0;
        });

        if (m_counter > 0) {
            --m_counter;
            return true;
        }
        return false;
    }

    template<class Rep, class Period>
    bool try_acquire_for(const std::chrono::duration<Rep, Period> &rel_time) {
        return try_acquire_until(std::chrono::steady_clock::now() + rel_time);
    }

private:
    std::mutex m_mtx;
    std::condition_variable m_condvar;
    uint32_t m_counter;
    const std::uint32_t m_max_count;
};

using binary_semaphore = semaphore<1>;

}  // namespace tarp
