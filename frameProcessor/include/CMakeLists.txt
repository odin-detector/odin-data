# Install header files into installation prefix

SET(HEADERS DataBlock.h
            DataBlockPool.h
            FrameMetaData.h
            Frame.h
            SharedBufferFrame.h
            DataBlockFrame.h
            FrameProcessorPlugin.h
            FileWriterPlugin.h
            IFrameCallback.h
            MetaMessage.h
            MetaMessagePublisher.h
            WorkQueue.h
            CallDuration.h
            WatchdogTimer.h
            HDF5File.h
            Acquisition.h
            FrameProcessorDefinitions.h
            BloscPlugin.h
            SumPlugin.h
            GapFillPlugin.h
            KafkaProducerPlugin.h)

INSTALL(FILES ${HEADERS} DESTINATION include/frameProcessor)
