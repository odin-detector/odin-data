#include <boost/test/unit_test.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

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

            po::parsed_options parsed = po::command_line_parser(master_argc, boost::unit_test::framework::master_test_suite().argv).
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

	      std::string dataset_name = "/" + ptree.get<std::string>("Test.dataset");

              file_id = H5Fopen(output_file.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
              dataset = H5Dopen(file_id, dataset_name.c_str(), H5P_DEFAULT);
            } else {
              throw std::runtime_error("HDF5FrameTest: json file not specified!");
            }

        }
        ~HDF5FrameTest() {}

        hid_t file_id, dataset;
        boost::property_tree::ptree ptree;

    };

    BOOST_FIXTURE_TEST_SUITE(HDF5FrameUnitTest, HDF5FrameTest);

        BOOST_AUTO_TEST_CASE(HDF5Frame_size) {

          hid_t filetype = H5Dget_type(dataset);

          hid_t space = H5Dget_space(dataset);

          const int ndims = H5Sget_simple_extent_ndims(space);

          hsize_t dims[ndims];
          int ndms = H5Sget_simple_extent_dims(space, dims, NULL);

          BOOST_CHECK_EQUAL(ndims, ptree.get<int>("Test.dimensions"));
          BOOST_CHECK_EQUAL(dims[0], ptree.get<int>("Test.frames"));
          BOOST_CHECK_EQUAL(dims[1], ptree.get<int>("Test.width"));
          BOOST_CHECK_EQUAL(dims[2], ptree.get<int>("Test.height"));

          H5Fclose(file_id);

        };

    BOOST_AUTO_TEST_SUITE_END();

}
