#define BOOST_TEST_MODULE "FileWriterPluginTests"
#define BOOST_TEST_MAIN

#include "Fixtures.h"

BOOST_GLOBAL_FIXTURE(GlobalConfig);

BOOST_FIXTURE_TEST_SUITE(FileWriterPluginTestUnitTest, FileWriterPluginTestFixture);

BOOST_AUTO_TEST_CASE(FileWriterPluginCalcNumFramesTest1fpb)
{
    OdinData::IpcMessage reply;
    FrameProcessor::FileWriterPlugin fwp0;
    {
        OdinData::IpcMessage cfg;
        cfg.set_param("process/number", 4);
        cfg.set_param("process/rank", 0);
        cfg.set_param("process/frames_per_block", 1);
        fwp0.configure(cfg, reply);
    }
    FrameProcessor::FileWriterPlugin fwp1;
    {
        OdinData::IpcMessage cfg;
        cfg.set_param("process/number", 4);
        cfg.set_param("process/rank", 1);
        cfg.set_param("process/frames_per_block", 1);
        fwp1.configure(cfg, reply);
    }
    FrameProcessor::FileWriterPlugin fwp2;
    {
        OdinData::IpcMessage cfg;
        cfg.set_param("process/number", 4);
        cfg.set_param("process/rank", 2);
        cfg.set_param("process/frames_per_block", 1);
        fwp2.configure(cfg, reply);
    }
    FrameProcessor::FileWriterPlugin fwp3;
    {
        OdinData::IpcMessage cfg;
        cfg.set_param("process/number", 4);
        cfg.set_param("process/rank", 3);
        cfg.set_param("process/frames_per_block", 1);
        fwp3.configure(cfg, reply);
    }

    size_t num_frames_0 = fwp0.calc_num_frames(1141);
    BOOST_CHECK_EQUAL(286, num_frames_0);
    size_t num_frames_1 = fwp1.calc_num_frames(1141);
    BOOST_CHECK_EQUAL(285, num_frames_1);
    size_t num_frames_2 = fwp2.calc_num_frames(1141);
    BOOST_CHECK_EQUAL(285, num_frames_2);
    size_t num_frames_3 = fwp3.calc_num_frames(1141);
    BOOST_CHECK_EQUAL(285, num_frames_3);

    num_frames_0 = fwp0.calc_num_frames(3);
    BOOST_CHECK_EQUAL(1, num_frames_0);
    num_frames_1 = fwp1.calc_num_frames(3);
    BOOST_CHECK_EQUAL(1, num_frames_1);
    num_frames_2 = fwp2.calc_num_frames(3);
    BOOST_CHECK_EQUAL(1, num_frames_2);
    num_frames_3 = fwp3.calc_num_frames(3);
    BOOST_CHECK_EQUAL(0, num_frames_3);
}

BOOST_AUTO_TEST_CASE(FileWriterPluginCalcNumFramesTest1000fpb)
{
    OdinData::IpcMessage reply;
    FrameProcessor::FileWriterPlugin fwp0;
    {
        OdinData::IpcMessage cfg;
        cfg.set_param("process/number", 4);
        cfg.set_param("process/rank", 0);
        cfg.set_param("process/frames_per_block", 1000);
        fwp0.configure(cfg, reply);
    }
    FrameProcessor::FileWriterPlugin fwp1;
    {
        OdinData::IpcMessage cfg;
        cfg.set_param("process/number", 4);
        cfg.set_param("process/rank", 1);
        cfg.set_param("process/frames_per_block", 1000);
        fwp1.configure(cfg, reply);
    }
    FrameProcessor::FileWriterPlugin fwp2;
    {
        OdinData::IpcMessage cfg;
        cfg.set_param("process/number", 4);
        cfg.set_param("process/rank", 2);
        cfg.set_param("process/frames_per_block", 1000);
        fwp2.configure(cfg, reply);
    }
    FrameProcessor::FileWriterPlugin fwp3;
    {
        OdinData::IpcMessage cfg;
        cfg.set_param("process/number", 4);
        cfg.set_param("process/rank", 3);
        cfg.set_param("process/frames_per_block", 1000);
        fwp3.configure(cfg, reply);
    }

    size_t num_frames_0 = fwp0.calc_num_frames(1141);
    BOOST_CHECK_EQUAL(1000, num_frames_0);
    size_t num_frames_1 = fwp1.calc_num_frames(1141);
    BOOST_CHECK_EQUAL(141, num_frames_1);
    size_t num_frames_2 = fwp2.calc_num_frames(1141);
    BOOST_CHECK_EQUAL(0, num_frames_2);
    size_t num_frames_3 = fwp3.calc_num_frames(1141);
    BOOST_CHECK_EQUAL(0, num_frames_3);

    num_frames_0 = fwp0.calc_num_frames(3);
    BOOST_CHECK_EQUAL(3, num_frames_0);
    num_frames_1 = fwp1.calc_num_frames(3);
    BOOST_CHECK_EQUAL(0, num_frames_1);
    num_frames_2 = fwp2.calc_num_frames(3);
    BOOST_CHECK_EQUAL(0, num_frames_2);
    num_frames_3 = fwp3.calc_num_frames(3);
    BOOST_CHECK_EQUAL(0, num_frames_3);

    num_frames_0 = fwp0.calc_num_frames(33999);
    BOOST_CHECK_EQUAL(9000, num_frames_0);
    num_frames_1 = fwp1.calc_num_frames(33999);
    BOOST_CHECK_EQUAL(8999, num_frames_1);
    num_frames_2 = fwp2.calc_num_frames(33999);
    BOOST_CHECK_EQUAL(8000, num_frames_2);
    num_frames_3 = fwp3.calc_num_frames(33999);
    BOOST_CHECK_EQUAL(8000, num_frames_3);
}

