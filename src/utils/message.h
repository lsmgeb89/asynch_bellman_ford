#ifndef ASYNCH_BELLMAN_FORD_MESSAGE_H_
#define ASYNCH_BELLMAN_FORD_MESSAGE_H_

#include <string>
#include <sstream>

namespace utils {

enum MessageType {
  Explore = 0,
  Parent = 1,
  NonParent = 2,
  Complete = 3
};

struct Message {
  Message(const MessageType& type,
          const std::size_t& id,
          const std::size_t& dist)
    : type_(type),
      id_(id),
      dist_(dist) {}

  MessageType type_;
  std::size_t id_;
  std::size_t dist_;

  std::string ToString(void) {
    std::stringstream out;
    if (Explore == type_) {
      out << "explore message from proc " << id_ << ": dist = " << dist_;
    } else if (Parent == type_) {
      out << "parent message from proc " << id_;
    } else if (NonParent == type_) {
      out << "nonparent message from proc " << id_;
    } else if (Complete == type_) {
      out << "complete message from proc " << id_;
    }
    return out.str();
  }
};

} // namespace utils

#endif // ASYNCH_BELLMAN_FORD_MESSAGE_H_
