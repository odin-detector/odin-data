#define BOOST_TEST_MODULE "HDF5FileTests"
#define BOOST_TEST_MAIN

#include "Fixtures.h"

BOOST_GLOBAL_FIXTURE(GlobalConfig);

BOOST_FIXTURE_TEST_SUITE(HDF5FileUnitTest, FileWriterPluginTestFixture);

BOOST_AUTO_TEST_CASE(HDF5FileTest)
{

    std::cout << "**********************************************************************" << std::endl;
    std::cout << "*                                                                    *" << std::endl;
    std::cout << "* *** Starting HDF5File unit tests - ERROR messages are expected *** *" << std::endl;
    std::cout << "*                                                                    *" << std::endl;
    std::cout << "**********************************************************************" << std::endl;
    std::cout << std::endl;

    FrameProcessor::HDF5File hdf5f(hdf5_error_definition);

    // Use the PID to ensure file created has a unique name
    std::stringstream ss;
    ss << "/tmp/blah_pid" << getpid() << ".h5";
    BOOST_REQUIRE_NO_THROW(hdf5f.create_file(ss.str(), 0, false, 1, 1));
    BOOST_REQUIRE_NO_THROW(hdf5f.create_dataset(dset_def, -1, -1));
    BOOST_REQUIRE_EQUAL(dset_def.name, frame->get_meta_data().get_dataset_name());

    BOOST_REQUIRE_NO_THROW(hdf5f.write_frame(*frame, frame->get_frame_number(), 1, durations));
    BOOST_REQUIRE_NO_THROW(hdf5f.close_file());
}

BOOST_AUTO_TEST_CASE(HDF5FileMultiDatasetTest)
{
    FrameProcessor::HDF5File hdf5f(hdf5_error_definition);

    // Use the PID to ensure file created has a unique name
    std::stringstream ss;
    ss << "/tmp/blah_multidataset_pid" << getpid() << ".h5";
    BOOST_REQUIRE_NO_THROW(hdf5f.create_file(ss.str(), 0, false, 1, 1));

    // Create the first dataset "data"
    BOOST_REQUIRE_NO_THROW(hdf5f.create_dataset(dset_def, -1, -1));
    BOOST_CHECK_EQUAL(dset_def.name, frame->get_meta_data().get_dataset_name());

    // Create the second dataset "stuff"
    dset_def.name = "stuff";
    BOOST_REQUIRE_NO_THROW(hdf5f.create_dataset(dset_def, -1, -1));

    // Write first frame to "data"
    BOOST_REQUIRE_NO_THROW(hdf5f.write_frame(*frame, frame->get_frame_number(), 1, durations));

    // Write the frame to "stuff"
    BOOST_CHECK_NO_THROW(frame->meta_data().set_dataset_name("stuff"));
    BOOST_CHECK_EQUAL(dset_def.name, frame->get_meta_data().get_dataset_name());
    BOOST_REQUIRE_NO_THROW(hdf5f.write_frame(*frame, frame->get_frame_number(), 1, durations));

    // write another frame to "data"
    BOOST_CHECK_EQUAL("data", frames[2]->get_meta_data().get_dataset_name());
    BOOST_REQUIRE_NO_THROW(hdf5f.write_frame(*frames[2], frames[2]->get_frame_number(), 1, durations));
    // and yet another frame to "stuff"
    BOOST_CHECK_NO_THROW(frames[2]->meta_data().set_dataset_name("stuff"));
    BOOST_REQUIRE_NO_THROW(hdf5f.write_frame(*frames[2], frames[2]->get_frame_number(), 1, durations));

    BOOST_REQUIRE_NO_THROW(hdf5f.close_file());
}

