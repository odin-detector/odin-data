#define BOOST_TEST_MODULE "FileWriterPluginTests"
#define BOOST_TEST_MAIN

#include "Fixtures.h"

BOOST_TEST_GLOBAL_FIXTURE(GlobalConfig);

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

BOOST_AUTO_TEST_SUITE_END();
