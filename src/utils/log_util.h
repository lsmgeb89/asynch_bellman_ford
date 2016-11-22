#ifndef ASYNCH_BELLMAN_FORD_LOG_UTIL_H_
#define ASYNCH_BELLMAN_FORD_LOG_UTIL_H_

#include <algorithm>
#include <iomanip>
#include <mutex>

#ifndef NDEBUG
#define debug_log std::clog << lock_with(mutex_log_) << "[r " << round << "]"
#define proc_debug debug_log << "[proc " << id << "] "
#define proc_cout proc_debug
#else
class NullBuffer : public std::streambuf {
public:
  int overflow(int c) { return c; }
};

class NullStream : public std::ostream {
  public:
    NullStream() : std::ostream(&m_sb) {}
  private:
    NullBuffer m_sb;
};
static NullStream null_stream;

#define debug_log null_stream
#define proc_debug null_stream << "[proc " << id << "] "
#define proc_cout std::cout << lock_with(mutex_log_) \
                            << "[r " << round << "][proc " << id << "] "
#endif

inline std::ostream& operator<<(std::ostream& out,
                                const std::lock_guard<std::mutex> &) {
  return out;
}

template <typename T> inline std::lock_guard<T> lock_with(T &mutex) {
  mutex.lock();
  return { mutex, std::adopt_lock };
}

#endif // ASYNCH_BELLMAN_FORD_LOG_UTIL_H_
