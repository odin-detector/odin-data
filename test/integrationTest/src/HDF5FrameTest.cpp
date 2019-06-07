#include <boost/test/unit_test.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include "PropertyTreeUtility.h"

#include <iostream>

#include <hdf5.h>
#include <hdf5_hl.h>

namespace FrameSimulatorTest {

    class HDF5FrameTest {

    public:

        HDF5FrameTest() {

          const int master_argc = boost::unit_test::framework::master_test_suite().argc;

          if (master_argc > 1) {

            std::string config_file = boost::unit_test::framework::master_test_suite().argv[1];

            boost::property_tree::ini_parser::read_ini(config_file, ptree);

            // Get path to hdf5 file
            std::string output_file = ptree.get<std::string>("Test.output_file");
            PropertyTreeUtility::expandEnvVars(output_file);

            file_id = H5Fopen(output_file.c_str(), H5F_ACC_SWMR_READ, H5P_DEFAULT);
            dataset = H5Dopen(file_id, "/data", H5P_DEFAULT);
          } else {
            throw std::runtime_error("HDF5FrameTest: ini file not specified!");
          };
        }

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

        };

    BOOST_AUTO_TEST_SUITE_END();

}
