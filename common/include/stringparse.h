#ifndef ODINDATA_STRINGPARSE_H
#define ODINDATA_STRINGPARSE_H

#include <boost/algorithm/string.hpp>

static std::string extract_substr_at_pos(const std::string &input, const int position, const int limit) {

  int following_nline = std::min(position+limit, int(input.find_first_of("\n", position)));
  int preceding_nline = std::min(position-limit, int(input.find_last_of("\n", position)));
  std::string substr = input.substr(preceding_nline+1, following_nline-preceding_nline-1);
  boost::erase_all(substr, "");
  return substr;

}

#endif //ODINDATA_STRINGPARSE_H
