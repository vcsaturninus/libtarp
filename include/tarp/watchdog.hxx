#ifndef TARP_WATCHDOG_HXX_
#define TARP_WATCHDOG_HXX_

#include <chrono>
#include <functional>

#include <tarp/thread_entity.hxx>

namespace tarp {

using wd_guard_cb_t = std::function<bool(void)>;
using wd_action_cb_t = std::function<void(void)>;

/*
 * Simple watchdog class running in its own thread.
 *
 * The watchdog can run in two modes (see constructors below):
 *
 * 1. guarded mode
 * The user must provide a guard callback. At the end of every
 * lapsed interval, the watchdog will invoke the guard. If
 * it returns true, the watchdog resets itself. If it returns
 * false, the watchdog will invoke the action callback and then
 * pause itself. The user need not explicitly reset the watchdog
 * in this case.
 *
 * 2. unguarded mode
 * At the end of every lapsed interval, the watchdog is triggered
 * -- 'bites' -- and will invoke the action callback, and then
 * pause itself. The user must explicitly call reset() to prevent
 * the watchdog from triggering.
 *
 * NOTE:
 * A paused watchdog can be resumed via another call to run().
 * When this happens, the watchdog implicitly resets itself as well
 * so that it is not instantlly triggered again.
 */
template<typename T = std::chrono::seconds>
class Watchdog : public tarp::ThreadEntity {
public:
    /* guarded-mode watchdog constructor */
    Watchdog(T interval, wd_action_cb_t action, wd_guard_cb_t guard)
        : m_guard_mode(true)
        , m_interval(interval)
        , m_action_cb(action)
        , m_guard(guard) {}

    /* unguarded-mode watchdog constructor */
    Watchdog(T interval, wd_action_cb_t action)
        : m_guard_mode(false)
        , m_interval(interval)
        , m_action_cb(action)
        , m_guard() {}

    /* Reset the watchdog so that it waits another full interval
     * before it 'bites'. I.e. keep the watchdog at bay. */
    bool reset(void);

    /* Called either manually or automatically on failure to
     * reset the watchdog.
     * This in turn calls the user-supplied action callback.
     * The watchdog will be paused after this and can be resumed
     * (assuming the process is not exited) with run().
     * When that is done, the watchdog is implicitly reset as well
     * so that it doesn't instantly trigger again. */
    void bite(void);

private:
    void initialize(void) override;
    void prepare_resume(void) override;
    void do_task(void) override;

    bool m_guard_mode;
    T m_interval;
    std::chrono::steady_clock::time_point m_deadline;
    wd_action_cb_t m_action_cb;
    wd_guard_cb_t m_guard;
};

template<typename T>
bool Watchdog<T>::reset() {
    // printf("****** WATCHDOG RESET\n");
    initialize();
}

template<typename T>
void Watchdog<T>::initialize() {
    m_deadline = std::chrono::steady_clock::now() + m_interval;
}

template<typename T>
void Watchdog<T>::prepare_resume() {
    initialize();
}

template<typename T>
void Watchdog<T>::bite() {
    // printf("****** WATCHDOG TRIGGERED\n");
    m_action_cb();
    set_state(PAUSED);
}

template<typename T>
void Watchdog<T>::do_task(void) {
    /* NOTE: this may be updated by the user in a separate thread
     * by calling reset(). If we miss it, we'll get it in the next
     * pass, so no harm done, and no locking necessary. */
    if (std::chrono::steady_clock::now() < m_deadline) {
        std::this_thread::sleep_for(m_interval);
        return;
    }

    if (m_guard_mode) {
        if (m_guard()) {
            return;
        }
    }

    bite();
}

}  // namespace tarp

#endif
