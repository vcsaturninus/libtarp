#include <tarp/sched.hxx>

namespace tarp::sched {
using namespace std;

using T = interval_task_mixin;

interval_task_mixin::interval_task_mixin(
  std::chrono::milliseconds interval,
  std::optional<std::size_t> max_num_expirations,
  bool starts_expired)
    : m_interval(interval)
    , m_max_num_expirations(max_num_expirations)
    , m_num_expirations(0)
    , m_start_expired(starts_expired) {
    if (starts_expired) {
        m_next_deadline = time_now();
    } else {
        m_next_deadline = time_now() + interval;
    }
}

/* True if we are at or past the deadline, else False */
bool T::expired() const {
    return time_now() >= m_next_deadline;
}

std::chrono::system_clock::time_point T::get_expiration_time() const {
    return m_next_deadline;
}

void T::delay(std::chrono::microseconds delay) {
    m_next_deadline += delay;
}

bool T::renewable() const {
    // no upper limit to the number of times we can expire.
    if (!m_max_num_expirations.has_value()) {
        return true;
    }

    // an upper limit, but not yet reached
    if (m_max_num_expirations.value() > m_num_expirations) {
        return true;
    }

    // an upper limit, already reached
    return false;
}

void T::renew() {
    if (!expired()) {
        throw std::logic_error("Illegal attempt to renew before deadline");
    }

    if (!renewable()) {
        throw std::logic_error("Illegal attempt to renew un-renewable item");
    }

    // m_next_deadline has expired; find out the next deadline.
    auto current_deadline = get_expiration_time();
    auto now = time_now();

    // if we are far behind, bring up to date so the expiration is in the
    // future.
    if (current_deadline + m_interval > now) {
        m_next_deadline = current_deadline + m_interval;
    } else {
        m_next_deadline = now + m_interval;
    }

    m_num_expirations++;
}

void T::set_expiration(std::chrono::system_clock::time_point deadline) {
    m_next_deadline = deadline;
}

std::chrono::system_clock::time_point T::time_now() const {
    return std::chrono::system_clock::now();
}

std::chrono::milliseconds T::get_interval() const {
    return m_interval;
}

bool T::starts_expired() const {
    return m_start_expired;
}

std::optional<std::size_t> T::get_max_num_expirations() const {
    return m_max_num_expirations;
}


}  // namespace tarp::sched
