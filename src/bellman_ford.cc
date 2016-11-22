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

void BellmanFord::Process(const bool is_source,
                          const utils::ProcessId id,
                          ThreadState* state,
                          std::vector<utils::MessageChannel*> channels) {
  bool exit(false);
  std::ptrdiff_t round(0);
  utils::ProcessId parent_id(0);
  utils::MessageChannel *parent_channel(nullptr);
  std::vector<Relation> relation_list(channels.size(), Null);
  std::size_t dist(is_source ? 0 : std::numeric_limits<std::size_t>::max());

  // initialization step
  if (is_source) {
    proc_cout << "source proc sends dist = " << dist << " to neighbors\n";
    std::for_each(channels.begin(), channels.end(),
                  [&id, &dist](utils::MessageChannel *const c) {
                    c->Send(id, new utils::Message(utils::Explore, id, dist)); });
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

      if (channels.at(i)->Receive(id, &msg_received)) {
        proc_debug << "receives " << msg_received->ToString() << std::endl;

        // explore messages
        if (msg_received->type_ == utils::Explore) {
          auto d(channels.at(i)->GetWeight() + msg_received->dist_);

          // relaxation step
          if (d < dist) {
            // inform previous parent that you are not my parent any more
            if (parent_id && parent_channel) {
              relation_list.at(i) = Neighbor;
              proc_debug << "rejects old parent proc " << parent_id << std::endl;
              parent_channel->Send(id, new utils::Message(utils::NonParent, id, 0));
            }

            // bookkeeping
            dist = d;
            parent_id = msg_received->id_;
            parent_channel = channels.at(i);
            relation_list.at(i) = Parent;

            // inform new parent
            proc_debug << "informs new parent proc " << parent_id << std::endl;
            channels.at(i)->Send(id, new utils::Message(utils::Parent, id, 0));

            // inform other neighbors to update distance
            proc_debug << "informs new dist = " << dist << " to neighbors" << std::endl;
            for (auto &channel_update : channels) {
              if (channel_update != channels.at(i)) {
                channel_update->Send(id, new utils::Message(utils::Explore, id, dist));
              }
            }
          } else {
            // inform this neighbor that you are not my parent
            proc_debug << "rejects proc " << msg_received->id_ << " as parent\n";
            channels.at(i)->Send(id, new utils::Message(utils::NonParent, id, 0));
          }
        } else if (msg_received->type_ == utils::NonParent) {
          relation_list.at(i) = Neighbor;
        } else if (msg_received->type_ == utils::Parent) {
          relation_list.at(i) = Children;
        } else if (msg_received->type_ == utils::Complete) {
          relation_list.at(i) = static_cast<Relation>(relation_list.at(i) | Complete);
        }
        delete msg_received;
      }
    }

    // exit protocol
    proc_debug << "relation list:" << RelationListToString(relation_list) << std::endl;
    if (IsLeaf(relation_list) || IsInternal(relation_list)) {
      proc_cout << "converge-casts complete message to parent proc " << parent_id
                 << " and exits. output: parent = " << parent_id << " dist = " << dist << std::endl;
      parent_channel->Send(id, new utils::Message(utils::Complete, id, 0));
      exit = true;
    } else if (IsRoot(relation_list)) {
      proc_cout << "source proc receives converge cast and exits\n";
      exit = true;
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
