#include <iostream>
#include "bellman_ford.h"
#include "log_util.h"

namespace Algo {

BellmanFord::BellmanFord(const ConnectivityMatrix &conn_info,
                         const utils::ProcessId &root_id)
  : root_id_(root_id),
    thread_states_(conn_info.size(), RoundEnd) {
  channels_.resize(conn_info.size());
  for (std::size_t i(0); i < conn_info.size(); i++) {
    channels_.at(i).resize(conn_info.size(), nullptr);
    for (std::size_t j(0); j < conn_info.at(i).size(); j++) {
      if (conn_info.at(i).at(j) != -1) {
        if (j > i) {
          channels_.at(i).at(j) = new utils::MessageChannel(i + 1, j + 1, conn_info.at(i).at(j));
        } else {
          channels_.at(i).at(j) = channels_.at(j).at(i);
        }
      }
    }
  }
}

BellmanFord::~BellmanFord(void) {
  for (std::size_t i(0); i < channels_.size(); i++) {
    for (std::size_t j(0); j < channels_.at(i).size(); j++) {
      if (channels_.at(i).at(j) && j > i) {
        delete channels_.at(i).at(j);
      }
    }
  }
}

void BellmanFord::Run(void) {
  master_thread_ = std::thread([this] { this->Master(); });
  master_thread_.join();
}

inline void BellmanFord::ResetWaitingList(std::size_t& waiting_msg_id,
                                          const std::ptrdiff_t& parent_index,
                                          std::vector<bool>& waiting_list) {
  // generate explore message id
  // now we begins to wait acknowledges for this explore message
  ++waiting_msg_id;
  // reset waiting list except parent
  std::fill(waiting_list.begin(), waiting_list.end(), false);
  if (parent_index != -1) {
    waiting_list.at(static_cast<size_t>(parent_index)) = true;
  }
}

inline void BellmanFord::BroadcastTermination(const utils::ProcessId& id,
                                              const std::vector<Relation>& relation_list,
                                              const std::vector<utils::MessageChannel*> channels) {
  // broadcast termination to children
  for (std::size_t i(0); i < relation_list.size(); i++) {
    if (relation_list.at(i) == Children) {
      channels.at(i)->Send(id, new utils::Message(utils::Terminate, id, 0, 0.0f));
    }
  }
}

inline const bool BellmanFord::IsAllAckReceived(const std::vector<bool>& waiting_list) {
  return std::all_of(waiting_list.begin(), waiting_list.end(),
                     [](const bool &received) { return received; });
}

void BellmanFord::Process(const bool is_source,
                          const utils::ProcessId id,
                          ThreadState* state,
                          std::vector<utils::MessageChannel*> channels) {
  bool exit(false);
  std::ptrdiff_t round(0);
  // waiting acknowledgements
  std::vector<Relation> relation_list(channels.size(), Neighbor);
  std::size_t waiting_msg_id(0); // waiting for which explore message
  std::vector<bool> waiting_list(channels.size(), false);
  std::size_t prev_parent_explore_msg_id(0);
  std::size_t curr_parent_explore_msg_id(0);
  // bookkeeping
  utils::ProcessId parent_id(0);
  std::ptrdiff_t parent_index(-1);
  float dist(is_source ? 0.0f : std::numeric_limits<float>::infinity());

  // initialization step for source
  if (is_source) {
    ResetWaitingList(waiting_msg_id, parent_index, waiting_list);
    proc_cout << "source process sends dist = " << dist << " to neighbors\n";
    std::for_each(channels.begin(), channels.end(),
                  [&id, &dist, &waiting_msg_id](utils::MessageChannel *const c) {
                    c->Send(id, new utils::Message(utils::Explore, id, waiting_msg_id, dist)); });
  }

  while (!exit) {
    // wait master to notify that this round begins
    {
      std::unique_lock<std::mutex> lock(mutex_round_begin_);
      round_begin_.wait(lock, [state] { return (*state == RoundBegin); });
      proc_debug << "round begins\n";
    }

    // query messages from all links
    for (size_t i(0); i < channels.size(); i++) {
      utils::Message *msg_received(nullptr);

      if (!channels.at(i)->Receive(id, &msg_received)) { continue; }
      proc_debug << "receives " << msg_received->ToString() << std::endl;

      // explore messages
      if (msg_received->type_ == utils::Explore) {
        float dist_received(channels.at(i)->GetWeight() + msg_received->dist_);

        // relaxation step
        if (dist_received < dist) {
          // bookkeeping
          dist = dist_received;
          std::ptrdiff_t prev_parent(parent_index);
          parent_index = i;
          parent_id = msg_received->pid_;

          prev_parent_explore_msg_id = curr_parent_explore_msg_id;
          curr_parent_explore_msg_id = msg_received->msg_id_;

          relation_list.at(static_cast<size_t>(parent_index)) = Parent;

          ResetWaitingList(waiting_msg_id, parent_index, waiting_list);

          // inform other neighbors to update distance except new parent, but include old parent
          proc_debug << "informs new dist = " << dist << " to neighbors" << std::endl;
          for (auto &channel_update : channels) {
            if (channel_update != channels.at(static_cast<size_t>(parent_index))) {
              channel_update->Send(id, new utils::Message(utils::Explore, id, waiting_msg_id, dist));
            }
          }

          // reject old parent
          if (prev_parent != -1) {
            proc_debug << "rejects old parent " << msg_received->pid_ << "\n";
            channels.at(static_cast<std::size_t>(prev_parent))->Send(id, new utils::Message(utils::NonParent, id, prev_parent_explore_msg_id, 0.0f));
          }
        } else {
          // inform this neighbor that you are not my parent
          proc_debug << "rejects process " << msg_received->pid_ << " as parent\n";
          channels.at(i)->Send(id, new utils::Message(utils::NonParent, id, msg_received->msg_id_, 0.0f));
        }
      } else if (msg_received->type_ == utils::NonParent) {
        if (msg_received->msg_id_ == waiting_msg_id) {
          waiting_list.at(i) = true;
          relation_list.at(i) = Neighbor;
        }
      } else if (msg_received->type_ == utils::Parent) {
        if (msg_received->msg_id_ == waiting_msg_id) {
          waiting_list.at(i) = true;
          relation_list.at(i) = Children;
        }
      } else if (msg_received->type_ == utils::Terminate) {
        // exit protocol for non-source node
        if (i == static_cast<std::size_t>(parent_index) && !is_source) {
          // broadcast termination to children
          BroadcastTermination(id, relation_list, channels);
          delete msg_received;
          exit = true;
          proc_cout << "terminates output: parent = " << parent_id << " dist = " << dist << std::endl;
          break; // for loop
        }
      }

      // proceed this message, delete it
      delete msg_received;

      // inform new parent
      if (IsAllAckReceived(waiting_list) && !is_source) {
        proc_debug << "informs new parent process " << parent_id << std::endl;
        channels.at(static_cast<std::size_t>(parent_index))->Send(id, new utils::Message(utils::Parent, id, curr_parent_explore_msg_id, 0.0f));
      }
    } // end for loop

    proc_debug <<"relation list:" << RelationListToString(relation_list) << std::endl;

    // exit protocol for source node
    if (is_source && IsAllAckReceived(waiting_list)) {
      BroadcastTermination(id, relation_list, channels);
      exit = true;
      proc_cout << "source process terminates and broadcasts termination\n";
    }

    // notify master process that I'm done with this round or I exit
    {
      std::unique_lock<std::mutex> lock(mutex_round_end_);
      if (exit) {
        *state = Exited;
      } else {
        *state = RoundEnd;
      }
      round_end_.notify_one();
      proc_debug << "round ends\n";
    }

    round++;
  }
}

void BellmanFord::Master(void) {
  std::ptrdiff_t round(-1);
  // spawn threads
  for (size_t i = 0; i < channels_.size(); i++) {
    std::vector<utils::MessageChannel*> channel;
    std::copy_if(channels_.at(i).begin(), channels_.at(i).end(),
                std::back_inserter(channel), [](const utils::MessageChannel *c) { return c; });
    process_threads_.emplace_back([this, i, channel] { this->Process(root_id_ == i + 1,
                                                                     i + 1,
                                                                     &thread_states_.at(i),
                                                                     channel); });
  }

  // If there is any process wants to go to next round, we do this.
  // Otherwise, we exits.
  while (std::any_of(thread_states_.cbegin(),
                     thread_states_.cend(),
                     [](const ThreadState& state) { return (state == RoundEnd);})) {
    // signal all non exited processes that this round begins
    {
      std::unique_lock<std::mutex> lock(mutex_round_begin_);
      round++;
      std::for_each(thread_states_.begin(), thread_states_.end(),
                    [](ThreadState& state) {
                      if (state == RoundEnd) { state = RoundBegin; }});
      debug_log << "[master] round begins\n";
      round_begin_.notify_all();
    }

    // wait all processes end or exit this round
    {
      std::unique_lock<std::mutex> lock(mutex_round_end_);
      round_end_.wait(lock, [this] {
        return (std::none_of(thread_states_.cbegin(), thread_states_.cend(),
                             [](const ThreadState& state) { return (state == RoundBegin); })); });
      debug_log << "[master] round ends\n";
    }
  }

  // wait all threads exit
  for (auto& thread : process_threads_) {
    thread.join();
  }
}

} // namespace Algo
