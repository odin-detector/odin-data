#ifndef FRAMESIMULATOR_FRAMESIMULATOROPTION_H
#define FRAMESIMULATOR_FRAMESIMULATOROPTION_H

#include <map>
#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include <boost/optional.hpp>

namespace po = boost::program_options;

namespace FrameSimulator {

    template <class T>
    class FrameSimulatorOption {

    public:

        FrameSimulatorOption(const std::string& astr, const std::string& desc) : argstring(astr), description(desc) {}
        FrameSimulatorOption(const std::string& astr, const std::string& desc, const T& dval) : argstring(astr), description(desc), defaultval(dval) {}

        const char* get_arg() const {
            return argstring.c_str();
        }

        bool is_specified(const po::variables_map& vm) const {
            return vm.count(argstring);
        }

        T get_val(const po::variables_map& vm) const {
            return vm[argstring].as<T>();
        }

        void get_val(const po::variables_map& vm, boost::optional<T>& val) const {
            if (is_specified(vm))
                val = get_val(vm);
        }

        void add_option_to(po::options_description& options) const {
            if (defaultval) {
                options.add_options()
                        (argstring.c_str(), po::value<T>()->default_value(defaultval.get()), description.c_str());
            }
            else
                options.add_options()
                        (argstring.c_str(), po::value<T>(), description.c_str());
             }

    private:

        const boost::optional<T> defaultval;
        const std::string argstring;
        const std::string description;

    };

}

#endif //FRAMESIMULATOR_FRAMESIMULATOROPTIONSHELPER_H
