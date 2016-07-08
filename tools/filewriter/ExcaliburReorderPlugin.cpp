/*
 * ExcaliburReorderPlugin.cpp
 *
 *  Created on: 6 Jun 2016
 *      Author: gnx91527
 */

#include <ExcaliburReorderPlugin.h>

namespace filewriter
{

  const std::string ExcaliburReorderPlugin::CONFIG_ASIC_COUNTER_DEPTH = "bitdepth";
  const std::string ExcaliburReorderPlugin::BIT_DEPTH[4] = {"1-bit", "6-bit", "12-bit", "24-bit"};

  /**
   * The constructor sets up logging used within the class.
   */
  ExcaliburReorderPlugin::ExcaliburReorderPlugin() :
      gAsicCounterDepth_(DEPTH_12_BIT)
  {
    // Setup logging for the class
    logger_ = Logger::getLogger("ExcaliburReorderPlugin");
    logger_->setLevel(Level::getAll());
    LOG4CXX_TRACE(logger_, "ExcaliburReorderPlugin constructor.");
  }

  /**
   * Destructor.
   */
  ExcaliburReorderPlugin::~ExcaliburReorderPlugin()
  {
    LOG4CXX_TRACE(logger_, "ExcaliburReorderPlugin destructor.");
  }

  /**
   * Configure the Excalibur plugin.  This receives an IpcMessage which should be processed
   * to configure the plugin, and any response can be added to the reply IpcMessage.  This
   * plugin supports the following configuration parameters:
   * - bitdepth
   *
   * \param[in] config - Reference to the configuration IpcMessage object.
   * \param[out] reply - Reference to the reply IpcMessage object.
   */
  void ExcaliburReorderPlugin::configure(FrameReceiver::IpcMessage& config, FrameReceiver::IpcMessage& reply)
  {
    if (config.has_param(ExcaliburReorderPlugin::CONFIG_ASIC_COUNTER_DEPTH)){
      std::string sBitDepth = config.get_param<std::string>(ExcaliburReorderPlugin::CONFIG_ASIC_COUNTER_DEPTH);
      if (sBitDepth == BIT_DEPTH[DEPTH_1_BIT]){
        gAsicCounterDepth_ = DEPTH_1_BIT;
      } else if (sBitDepth == BIT_DEPTH[DEPTH_6_BIT]){
        gAsicCounterDepth_ = DEPTH_6_BIT;
      } else if (sBitDepth == BIT_DEPTH[DEPTH_12_BIT]){
        gAsicCounterDepth_ = DEPTH_12_BIT;
      } else if (sBitDepth == BIT_DEPTH[DEPTH_24_BIT]){
        gAsicCounterDepth_ = DEPTH_24_BIT;
      } else {
        LOG4CXX_ERROR(logger_, "Invalid bit depth requested: " << sBitDepth);
        throw std::runtime_error("Invalid bit depth requested");
      }
    }
  }

  /**
   * Collate status information for the plugin.  The status is added to the status Value object.
   *
   * \param[out] status - Reference to a RapidJSON generic value to store the status.
   * \param[in] allocator - Allocator used for constructing RapidJSON values.
   */
  void ExcaliburReorderPlugin::status(FrameReceiver::IpcMessage& status)
  {
    // Record the plugin's status items
    LOG4CXX_DEBUG(logger_, "Status requested for Excalibur plugin");
    status.set_param(getName() + "/bitdepth", BIT_DEPTH[gAsicCounterDepth_]);
  }

