#include <fstream>
#include <iostream>
#include "bellman_ford.h"
#include "log_util.h"
#include "connectivity_parser.h"

int main(int argc, char* argv[]) {
  std::ifstream test_file(argv[1]);
  Algo::ConnectivityMatrix conn_info;
  utils::ProcessId root_id;

  if (argc != 2) {
    std::cerr << "Wrong parameter number: " << argc - 1 << std::endl;
    return -1;
  }

  if (!test_file.is_open()) {
    std::cerr << "Wrong test file path: " << argv[1] << std::endl;
    return -1;
  }

  if (!utils::ParseConnectivity(test_file, conn_info, root_id)) {
    test_file.close();
    return -1;
  } else {
    test_file.close();
  }

  Algo::BellmanFord bellman_ford_algo(conn_info, root_id);
  bellman_ford_algo.Run();

  return 0;
}
