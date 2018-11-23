#include "FrameSimulatorOption.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace FrameSimulator {

      void set_list_option(const std::string& option_val, std::vector<std::string>& list) {
          boost::split(list, option_val, boost::is_any_of(","), boost::token_compress_on);
      }

      void set_optionallist_option(const std::string& option_val, boost::optional<std::vector<std::string> >& list) {
          std::vector<std::string> tmp_list;
          set_list_option(option_val, tmp_list);
          list = tmp_list;
      }

}