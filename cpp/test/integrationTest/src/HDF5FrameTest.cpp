#include <boost/test/unit_test.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/program_options.hpp>

#include "PropertyTreeUtility.h"

#include <iostream>

#include <hdf5.h>
#include <hdf5_hl.h>

namespace po = boost::program_options;

namespace FrameSimulatorTest {

    class HDF5FrameTest {

    public:

        HDF5FrameTest() {

          const int master_argc = boost::unit_test::framework::master_test_suite().argc;

          po::options_description generic("Generic options");

          generic.add_options()
                  ("json", po::value<std::string>(), "Configuration file");

          po::options_description cmdline_options;
          cmdline_options.add(generic);

          po::parsed_options parsed = po::command_line_parser(master_argc,
                                                              boost::unit_test::framework::master_test_suite().argv).
                  options(generic).
                  allow_unregistered().
                  run();

          po::variables_map vm;
          po::store(parsed, vm);

          if (vm.count("json")) {

            std::string config_file = vm["json"].as<std::string>();

            boost::property_tree::json_parser::read_json(config_file, ptree);

            // Get path to hdf5 file
            std::string output_file = ptree.get<std::string>("Test.output_file");
            PropertyTreeUtility::expandEnvVars(output_file);

            std::string dataset_name = '/' + ptree.get<std::string>("Test.dataset");

            file_id = H5Fopen(output_file.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
            dataset = H5Dopen(file_id, dataset_name.c_str(), H5P_DEFAULT);
            filetype = H5Dget_type(dataset);
            space = H5Dget_space(dataset);

          } else {
            throw std::runtime_error("HDF5FrameTest: json file not specified!");
          }

        }

        template<class T> void data_check() {

          const int ndims = H5Sget_simple_extent_ndims(space);

          hsize_t dims[ndims];
          int ndms = H5Sget_simple_extent_dims(space, dims, NULL);

          int num_pts = dims[0];

          for(int n=1; n<ndms; n++)
            num_pts *= dims[n];

          T *data_out = new T[num_pts];

          H5Dread(dataset, filetype, H5S_ALL, H5S_ALL, H5P_DEFAULT, data_out);

          BOOST_FOREACH(boost::property_tree::ptree::value_type &vc, ptree.get_child("Test.data"))
                  BOOST_CHECK_EQUAL(data_out[std::atoi(vc.first.c_str())],
                                    ptree.get<T>("Test.data." + vc.first));

        };

        ~HDF5FrameTest() {}

        hid_t file_id, dataset, filetype, space;
        boost::property_tree::ptree ptree;

    };

    BOOST_FIXTURE_TEST_SUITE(HDF5FrameUnitTest, HDF5FrameTest);

        BOOST_AUTO_TEST_CASE(HDF5Frame_size) {

          const int ndims = H5Sget_simple_extent_ndims(space);

          hsize_t dims[ndims];
          int ndms = H5Sget_simple_extent_dims(space, dims, NULL);

          int frames = dims[0];

          if (boost::optional<int> t_dims = ptree.get_optional<int>("Test.dimensions"))
            BOOST_CHECK_EQUAL(ndims, t_dims.get());

          if (boost::optional<int> t_frames = ptree.get_optional<int>("Test.frames"))
            BOOST_CHECK_EQUAL(frames, t_frames.get());

          if (boost::optional<int> t_width = ptree.get_optional<int>("Test.width")) {
            int width = dims[1];
            BOOST_CHECK_EQUAL(width, t_width.get());
          }
          if (boost::optional<int> t_height = ptree.get_optional<int>("Test.height")) {
            int height = dims[2];
            BOOST_CHECK_EQUAL(height, t_height.get());
          }

        };

        BOOST_AUTO_TEST_CASE(HFD5Frame_data) {

          std::string data_type = ptree.get<std::string>("Test.type");

          if (data_type == "uint8")
            data_check<uint8_t>();
          else if (data_type == "uint16")
            data_check<uint16_t>();
          else if (data_type == "uint32")
            data_check<uint32_t>();
          else if (data_type == "uint64")
            data_check<uint64_t>();
          else if (data_type == "float")
            data_check<float>();

        }

    BOOST_AUTO_TEST_SUITE_END();

}
