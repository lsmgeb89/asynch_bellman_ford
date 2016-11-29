#ifndef ASYNCH_BELLMAN_FORD_BELLMAN_FORD_H_
#define ASYNCH_BELLMAN_FORD_BELLMAN_FORD_H_

#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <thread>
#include <vector>
#include <unordered_map>
#include "message_channel.h"

namespace Algo {

enum ThreadState {
  RoundBegin = 0, // Master notifies all processes that this round begins
  RoundEnd = 1,   // A process has done all the things in this round and wants to go to next round
  Exited = 2      // A process has done all the things in this round and exits in this round
};

enum Relation {
  Neighbor = 0,
  Parent = 1,
  Children = 2
};

typedef std::vector<std::vector<ptrdiff_t>> ConnectivityMatrix;

class BellmanFord {
 public:
  BellmanFord(const ConnectivityMatrix &conn_info,
              const utils::ProcessId &root_id);

  ~BellmanFord(void);

  void Run(void);

 private:
  void Process(const bool is_source,
               const utils::ProcessId id,
               ThreadState* const state,
               std::vector<utils::MessageChannel*> channels);

  void Master(void);

  // helpers
  static const std::string RelationListToString(const std::vector<Relation>& list) {
    std::stringstream ss;
    std::for_each(list.begin(), list.end(),
                  [&ss](const Relation& r) { ss << " " << r; });
    return ss.str();
  }

  inline void ResetWaitingList(std::size_t& waiting_msg_id,
                               const std::ptrdiff_t& parent_index,
                               std::vector<bool>& waiting_list);

  inline void BroadcastTermination(const utils::ProcessId& id,
                                   const std::vector<Relation>& relation_list,
                                   const std::vector<utils::MessageChannel*> channels);

  inline const bool IsAllAckReceived(const std::vector<bool>& waiting_list);

 private:
  const utils::ProcessId root_id_;

  // round begin
  mutable std::mutex mutex_round_begin_;
  std::condition_variable round_begin_;

  // round end
  mutable std::mutex mutex_round_end_;
  std::vector<ThreadState> thread_states_;
  std::condition_variable round_end_;

  // threads
  std::thread master_thread_;
  std::vector<std::thread> process_threads_;

  // message channel
  std::vector<std::vector<utils::MessageChannel*>> channels_;

  // log
  mutable std::mutex mutex_log_;
};

} // namespace Algo

#endif // ASYNCH_BELLMAN_FORD_BELLMAN_FORD_H_
