#define BOOST_TEST_MODULE "AcquisitionPluginTests"
#define BOOST_TEST_MAIN

#include "Fixtures.h"
#include "TestHelperFunctions.h"

BOOST_TEST_GLOBAL_FIXTURE(GlobalConfig);

BOOST_FIXTURE_TEST_SUITE(AcquisitionUnitTest, FileWriterPluginTestFixture);

BOOST_AUTO_TEST_CASE(AcquisitionGetFileIndex)
{
    FrameProcessor::Acquisition acquisition(hdf5_error_definition);

    acquisition.concurrent_rank_ = 0;
    acquisition.concurrent_processes_ = 4;
    acquisition.frames_per_block_ = 1000;
    acquisition.blocks_per_file_ = 1;

    size_t file_index = acquisition.get_file_index(0);
    BOOST_CHECK_EQUAL(0, file_index);

    file_index = acquisition.get_file_index(1);
    BOOST_CHECK_EQUAL(0, file_index);

    file_index = acquisition.get_file_index(999);
    BOOST_CHECK_EQUAL(0, file_index);

    file_index = acquisition.get_file_index(4000);
    BOOST_CHECK_EQUAL(4, file_index);

    file_index = acquisition.get_file_index(4523);
    BOOST_CHECK_EQUAL(4, file_index);

    file_index = acquisition.get_file_index(4999);
    BOOST_CHECK_EQUAL(4, file_index);

    file_index = acquisition.get_file_index(8231);
    BOOST_CHECK_EQUAL(8, file_index);

    acquisition.concurrent_rank_ = 1;

    file_index = acquisition.get_file_index(1000);
    BOOST_CHECK_EQUAL(1, file_index);

    file_index = acquisition.get_file_index(1111);
    BOOST_CHECK_EQUAL(1, file_index);

    file_index = acquisition.get_file_index(1999);
    BOOST_CHECK_EQUAL(1, file_index);

    file_index = acquisition.get_file_index(5000);
    BOOST_CHECK_EQUAL(5, file_index);

    acquisition.concurrent_rank_ = 2;

    file_index = acquisition.get_file_index(2000);
    BOOST_CHECK_EQUAL(2, file_index);

    file_index = acquisition.get_file_index(2311);
    BOOST_CHECK_EQUAL(2, file_index);

    file_index = acquisition.get_file_index(2999);
    BOOST_CHECK_EQUAL(2, file_index);

    file_index = acquisition.get_file_index(6999);
    BOOST_CHECK_EQUAL(6, file_index);

    acquisition.concurrent_rank_ = 3;

    file_index = acquisition.get_file_index(3000);
    BOOST_CHECK_EQUAL(3, file_index);

    file_index = acquisition.get_file_index(3311);
    BOOST_CHECK_EQUAL(3, file_index);

    file_index = acquisition.get_file_index(3999);
    BOOST_CHECK_EQUAL(3, file_index);

    file_index = acquisition.get_file_index(7452);
    BOOST_CHECK_EQUAL(7, file_index);

    acquisition.concurrent_rank_ = 0;
    acquisition.concurrent_processes_ = 1;
    acquisition.frames_per_block_ = 3;
    acquisition.blocks_per_file_ = 5;

    file_index = acquisition.get_file_index(0);
    BOOST_CHECK_EQUAL(0, file_index);

    file_index = acquisition.get_file_index(2);
    BOOST_CHECK_EQUAL(0, file_index);

    file_index = acquisition.get_file_index(10);
    BOOST_CHECK_EQUAL(0, file_index);

    file_index = acquisition.get_file_index(14);
    BOOST_CHECK_EQUAL(0, file_index);

    file_index = acquisition.get_file_index(15);
    BOOST_CHECK_EQUAL(1, file_index);

    file_index = acquisition.get_file_index(20);
    BOOST_CHECK_EQUAL(1, file_index);

    file_index = acquisition.get_file_index(29);
    BOOST_CHECK_EQUAL(1, file_index);

    file_index = acquisition.get_file_index(30);
    BOOST_CHECK_EQUAL(2, file_index);

    acquisition.concurrent_rank_ = 0;
    acquisition.concurrent_processes_ = 4;
    acquisition.frames_per_block_ = 100;
    acquisition.blocks_per_file_ = 1;

    file_index = acquisition.get_file_index(0);
    BOOST_CHECK_EQUAL(0, file_index);

    file_index = acquisition.get_file_index(401);
    BOOST_CHECK_EQUAL(4, file_index);
}

