/*
 * ExcaliburReorderPlugin.cpp
 *
 *  Created on: 6 Jun 2016
 *      Author: gnx91527
 */

#include <ExcaliburReorderPlugin.h>

namespace filewriter
{

  ExcaliburReorderPlugin::ExcaliburReorderPlugin() :
      gAsicCounterDepth_(0)
  {
    // Setup logging for the class
    logger_ = Logger::getLogger("ExcaliburReorderPlugin");
    logger_->setLevel(Level::getAll());
    LOG4CXX_TRACE(logger_, "ExcaliburReorderPlugin constructor.");
  }

  ExcaliburReorderPlugin::~ExcaliburReorderPlugin()
  {
    LOG4CXX_TRACE(logger_, "ExcaliburReorderPlugin destructor.");
  }

  void ExcaliburReorderPlugin::processFrame(boost::shared_ptr<Frame> frame)
  {
    LOG4CXX_TRACE(logger_, "Reordering frame.");

    static void* reorderedPartImageC1;

    // TODO: Current hack to get this to compile.
    // Where does gFramesReceived get reset/set from???
    int gFramesReceived = 0;

    // Allocate buffer to receive reordered image;
    void* reorderedImage = NULL;

    // Reorder image according to counter depth
    switch (gAsicCounterDepth_)
    {
    case 0: // 1-bit counter depth
      reorderedImage = (void *)malloc(frame->get_data_size());
      memcpy(reorderedImage, (void*)(frame->get_data()), (frame->get_data_size()));
      reorder1BitImage((unsigned int*)(frame->get_data()), (unsigned char *)reorderedImage);
      break;

    case 1: // 6-bit counter depth
      reorderedImage = (void *)malloc(frame->get_data_size());
      reorder6BitImage((unsigned char *)(frame->get_data()), (unsigned char *)reorderedImage);
      break;

    case 2: // 12-bit counter depth
      reorderedImage = (void *)malloc(frame->get_data_size());
      reorder12BitImage((unsigned short *)(frame->get_data()), (unsigned short*)reorderedImage);
      break;

    case 3: // 24-bit counter depth - needs special handling to merge successive frames

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
      frame->copy_data(reorderedImage, frame->get_data_size());
      free(reorderedImage);
      reorderedImage = NULL;
    }
  }

  void ExcaliburReorderPlugin::reorder1BitImage(unsigned int* in, unsigned char* out)
  {
    // 1 bit images are captured in raw data mode, i.e. without reordering. In this mode, each
    // 32-bit word contains the current pixel being output on each data line of the group of
    // 4 ASICs, i.e. a supercolumn

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
            }
          }
        }
      }
    }
  }

  void ExcaliburReorderPlugin::build24BitImage(unsigned short* inC0, unsigned short* inC1, unsigned int* out)
  {
    int addr;
    for (addr = 0; addr < FEM_TOTAL_PIXELS; addr++)
    {
      out[addr] = (((unsigned int)(inC1[addr] & 0xFFF)) << 12) | (inC0[addr] & 0xFFF);
    }
  }

} /* namespace filewriter */
