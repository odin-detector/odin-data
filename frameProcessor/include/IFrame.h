#ifndef FRAMEPROCESSOR_IFRAME_H
#define FRAMEPROCESSOR_IFRAME_H

#include <string>
#include <log4cxx/logger.h>

#include <stdint.h>

#include "IFrameMetaData.h"

namespace FrameProcessor {

/** Interface class for a Frame; all Frames must sub-class this.
 *
 */
    class IFrame {

    public:

        /** Base constructor */
        IFrame(const IFrameMetaData &meta_data, const int &image_offset = 0);

        /** Shallow-copy copy */
        IFrame(const IFrame &frame);

        /** Deep-copy assignment */
        IFrame &operator=(IFrame &frame);

        /** Return a void pointer to the raw data */
        virtual void *get_data_ptr() const = 0;

        /** Return a void pointer to the image data */
        virtual void *get_image_ptr() const;

        /** Return the data size */
        virtual size_t get_data_size() const = 0;

        /** Return the MetaData */
        IFrameMetaData get_meta_data() const;

        /** Return the image offset */
        int get_image_offset() const;

        /** Set MetaData */
        void set_meta_data(const IFrameMetaData &meta_data);

        /** Set the image offset */
        void set_image_offset(const int &offset);

    protected:

        /** Pointer to logger */
        log4cxx::LoggerPtr logger;

        /** Frame MetaData */
        IFrameMetaData meta_data_;

        /** Offset from frame memory start to image data */
        int image_offset_;

    };

}

#endif