BOOST_AUTO_TEST_CASE(AcquisitionGetOffsetInFile)
{
    FrameProcessor::Acquisition acquisition(hdf5_error_definition);

    acquisition.concurrent_rank_ = 0;
    acquisition.concurrent_processes_ = 4;
    acquisition.frames_per_block_ = 1000;
    acquisition.blocks_per_file_ = 1;

    size_t file_index = acquisition.get_frame_offset_in_file(0);
    BOOST_CHECK_EQUAL(0, file_index);

    file_index = acquisition.get_frame_offset_in_file(1);
    BOOST_CHECK_EQUAL(1, file_index);

    file_index = acquisition.get_frame_offset_in_file(999);
    BOOST_CHECK_EQUAL(999, file_index);

    file_index = acquisition.get_frame_offset_in_file(4000);
    BOOST_CHECK_EQUAL(0, file_index);

    file_index = acquisition.get_frame_offset_in_file(4523);
    BOOST_CHECK_EQUAL(523, file_index);

    file_index = acquisition.get_frame_offset_in_file(4999);
    BOOST_CHECK_EQUAL(999, file_index);

    file_index = acquisition.get_frame_offset_in_file(8231);
    BOOST_CHECK_EQUAL(231, file_index);

    acquisition.concurrent_rank_ = 1;

    file_index = acquisition.get_frame_offset_in_file(1000);
    BOOST_CHECK_EQUAL(0, file_index);

    file_index = acquisition.get_frame_offset_in_file(1430);
    BOOST_CHECK_EQUAL(430, file_index);

    file_index = acquisition.get_frame_offset_in_file(5999);
    BOOST_CHECK_EQUAL(999, file_index);

    acquisition.concurrent_rank_ = 2;

    file_index = acquisition.get_frame_offset_in_file(2000);
    BOOST_CHECK_EQUAL(0, file_index);

    file_index = acquisition.get_frame_offset_in_file(2999);
    BOOST_CHECK_EQUAL(999, file_index);

    acquisition.concurrent_rank_ = 3;

    file_index = acquisition.get_frame_offset_in_file(7000);
    BOOST_CHECK_EQUAL(0, file_index);

    file_index = acquisition.get_frame_offset_in_file(7549);
    BOOST_CHECK_EQUAL(549, file_index);

    acquisition.concurrent_rank_ = 0;
    acquisition.concurrent_processes_ = 4;
    acquisition.frames_per_block_ = 100;
    acquisition.blocks_per_file_ = 2;

    file_index = acquisition.get_frame_offset_in_file(23);
    BOOST_CHECK_EQUAL(23, file_index);

    file_index = acquisition.get_frame_offset_in_file(464);
    BOOST_CHECK_EQUAL(164, file_index);

    file_index = acquisition.get_frame_offset_in_file(801);
    BOOST_CHECK_EQUAL(1, file_index);

    acquisition.concurrent_rank_ = 1;

    file_index = acquisition.get_frame_offset_in_file(151);
    BOOST_CHECK_EQUAL(51, file_index);

    file_index = acquisition.get_frame_offset_in_file(599);
    BOOST_CHECK_EQUAL(199, file_index);

    acquisition.concurrent_rank_ = 3;

    file_index = acquisition.get_frame_offset_in_file(1141);
    BOOST_CHECK_EQUAL(41, file_index);
}

BOOST_AUTO_TEST_CASE(AcquisitionAdjustFrameOffset)
{
    FrameProcessor::Acquisition acquisition(hdf5_error_definition);
    std::shared_ptr<FrameProcessor::DataBlockFrame> frame = get_dummy_frame();
    frame->meta_data().set_frame_offset(0);

    size_t adjusted_offset = acquisition.adjust_frame_offset(frame);
    BOOST_CHECK_EQUAL(0, adjusted_offset);

    frame->set_frame_number(1);
    adjusted_offset = acquisition.adjust_frame_offset(frame);
    BOOST_CHECK_EQUAL(1, adjusted_offset);

    frame->set_frame_number(0);
    frame->meta_data().set_frame_offset(1);
    adjusted_offset = acquisition.adjust_frame_offset(frame);
    BOOST_CHECK_EQUAL(1, adjusted_offset);

    frame->set_frame_number(1);
    frame->meta_data().set_frame_offset(1);
    adjusted_offset = acquisition.adjust_frame_offset(frame);
    BOOST_CHECK_EQUAL(2, adjusted_offset);

    frame->set_frame_number(1);
    frame->meta_data().set_frame_offset(-1);
    adjusted_offset = acquisition.adjust_frame_offset(frame);
    BOOST_CHECK_EQUAL(0, adjusted_offset);

    frame->set_frame_number(0);
    frame->meta_data().set_frame_offset(-1);
    BOOST_CHECK_THROW(acquisition.adjust_frame_offset(frame), std::range_error);
}

BOOST_AUTO_TEST_CASE(AcquisitionFrameVerification)
{
    FrameProcessor::Acquisition acquisition(hdf5_error_definition);
    FrameProcessor::DatasetDefinition dset_def;
    // Provide default values for the dataset
    dset_def.name = "raw";
    dset_def.data_type = FrameProcessor::raw_16bit;
    dset_def.compression = FrameProcessor::no_compression;
    dset_def.blosc_compressor = 0;
    dset_def.blosc_level = 0;
    dset_def.blosc_shuffle = 0;
    dset_def.num_frames = 1;
    std::vector<long long unsigned int> dims(2, 0);
    dset_def.frame_dimensions = dims;
    dset_def.chunks = dims;
    dset_def.create_low_high_indexes = false;

    acquisition.dataset_defs_["raw"] = dset_def;

    std::shared_ptr<FrameProcessor::DataBlockFrame> frame = get_dummy_frame();
    frame->meta_data().set_compression_type(FrameProcessor::unknown_compression);

    bool verified = acquisition.check_frame_valid(frame);
    BOOST_CHECK_EQUAL(false, verified);

    frame = get_dummy_frame();
    frame->meta_data().set_data_type(FrameProcessor::raw_unknown);

    verified = acquisition.check_frame_valid(frame);
    BOOST_CHECK_EQUAL(false, verified);
}

BOOST_AUTO_TEST_SUITE_END();
