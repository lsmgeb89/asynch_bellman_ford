#ifndef ASYNCH_BELLMAN_FORD_MESSAGE_CHANNEL_H_
#define ASYNCH_BELLMAN_FORD_MESSAGE_CHANNEL_H_

#include <queue>
#include <memory>
#include <mutex>
#include <random>
#include "message.h"

namespace utils {

typedef std::size_t ProcessId;
typedef std::size_t TimeStamp;
typedef float ChannelWeight;

class MessageChannel {
 public:
  MessageChannel(const ProcessId &id_0,
                 const ProcessId &id_1,
                 const ChannelWeight &weight)
    : id_0_(id_0),
      id_1_(id_1),
      weight_(weight),
      dist_(1, 15) {
    random_engine_.seed(std::random_device()());
  }

  ~MessageChannel(void) {
    while (!msg_queue_0_.empty()) {
      delete msg_queue_0_.front().second;
      msg_queue_0_.pop();
    }
    while (!msg_queue_1_.empty()) {
      delete msg_queue_1_.front().second;
      msg_queue_1_.pop();
    }
  }

  const ChannelWeight GetWeight(void) const { return weight_; }

  void Send(const ProcessId &sender, Message *msg) {
    if (sender == id_0_) {
      std::unique_lock<std::mutex> lock(msg_mutex_1_);
      msg_queue_1_.emplace(dist_(random_engine_), msg);
    } else if (sender == id_1_) {
      std::unique_lock<std::mutex> lock(msg_mutex_0_);
      msg_queue_0_.emplace(dist_(random_engine_), msg);
    }
  }

  bool Receive(const ProcessId &receiver, Message **msg) {
    if (receiver == id_0_) {
      std::unique_lock<std::mutex> lock(msg_mutex_0_);
      if (msg_queue_0_.empty()) { return false; }
      if (!msg_queue_0_.front().first) {
        *msg = msg_queue_0_.front().second;
        msg_queue_0_.pop();
        return true;
      } else {
        msg_queue_0_.front().first--;
        return false;
      }
    } else if (receiver == id_1_) {
      std::unique_lock<std::mutex> lock(msg_mutex_1_);
      if (msg_queue_1_.empty()) { return false; }
      if (!msg_queue_1_.front().first) {
        *msg = msg_queue_1_.front().second;
        msg_queue_1_.pop();
        return true;
      } else {
        msg_queue_1_.front().first--;
        return false;
      }
    } else {
      return false;
    }
  }

 private:
  // process ids
  const ProcessId id_0_;
  const ProcessId id_1_;
  const ChannelWeight weight_;

  // random delay
  std::mt19937 random_engine_;
  std::uniform_int_distribution<std::mt19937::result_type> dist_;

  // msg queue from p1 -> p0
  std::queue<std::pair<TimeStamp, Message*>> msg_queue_0_;
  mutable std::mutex msg_mutex_0_;

  // msg queue from p0 -> p1
  std::queue<std::pair<TimeStamp, Message*>> msg_queue_1_;
  mutable std::mutex msg_mutex_1_;
};

} // namespace utils

#endif // ASYNCH_BELLMAN_FORD_MESSAGE_CHANNEL_H_