  /**
   * Perform processing on the frame.  Depending on the selected bit depth
   * the corresponding pixel re-ordering algorithm is executed.
   *
   * \param[in] frame - Pointer to a Frame object.
   */
  void ExcaliburReorderPlugin::processFrame(boost::shared_ptr<Frame> frame)
  {
    LOG4CXX_TRACE(logger_, "Reordering frame.");
    LOG4CXX_TRACE(logger_, "Frame data size: " << frame->get_data_size());

    static void* reorderedPartImageC1;

    // TODO: Current hack to get this to compile.
    // Where does gFramesReceived get reset/set from???
    int gFramesReceived = 0;

    // Allocate buffer to receive reordered image;
    void* reorderedImage = NULL;

    // Reorder image according to counter depth
    switch (gAsicCounterDepth_)
    {
    case DEPTH_1_BIT: // 1-bit counter depth
      reorderedImage = (void *)malloc(frame->get_data_size());
      memcpy(reorderedImage, (void*)(frame->get_data()), (frame->get_data_size()));
      reorder1BitImage((unsigned int*)(frame->get_data()), (unsigned char *)reorderedImage);
      break;

    case DEPTH_6_BIT: // 6-bit counter depth
      reorderedImage = (void *)malloc(frame->get_data_size());
      reorder6BitImage((unsigned char *)(frame->get_data()), (unsigned char *)reorderedImage);
      break;

    case DEPTH_12_BIT: // 12-bit counter depth
      reorderedImage = (void *)malloc(frame->get_data_size());
      reorder12BitImage((unsigned short *)(frame->get_data()), (unsigned short*)reorderedImage);
      break;

    case DEPTH_24_BIT: // 24-bit counter depth - needs special handling to merge successive frames

  #if 1
      if (gFramesReceived == 1)
      {
        // First frame contains C1 data, so allocate space, reorder and store for later use
        reorderedPartImageC1 = (void *)malloc(frame->get_data_size());

        reorder12BitImage((unsigned short *)(frame->get_data()), (unsigned short*)reorderedPartImageC1);
        reorderedImage = 0;  // No buffer to write to file or release
      }
      else
      {
        // Second frame contains C0 data, allocate space for this and for output image (32bit)
        void* reorderedPartImageC0 = (void *)malloc(frame->get_data_size());

        // TODO: Where does this come from
        //size_t outputImageSize = buffer->sizeX * buffer->sizeY * buffer->sizeZ * 4;
        size_t outputImageSize = 0; // !!! big hack to compile
        reorderedImage = (void *)malloc(outputImageSize);

        // Reorder received buffer into C0
        reorder12BitImage((unsigned short *)(frame->get_data()), (unsigned short*)reorderedPartImageC0);

        // Build 24 bit image into output buffer
        build24BitImage((unsigned short *)reorderedPartImageC0, (unsigned short *)reorderedPartImageC1,
            (unsigned int*)reorderedImage);

        // Free the partial image buffers as no longer needed
        free(reorderedPartImageC0);
        free(reorderedPartImageC1);
      }
  #endif
      break;
    }

    // Set the frame image to the reordered image buffer if appropriate
    if (reorderedImage){
      // Assume our image dimensions are:
      size_t bytes_per_pixel = 2;
      dimensions_t dims(2); dims[0] = 2048; dims[1] = 256;

      boost::shared_ptr<Frame> data_frame;
      data_frame = boost::shared_ptr<Frame>(new Frame("data"));
      data_frame->set_frame_number(0);
      data_frame->set_dimensions("frame", dims);
      data_frame->copy_data(reorderedImage, frame->get_data_size());
//      data_frame->copy_data(frame->get_data(), frame->get_data_size());
      LOG4CXX_TRACE(logger_, "Pushing data frame.");
      this->push(data_frame);
      free(reorderedImage);
      reorderedImage = NULL;
    }
  }

  /**
   * Reorder the image using 1 bit re-ordering.
   * 1 bit images are captured in raw data mode, i.e. without reordering. In this mode, each
   * 32-bit word contains the current pixel being output on each data line of the group of
   * 4 ASICs, i.e. a supercolumn
   *
   * \param[in] in - Pointer to the incoming image data.
   * \param[out] out - Pointer to the allocated memory where the reordered image is written.
   */
  void ExcaliburReorderPlugin::reorder1BitImage(unsigned int* in, unsigned char* out)
  {
    int block, y, x, x2, chip, pixelX, pixelY, pixelAddr, bitPosn;
    int rawAddr = 0;

    // Loop over two blocks of data
    for (block = 0; block < FEM_BLOCKS_PER_STRIPE_X; block++)
    {
      // Loop over Y axis (rows)
      for (y = 0; y < FEM_PIXELS_PER_CHIP_Y; y++)
      {
        pixelY = 255 - y;
        // Loop over pixels in a supercolumn
        for (x = 0; x < FEM_PIXELS_PER_SUPERCOLUMN_X; x++)
        {
          // Loop over chips in x per block
          for (chip = 0; chip < FEM_CHIPS_PER_BLOCK_X; chip++)
          {
            // Loop over supercolumns per chip
            for (x2 = 0; x2 < FEM_SUPERCOLUMNS_PER_CHIP; x2++)
            {
              pixelX = (block*(FEM_PIXELS_PER_CHIP_X*FEM_CHIPS_PER_STRIPE_X/2)) +
                   (chip * FEM_PIXELS_PER_CHIP_X) +
                   (255 - ((x2 * FEM_PIXELS_PER_SUPERCOLUMN_X) + x));
              pixelAddr = pixelX + pixelY*(FEM_PIXELS_PER_CHIP_X*FEM_CHIPS_PER_STRIPE_X);
              bitPosn = (chip * 8) + x2;
              out[pixelAddr] = (in[rawAddr] >> bitPosn) & 0x1;
            }
          }
          rawAddr++;
        }
      }
    }
  }

