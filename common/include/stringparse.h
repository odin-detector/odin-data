#ifndef ODINDATA_STRINGPARSE_H
#define ODINDATA_STRINGPARSE_H

#include <boost/algorithm/string.hpp>

// Return sub string at position
// Returns minimum of: back to the previous and up to the next line, or up to +/- limit characters either side of the position
static std::string extract_substr_at_pos(const std::string &input, const int position, const int limit) {

  int following_nline = std::min(position+limit, int(input.find_first_of("\n", position)));
  int preceding_nline = std::min(position-limit, int(input.find_last_of("\n", position)));
  std::string substr = input.substr(preceding_nline+1, following_nline-preceding_nline-1);
  boost::erase_all(substr, "");
  return substr;

}

// Returns line in input where position is
static int extract_line_no(const std::string &input, const int position) {

  std::string line_char = "\n";

  int line_no = 1;

  size_t pos = input.find(line_char);

  while(pos < position) {
    line_no ++;
    pos = input.find(line_char, pos + line_char.size());
  }

  return line_no;

}

#endif //ODINDATA_STRINGPARSE_H