BOOST_AUTO_TEST_CASE(HDF5FileBadFileTest)
{
    FrameProcessor::HDF5File hdf5f(hdf5_error_definition);

    // Check for an error when a bad file path is provided
    BOOST_CHECK_THROW(hdf5f.create_file("/non/existent/path/blah_throw.h5", 0, false, 1, 1), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(FileWriterPluginDatasetWithoutOpenFileTest)
{
    FrameProcessor::HDF5File hdf5f(hdf5_error_definition);

    // Check for an error when a dataset is created without a file open
    BOOST_CHECK_THROW(hdf5f.create_dataset(dset_def, -1, -1), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(HDF5FileNoDatasetDefinitionsTest)
{
    FrameProcessor::HDF5File hdf5f(hdf5_error_definition);

    // Create a file ready for writing
    // Use the PID to ensure file created has a unique name
    std::stringstream ss;
    ss << "/tmp/blah_throw_pid" << getpid() << ".h5";
    BOOST_REQUIRE_NO_THROW(hdf5f.create_file(ss.str(), 0, false, 1, 1));

    // Write a frame without first creating the dataset
    BOOST_CHECK_THROW(hdf5f.write_frame(*frame, frame->get_frame_number(), 1, durations), std::runtime_error);

    // Attempt to close the file
    BOOST_REQUIRE_NO_THROW(hdf5f.close_file());
}

BOOST_AUTO_TEST_CASE(HDF5FileInvalidDatasetTest)
{
    FrameProcessor::HDF5File hdf5f(hdf5_error_definition);

    // Use the PID to ensure file created has a unique name
    std::stringstream ss;
    ss << "/tmp/blah_throw_pid" << getpid() << ".h5";
    BOOST_REQUIRE_NO_THROW(hdf5f.create_file(ss.str(), 0, false, 1, 1));
    BOOST_REQUIRE_NO_THROW(hdf5f.create_dataset(dset_def, -1, -1));
    BOOST_REQUIRE_NO_THROW(frame->meta_data().set_dataset_name("non_existing_dataset_name"));

    BOOST_CHECK_THROW(hdf5f.write_frame(*frame, frame->get_frame_number(), 1, durations), std::runtime_error);
    BOOST_REQUIRE_NO_THROW(hdf5f.close_file());
}

BOOST_AUTO_TEST_CASE(FileWriterPluginMultipleFramesTest)
{
    FrameProcessor::HDF5File hdf5f(hdf5_error_definition);

    // Use the PID to ensure file created has a unique name
    std::stringstream ss;
    ss << "/tmp/blah_multiple_pid" << getpid() << ".h5";
    BOOST_REQUIRE_NO_THROW(hdf5f.create_file(ss.str(), 0, false, 1, 1));
    BOOST_REQUIRE_NO_THROW(hdf5f.create_dataset(dset_def, -1, -1));

    std::vector<boost::shared_ptr<FrameProcessor::DataBlockFrame>>::iterator it;
    for (it = frames.begin(); it != frames.end(); ++it) {
        BOOST_TEST_MESSAGE("Writing frame: " << (*it)->get_frame_number());
        BOOST_REQUIRE_NO_THROW(hdf5f.write_frame(*(*it), (*it)->get_frame_number(), 1, durations));
    }
    BOOST_REQUIRE_NO_THROW(hdf5f.close_file());
}

BOOST_AUTO_TEST_CASE(HDF5FileMultipleReverseTest)
{
    FrameProcessor::HDF5File hdf5f(hdf5_error_definition);

    // Just reverse through the list of frames and write them out.
    // The frames should still appear in the file in the original order...
    // Use the PID to ensure file created has a unique name
    std::stringstream ss;
    ss << "/tmp/blah_multiple_reverse_pid" << getpid() << ".h5";
    BOOST_REQUIRE_NO_THROW(hdf5f.create_file(ss.str(), 0, false, 1, 1));
    BOOST_REQUIRE_NO_THROW(hdf5f.create_dataset(dset_def, -1, -1));

    // The first frame to write is used as an offset - so must be the lowest
    // frame number. Frames received later with a smaller number would result
    // in negative file index...
    BOOST_TEST_MESSAGE("Writing frame: " << frame->get_frame_number());
    BOOST_REQUIRE_NO_THROW(hdf5f.write_frame(*frame, frame->get_frame_number(), 1, durations));

    std::vector<boost::shared_ptr<FrameProcessor::DataBlockFrame>>::reverse_iterator rit;
    for (rit = frames.rbegin(); rit != frames.rend(); ++rit) {
        BOOST_TEST_MESSAGE("Writing frame: " << (*rit)->get_frame_number());
        BOOST_REQUIRE_NO_THROW(hdf5f.write_frame(*(*rit), (*rit)->get_frame_number(), 1, durations));
    }
    BOOST_REQUIRE_NO_THROW(hdf5f.close_file());
}

BOOST_AUTO_TEST_CASE(HDF5FileAdjustHugeOffset)
{
    FrameProcessor::HDF5File hdf5f(hdf5_error_definition);

    // Use the PID to ensure file created has a unique name
    std::stringstream ss;
    ss << "/tmp/blah_huge_offset_pid" << getpid() << ".h5";
    BOOST_REQUIRE_NO_THROW(hdf5f.create_file(ss.str(), 0, false, 1, 1));
    BOOST_REQUIRE_NO_THROW(hdf5f.set_unlimited());
    BOOST_REQUIRE_NO_THROW(dset_def.num_frames = 0);
    BOOST_REQUIRE_NO_THROW(hdf5f.create_dataset(dset_def, -1, -1));

    hsize_t huge_offset = 100000;

    std::vector<boost::shared_ptr<FrameProcessor::DataBlockFrame>>::iterator it;
    for (it = frames.begin(); it != frames.end(); ++it) {
        size_t frame_no = (*it)->get_frame_number();
        // PercivalEmulator::FrameHeader img_header = *((*it)->get_header());
        // img_header.frame_number = frame_no + huge_offset;
        //(*it)->copy_header(&img_header);
        (*it)->meta_data().set_frame_offset(huge_offset);
        (*it)->set_frame_number(frame_no + huge_offset);
        BOOST_REQUIRE_NO_THROW(hdf5f.write_frame(*(*it), (*it)->get_frame_number(), 1, durations));
    }
    BOOST_REQUIRE_NO_THROW(hdf5f.close_file());
}

BOOST_AUTO_TEST_CASE(FileWriterPluginWriteParamTest)
{
    FrameProcessor::HDF5File hdf5f(hdf5_error_definition);

    // Use the PID to ensure file created has a unique name
    std::stringstream ss;
    ss << "/tmp/test_write_params_pid" << getpid() << ".h5";
    BOOST_REQUIRE_NO_THROW(hdf5f.create_file(ss.str(), 0, false, 1, 1));

    FrameProcessor::DatasetDefinition param_dset_def;
    param_dset_def.name = "p1";
    param_dset_def.data_type = FrameProcessor::raw_64bit;
    param_dset_def.num_frames = 10;
    dimensions_t chunk_dims(1);
    chunk_dims[0] = 1;
    param_dset_def.chunks = chunk_dims;
    param_dset_def.compression = FrameProcessor::no_compression;

    BOOST_REQUIRE_NO_THROW(hdf5f.create_dataset(param_dset_def, -1, -1));

    std::vector<boost::shared_ptr<FrameProcessor::DataBlockFrame>>::iterator it;
    uint64_t val = 0;
    for (it = frames.begin(); it != frames.end(); ++it) {
        (*it)->meta_data().set_parameter("p1", val);
        BOOST_TEST_MESSAGE("Writing frame: " << (*it)->get_frame_number());
        BOOST_REQUIRE_NO_THROW(hdf5f.write_parameter(*(*it), param_dset_def, (*it)->get_frame_number()));
        val++;
    }
    BOOST_REQUIRE_NO_THROW(hdf5f.close_file());
}

BOOST_AUTO_TEST_CASE(FileWriterPluginWriteParamWrongTypeTest)
{
    FrameProcessor::HDF5File hdf5f(hdf5_error_definition);

    // Use the PID to ensure file created has a unique name
    std::stringstream ss;
    ss << "/tmp/test_wrong_params_pid" << getpid() << ".h5";
    BOOST_REQUIRE_NO_THROW(hdf5f.create_file(ss.str(), 0, false, 1, 1));

    FrameProcessor::DatasetDefinition param_dset_def;
    param_dset_def.name = "p1";
    param_dset_def.data_type = FrameProcessor::raw_16bit;
    param_dset_def.num_frames = 10;
    dimensions_t chunk_dims(1);
    chunk_dims[0] = 1;
    param_dset_def.chunks = chunk_dims;
    param_dset_def.compression = FrameProcessor::no_compression;

    BOOST_REQUIRE_NO_THROW(hdf5f.create_dataset(param_dset_def, -1, -1));

    std::vector<boost::shared_ptr<FrameProcessor::DataBlockFrame>>::iterator it;
    for (it = frames.begin(); it != frames.end(); ++it) {
        uint64_t val = 123;
        (*it)->meta_data().set_parameter("p1", val);
        BOOST_TEST_MESSAGE("Writing frame: " << (*it)->get_frame_number());
        BOOST_CHECK_THROW(hdf5f.write_parameter(*(*it), param_dset_def, (*it)->get_frame_number()), std::runtime_error);
    }
    BOOST_REQUIRE_NO_THROW(hdf5f.close_file());
}

BOOST_AUTO_TEST_CASE(FileWriterPluginWriteParamNoParamTest)
{
    FrameProcessor::HDF5File hdf5f(hdf5_error_definition);

    // Use the PID to ensure file created has a unique name
    std::stringstream ss;
    ss << "/tmp/test_no_params_pid" << getpid() << ".h5";
    BOOST_REQUIRE_NO_THROW(hdf5f.create_file(ss.str(), 0, false, 1, 1));

    FrameProcessor::DatasetDefinition param_dset_def;
    param_dset_def.name = "p1";
    param_dset_def.data_type = FrameProcessor::raw_64bit;
    param_dset_def.num_frames = 10;
    dimensions_t chunk_dims(1);
    chunk_dims[0] = 1;
    param_dset_def.chunks = chunk_dims;
    param_dset_def.compression = FrameProcessor::no_compression;

    BOOST_REQUIRE_NO_THROW(hdf5f.create_dataset(param_dset_def, -1, -1));

    std::vector<boost::shared_ptr<FrameProcessor::DataBlockFrame>>::iterator it;
    for (it = frames.begin(); it != frames.end(); ++it) {
        uint64_t val = 123;
        (*it)->meta_data().set_parameter("p2", val);
        BOOST_TEST_MESSAGE("Writing frame: " << (*it)->get_frame_number());
        BOOST_CHECK_THROW(hdf5f.write_parameter(*(*it), param_dset_def, (*it)->get_frame_number()), std::runtime_error);
    }
    BOOST_REQUIRE_NO_THROW(hdf5f.close_file());
}

BOOST_AUTO_TEST_SUITE_END();