  /**
   * Reorder the image using 6 bit re-ordering.
   *
   * \param[in] in - Pointer to the incoming image data.
   * \param[out] out - Pointer to the allocated memory where the reordered image is written.
   */
  void ExcaliburReorderPlugin::reorder6BitImage(unsigned char* in, unsigned char* out)
  {
    int block, y, x, chip, x2, pixelX, pixelY, pixelAddr;
    int rawAddr = 0;

    for(block=0; block<FEM_BLOCKS_PER_STRIPE_X; block++)
    {
      for(y=0; y<FEM_PIXELS_PER_CHIP_Y; y+=2)
      {
        for(x=0; x<FEM_PIXELS_PER_CHIP_X/FEM_PIXELS_IN_GROUP_6BIT; x++)
        {
          for(chip=0; chip<FEM_CHIPS_PER_BLOCK_X; chip++)
          {
            for(x2=0; x2<FEM_PIXELS_IN_GROUP_6BIT; x2++)
            {
              pixelX = (block*(FEM_PIXELS_PER_CHIP_X*FEM_CHIPS_PER_STRIPE_X/2) +
                  chip*FEM_PIXELS_PER_CHIP_X + (255-(x2 + x*FEM_PIXELS_IN_GROUP_6BIT)));
              pixelY = 254-y;
              pixelAddr = pixelX + pixelY*(FEM_PIXELS_PER_CHIP_X*FEM_CHIPS_PER_STRIPE_X);
              out[pixelAddr] = in[rawAddr];
              rawAddr++;
              pixelY = 255-y;
              pixelAddr = pixelX + pixelY*(FEM_PIXELS_PER_CHIP_X*FEM_CHIPS_PER_STRIPE_X);
              out[pixelAddr] = in[rawAddr];
              rawAddr++;
            }
          }
        }
      }
    }
  }

  /**
   * Reorder the image using 12 bit re-ordering.
   *
   * \param[in] in - Pointer to the incoming image data.
   * \param[out] out - Pointer to the allocated memory where the reordered image is written.
   */
  void ExcaliburReorderPlugin::reorder12BitImage(unsigned short* in, unsigned short* out)
  {
    int block, y, x, chip, x2, pixelX, pixelY, pixelAddr;
    int rawAddr = 0;

    for(block=0; block<FEM_BLOCKS_PER_STRIPE_X; block++)
    {
      for(y=0; y<FEM_PIXELS_PER_CHIP_Y; y++)
      {
        for(x=0; x<FEM_PIXELS_PER_CHIP_X/FEM_PIXELS_IN_GROUP_12BIT; x++)
        {
          for(chip=0; chip<FEM_CHIPS_PER_BLOCK_X; chip++)
          {
            for(x2=0; x2<FEM_PIXELS_IN_GROUP_12BIT; x2++)
            {
              pixelX = (block*(FEM_PIXELS_PER_CHIP_X*FEM_CHIPS_PER_STRIPE_X/2) +
                       chip*FEM_PIXELS_PER_CHIP_X + (255-(x2 + x*FEM_PIXELS_IN_GROUP_12BIT)));
              pixelY = 255-y;
              pixelAddr = pixelX + pixelY*(FEM_PIXELS_PER_CHIP_X*FEM_CHIPS_PER_STRIPE_X);
              out[pixelAddr] = in[rawAddr];
              rawAddr++;
              //LOG4CXX_TRACE(logger_, "Pixel Address: " << pixelAddr);
              //LOG4CXX_TRACE(logger_, "Raw Address: " << rawAddr);
            }
          }
        }
      }
    }
  }

  /**
   * Build a 24bit image from two images.
   *
   * \param[in] inC0 - Pointer to the incoming first image data.
   * \param[in] inC1 - Pointer to the incoming second image data.
   * \param[out] out - Pointer to the allocated memory where the combined image is written.
   */
  void ExcaliburReorderPlugin::build24BitImage(unsigned short* inC0, unsigned short* inC1, unsigned int* out)
  {
    int addr;
    for (addr = 0; addr < FEM_TOTAL_PIXELS; addr++)
    {
      out[addr] = (((unsigned int)(inC1[addr] & 0xFFF)) << 12) | (inC0[addr] & 0xFFF);
    }
  }

} /* namespace filewriter */
