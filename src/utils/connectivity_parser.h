#ifndef ASYNCH_BELLMAN_FORD_CONNECTIVITY_PARSER_H_
#define ASYNCH_BELLMAN_FORD_CONNECTIVITY_PARSER_H_

#include <regex>

namespace utils {

bool ExtractStr(const std::string &target_str,
                const std::string &regex_str,
                std::vector<std::string> &result_str) {
  std::string target(target_str);
  std::regex reg(regex_str);
  std::smatch match_result;
  result_str.clear();

  while (std::regex_search(target, match_result, reg)) {
    result_str.push_back(match_result[1]);
    target = match_result.suffix().str();
  }

  return !result_str.empty();
}

bool ParseConnectivity(std::ifstream &file,
                       Algo::ConnectivityMatrix &conn_info,
                       utils::ProcessId &root_id) {
  std::size_t process_num(0);
  std::size_t line_num(0);
  std::string line;
  std::vector<std::string> result_str;
  const std::string num_regex("(-?[\\d]+)");

  while (std::getline(file, line)) {
    if (!utils::ExtractStr(line, num_regex, result_str)) { goto error; }
    if (!line_num) {
      if (result_str.size() != 2) { goto error; }
      process_num = std::stoul(result_str.at(0));
      root_id = std::stoul(result_str.at(1));
      conn_info.resize(process_num);
    } else {
      if (result_str.size() != process_num) { goto error; }
      for (auto &str : result_str) {
        conn_info.at(line_num - 1).push_back(std::stol(str));
      }
    }
    line_num++;
  }
  return true;
error:
  std::cerr << "format error at line " << line_num + 1 << "\n";
  return false;
}

} // namespace utils

#endif // ASYNCH_BELLMAN_FORD_CONNECTIVITY_PARSER_H_
