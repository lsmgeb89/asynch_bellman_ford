#ifndef ASYNCH_BELLMAN_FORD_MESSAGE_H_
#define ASYNCH_BELLMAN_FORD_MESSAGE_H_

#include <string>
#include <sstream>

namespace utils {

enum MessageType {
  Explore = 0,
  Parent = 1,
  NonParent = 2,
  Terminate = 3
};

struct Message {
  Message(const MessageType& type,
          const std::size_t& pid,
          const std::size_t& msg_id,
          const float& dist)
    : type_(type),
      pid_(pid),
      msg_id_(msg_id),
      dist_(dist) {}

  MessageType type_;
  std::size_t pid_;
  std::size_t msg_id_;
  float dist_;

  std::string ToString(void) {
    std::stringstream out;
    if (Explore == type_) {
      out << "explore message from proc " << pid_ << ": dist = " << dist_;
    } else if (Parent == type_) {
      out << "parent message from proc " << pid_;
    } else if (NonParent == type_) {
      out << "non-parent message from proc " << pid_;
    } else if (Terminate == type_) {
      out << "terminate message from proc " << pid_;
    }
    return out.str();
  }
};

} // namespace utils

#endif // ASYNCH_BELLMAN_FORD_MESSAGE_H_
