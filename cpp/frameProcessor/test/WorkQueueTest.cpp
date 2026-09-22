#define BOOST_TEST_MODULE "WorkQueueTests"
#define BOOST_TEST_MAIN

#include "WorkQueue.h"
#include "Fixtures.h"
#include "TestHelperFunctions.h"

BOOST_GLOBAL_FIXTURE(GlobalConfig);

// BOOST_FIXTURE_TEST_SUITE(WorkQueueUnitTest, WorkQueueTestFixture);

BOOST_AUTO_TEST_CASE(WorkQueue_PushRemoveTest)
{
    FrameProcessor::WorkQueue<int> wq;
    wq.add(5);
    wq.add(4);
    wq.add(3);
    wq.add(2);
    wq.add(1);
    BOOST_CHECK(wq.remove() == 5);
    BOOST_CHECK(wq.remove() == 4);
    BOOST_CHECK(wq.remove() == 3);
    BOOST_CHECK(wq.remove() == 2);
    BOOST_CHECK(wq.remove() == 1);
}

BOOST_AUTO_TEST_CASE(WorkQueue_FramesTest)
{
    constexpr size_t no_of_frames = 6;

    FrameProcessor::WorkQueue<boost::shared_ptr<FrameProcessor::Frame>> wq;
    static_assert(no_of_frames < wq.max_size());
    unsigned int img[12] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
    std::vector<boost::shared_ptr<FrameProcessor::Frame>> frames;
    frames.reserve(no_of_frames);
    for (unsigned int i = 0; i < no_of_frames; ++i) {
        img[0] = i;
        boost::shared_ptr<FrameProcessor::DataBlockFrame> tmp_frame {
            boost::make_shared<FrameProcessor::DataBlockFrame>(
                FrameProcessor::FrameMetaData(
                    i, "data", FrameProcessor::raw_32bit, "scan2", { 3, 4 }, FrameProcessor::no_compression
                ),
                static_cast<void*>(img), 24
            )
        };
        frames.push_back(tmp_frame);
        wq.add(frames.back());
    }
    BOOST_CHECK(wq.size() == no_of_frames);
    for (int i = 0; i < no_of_frames; ++i) {
        BOOST_CHECK(wq.remove()->get_meta_data().get_frame_number() == frames[i]->get_meta_data().get_frame_number());
    }
}