BOOST_AUTO_TEST_CASE(FileWriterPluginZeroFrames)
{
    OdinData::IpcMessage reply;
    FrameProcessor::FileWriterPlugin fwp;
    fwp.set_name("hdf");
    {
        OdinData::IpcMessage cfg;
        cfg.set_param("frames", 1);
        fwp.configure(cfg, reply);

        OdinData::IpcMessage reply2;
        fwp.requestConfiguration(reply2);
        BOOST_CHECK_EQUAL(1, reply2.get_param<int>("hdf/frames"));
    }
    {
        OdinData::IpcMessage cfg;
        cfg.set_param("frames", 0);
        fwp.configure(cfg, reply);

        OdinData::IpcMessage reply2;
        fwp.requestConfiguration(reply2);
        BOOST_CHECK_EQUAL(0, reply2.get_param<int>("hdf/frames"));
    }
}

BOOST_AUTO_TEST_CASE(FileWriterPluginStatusSanityCheck)
{
    OdinData::IpcMessage reply;
    FrameProcessor::FileWriterPlugin fwp;
    using FPFW = FrameProcessor::FileWriterPlugin;
    fwp.set_name("hdf_status");
    BOOST_REQUIRE_NO_THROW(fwp.status(reply));

    std::vector<std::string> names = reply.get_param_names();
    for (auto iter = names.begin(); iter != names.end(); ++iter){
        std::cout << *iter << std::endl;
        BOOST_TEST_MESSAGE(*iter);
    }

    std::string prefix = fwp.get_name();
    OdinData::IpcMessage status(reply.get_param<const rapidjson::Value&>(prefix));
    names = status.get_param_names();
    for (auto iter = names.begin(); iter != names.end(); ++iter){
        BOOST_TEST_MESSAGE(*iter);
    }


//    BOOST_CHECK(status.has_param(FPFW::STATUS_WRITING));

    /*    BOOST_CHECK(reply.has_param(prefix + FPFW::STATUS_FRAMES_MAX));
    BOOST_CHECK(reply.has_param(prefix + FPFW::STATUS_FRAMES_WRITTEN));
    BOOST_CHECK(reply.has_param(prefix + FPFW::STATUS_FRAMES_WRITTEN));
    BOOST_CHECK(reply.has_param(prefix + FPFW::STATUS_FRAMES_PROCESSED));
    BOOST_CHECK(reply.has_param(prefix + FPFW::STATUS_FILE_PATH));
    BOOST_CHECK(reply.has_param(prefix + FPFW::STATUS_FILE_NAME));
    BOOST_CHECK(reply.has_param(prefix + FPFW::STATUS_ACQUISITION_ID));
    BOOST_CHECK(reply.has_param(prefix + FPFW::STATUS_PROCESSES));
    BOOST_CHECK(reply.has_param(prefix + FPFW::STATUS_RANK));
    BOOST_CHECK(reply.has_param(prefix + FPFW::STATUS_TIMEOUT_ACTIVE));

    prefix += FPFW::STATUS_TIMING;
    BOOST_CHECK(reply.has_param(prefix));
    prefix += '/';
    BOOST_CHECK(reply.has_param(prefix + FPFW::STATUS_LAST_CREATE));
    BOOST_CHECK(reply.has_param(prefix + FPFW::STATUS_MAX_CREATE));
    BOOST_CHECK(reply.has_param(prefix + FPFW::STATUS_MEAN_CREATE));
    BOOST_CHECK(reply.has_param(prefix + FPFW::STATUS_LAST_WRITE));
    BOOST_CHECK(reply.has_param(prefix + FPFW::STATUS_MEAN_WRITE));
    BOOST_CHECK(reply.has_param(prefix + FPFW::STATUS_LAST_FLUSH));
    BOOST_CHECK(reply.has_param(prefix + FPFW::STATUS_MAX_FLUSH));
    BOOST_CHECK(reply.has_param(prefix + FPFW::STATUS_MEAN_FLUSH));
    BOOST_CHECK(reply.has_param(prefix + FPFW::STATUS_LAST_CLOSE));
    BOOST_CHECK(reply.has_param(prefix + FPFW::STATUS_MAX_CLOSE));
    BOOST_CHECK(reply.has_param(prefix + FPFW::STATUS_MEAN_CLOSE));
    */
}

BOOST_AUTO_TEST_SUITE_END();
