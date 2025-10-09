/*
 * GapFillPlugin.cpp
 *
 *  Created on: 5 Feb 2019
 *      Author: Alan Greer
 */
#include "GapFillPlugin.h"
#include "version.h"
#include "DebugLevelLogger.h"
#include <boost/algorithm/string.hpp>

#include "DataBlockFrame.h"

namespace FrameProcessor
{
    /* Config Names*/
    const std::string GapFillPlugin::CONFIG_GRID_SIZE = "grid_size";
    const std::string GapFillPlugin::CONFIG_CHIP_SIZE = "chip_size";
    const std::string GapFillPlugin::CONFIG_GRID_X_GAPS = "x_gaps";
    const std::string GapFillPlugin::CONFIG_GRID_Y_GAPS = "y_gaps";

    /**
     * Constructor for this class.
     */
    GapFillPlugin::GapFillPlugin()
    {
        logger_ = Logger::getLogger("FP.GapFillPlugin");
        LOG4CXX_INFO(logger_, "GapFillPlugin version " << this->get_version_long() << " loaded");
    }

    GapFillPlugin::~GapFillPlugin()
    {
        // Nothing to do in the destructor
    }

    /**
     * Process received frame.
     *
     * \param[in] frame - pointer to a frame object.
     */
    void GapFillPlugin::process_frame(boost::shared_ptr<Frame> frame)
    {
        LOG4CXX_TRACE(logger_, "GapFillPlugin Process Frame.");

        // Call the insert gaps method and push the resulting frame if it is not null
        if (this->configuration_valid(frame)){
            boost::shared_ptr<Frame> gap_frame = this->insert_gaps(frame);
            if (gap_frame){
                this->push(gap_frame);
            }
        }
    }

    /**
     * Check the validity of the configuration against the incoming frame.
     *
     * Checks the frame size is equal to the grid size X chip size in both directions.
     * Checks the number of gaps correspond to the specified grid size.
     *
     * \param[in] frame - pointer to a frame object.
     * \return verified - true if the configuration is valid, false otherwise.
     */
    bool GapFillPlugin::configuration_valid(boost::shared_ptr<Frame> frame)
    {
        bool verified = false;
        dimensions_t frame_dimensions = frame->get_meta_data().get_dimensions();
        // Check the size of the incoming image is correct for the defined grid and chip size
        if (frame_dimensions[0] != grid_[0] * chip_[0]){
            std::stringstream ss;
            ss << "GapFill - Inconsistent frame dimension[0] => "
               << frame_dimensions[0]
               << " compared with (grid[0] x chip[0]) => "
               << grid_[0] * chip_[0];
            this->set_error(ss.str());
        } else if (frame_dimensions[1] != grid_[1] * chip_[1]){
            std::stringstream ss;
            ss << "GapFill - Inconsistent frame dimension[1] => "
               << frame_dimensions[1]
               << " compared with (grid[1] x chip[1]) => "
               << grid_[1] * chip_[1];
            this->set_error(ss.str());
        } else if (grid_[1]+1 > gaps_x_.size()){
            std::stringstream ss;
            ss << "GapFill - Grid size [1] => "
               << grid_[1]
               << " not enough x gap values speficied (should be "
               << grid_[1]+1 << ")";
            this->set_error(ss.str());
        } else if (grid_[0]+1 > gaps_y_.size()){
            std::stringstream ss;
            ss << "GapFill - Grid size [0] => "
               << grid_[0]
               << " not enough y gap values speficied (should be "
               << grid_[0]+1 << ")";
            this->set_error(ss.str());
        } else if (grid_[1]+1 < gaps_x_.size()){
            std::stringstream ss;
            ss << "GapFill - Grid size [1] => "
               << grid_[1]
               << " too many x gap values speficied (should be "
               << grid_[1]+1 << ")";
            this->set_error(ss.str());
        } else if (grid_[0]+1 < gaps_y_.size()){
            std::stringstream ss;
            ss << "GapFill - Grid size [0] => "
               << grid_[0]
               << " too many y gap values speficied (should be "
               << grid_[0]+1 << ")";
            this->set_error(ss.str());
        } else {
            verified = true;
        }
        return verified;
    }

    /**
     * Insert gaps into the frame according to the grid, gap_x and gap_y values.
     *
     * A series of copies is made copying each row of each chip into the appropriate
     * destination offset.
     *
     * \param[in] frame - pointer to a frame object.
     * \return gap_frame - pointer to a frame that has gaps inserted.
     */
    boost::shared_ptr<Frame> GapFillPlugin::insert_gaps(boost::shared_ptr<Frame> frame) {

        boost::shared_ptr<Frame> gap_frame;

        dimensions_t frame_dimensions = frame->get_meta_data().get_dimensions();

        int img_x = grid_[1] * chip_[1];
        for (int index = 0; index <= grid_[1]; index++){
            img_x += gaps_x_[index];
        }
        LOG4CXX_TRACE(logger_, "New image width: " << img_x);
        int img_y = grid_[0] * chip_[0];
        for (int index = 0; index <= grid_[0]; index++){
            img_y += gaps_y_[index];
        }
        LOG4CXX_TRACE(logger_, "New image height: " << img_y);

        size_t frame_data_type_size = get_size_from_enum(frame->get_meta_data().get_data_type());
        // Malloc the required memory
        void *new_image = malloc(img_x * img_y * frame_data_type_size);
        // Memset to ensure empty values
        memset(new_image, 0, img_x * img_y * frame_data_type_size);

        // Loop over the y grid
        int current_offset_y = 0;
        for (int y_index = 0; y_index < grid_[0]; y_index++) {
            current_offset_y += gaps_y_[y_index];
            // Loop over the individual y rows for the grid item
            for (int y_row = 0; y_row < chip_[0]; y_row++) {
                int current_offset_x = 0;
                int current_src_row = (y_index * chip_[0]) + y_row;
                int current_dest_row = current_src_row + current_offset_y;
                // Loop over the x grid
                for (int x_index = 0; x_index < grid_[1]; x_index++) {
                    // Calculate the current total x gap in pixels
                    current_offset_x += gaps_x_[x_index];

                    // src offset is the src row multiplied by the width + the current grid multiplied by chip width
                    int src_offset = (current_src_row * frame_dimensions[1]) + (x_index * chip_[1]);
                    // Multiply dest_offset by the data type size
                    src_offset *= frame_data_type_size;

                    char *src_ptr = (char *)frame->get_image_ptr();
                    src_ptr += src_offset;

                    // Dest offset is full width * (current_y_gap + (y_grid_index * chip_y) + y_row)
                    int dest_offset = (current_dest_row * img_x) + current_offset_x + (x_index * chip_[1]);
                    // Multiply dest_offset by the data type size
                    dest_offset *= frame_data_type_size;

                    char *dest_ptr = (char *)new_image;
                    dest_ptr += dest_offset;
                    // Now copy a single row of a single chip from the src to the destination
                    memcpy(dest_ptr, src_ptr, chip_[1] * frame_data_type_size);
                }
            }
        }
        // Create the return frame
        dimensions_t img_dims(2);
        img_dims[0] = img_y;
        img_dims[1] = img_x;


       FrameProcessor::FrameMetaData frame_meta = frame->get_meta_data_copy();
       frame_meta.set_dimensions(img_dims);

       gap_frame = boost::shared_ptr<FrameProcessor::DataBlockFrame>(
                new FrameProcessor::DataBlockFrame(frame_meta, static_cast<void*>(new_image), img_x * img_y * get_size_from_enum(frame->get_meta_data().get_data_type())));

        // Free the allocated memory
        free(new_image);
        return gap_frame;
    }

    /**
     * Set configuration options for this Plugin.
     *
     * This sets up the Live View Plugin according to the configuration IpcMessage
     * objects that are received.
     *
     * \param[in] config - IpcMessage containing configuration data.
     * \param[out] reply - Response IpcMessage.
     */
    void GapFillPlugin::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
    {
        try{
            // Check if the grid size is being set
            if (config.has_param(CONFIG_GRID_SIZE)){
                const rapidjson::Value& val = config.get_param<const rapidjson::Value&>(CONFIG_GRID_SIZE);
                // Loop over the grid size values
                grid_.clear();
                grid_.resize(val.Size());
                for (rapidjson::SizeType i = 0; i < val.Size(); i++) {
                    const rapidjson::Value& dim = val[i];
                    grid_[i] = dim.GetInt();
                }
                LOG4CXX_DEBUG_LEVEL(1, logger_, "Grid size set to [" << grid_[0] << " x " << grid_[1] << "]");
            }

            // Check if the chip size is being set
            if (config.has_param(CONFIG_CHIP_SIZE)){
                const rapidjson::Value& val = config.get_param<const rapidjson::Value&>(CONFIG_CHIP_SIZE);
                // Loop over the chip size values
                chip_.clear();
                chip_.resize(val.Size());
                for (rapidjson::SizeType i = 0; i < val.Size(); i++) {
                    const rapidjson::Value& dim = val[i];
                    chip_[i] = dim.GetInt();
                }
                LOG4CXX_DEBUG_LEVEL(1, logger_, "Chip size set to [" << chip_[0] << " x " << chip_[1] << "].");
            }

            // Check if the X grid gaps are being set
            if (config.has_param(CONFIG_GRID_X_GAPS)){
                const rapidjson::Value& val = config.get_param<const rapidjson::Value&>(CONFIG_GRID_X_GAPS);
                // Loop over the chip size values
                gaps_x_.clear();
                gaps_x_.resize(val.Size());
                std::stringstream ss;
                for (rapidjson::SizeType i = 0; i < val.Size(); i++) {
                    const rapidjson::Value& gap = val[i];
                    gaps_x_[i] = gap.GetInt();
                    ss << gaps_x_[i];
                    if (i < val.Size() - 1){
                        ss << ", ";
                    }
                }
                LOG4CXX_DEBUG_LEVEL(1, logger_, "X Gaps set to [" << ss.str() << "].");
            }

            // Check if the Y grid gaps are being set
            if (config.has_param(CONFIG_GRID_Y_GAPS)){
                const rapidjson::Value& val = config.get_param<const rapidjson::Value&>(CONFIG_GRID_Y_GAPS);
                // Loop over the chip size values
                gaps_y_.clear();
                gaps_y_.resize(val.Size());
                std::stringstream ss;
                for (rapidjson::SizeType i = 0; i < val.Size(); i++) {
                    const rapidjson::Value& gap = val[i];
                    gaps_y_[i] = gap.GetInt();
                    ss << gaps_y_[i];
                    if (i < val.Size() - 1){
                        ss << ", ";
                    }
                }
                LOG4CXX_DEBUG_LEVEL(1, logger_, "Y Gaps set to [" << ss.str() << "].");
            }

        }
        catch (std::runtime_error& e)
        {
            std::stringstream ss;
            ss << "Bad ctrl msg: " << e.what();
            this->set_error(ss.str());
            throw;
        }
    }

    int GapFillPlugin::get_version_major()
    {
        return ODIN_DATA_VERSION_MAJOR;
    }

    int GapFillPlugin::get_version_minor()
    {
        return ODIN_DATA_VERSION_MINOR;
    }

    int GapFillPlugin::get_version_patch()
    {
        return ODIN_DATA_VERSION_PATCH;
    }

    std::string GapFillPlugin::get_version_short()
    {
        return ODIN_DATA_VERSION_STR_SHORT;
    }

    std::string GapFillPlugin::get_version_long()
    {
        return ODIN_DATA_VERSION_STR;
    }

    /**
     * Get the configuration values for this Plugin.
     *
     * \param[out] reply - Response IpcMessage.
     */
    void GapFillPlugin::requestConfiguration(OdinData::IpcMessage& reply)
    {
        for (int index = 0; index < grid_.size(); index++) {
            reply.set_param(get_name() + '/' + CONFIG_GRID_SIZE + "[]", grid_[index]);
        }
        for (int index = 0; index < chip_.size(); index++) {
            reply.set_param(get_name() + '/' + CONFIG_CHIP_SIZE + "[]", chip_[index]);
        }
        for (int index = 0; index < gaps_x_.size(); index++) {
            reply.set_param(get_name() + '/' + CONFIG_GRID_X_GAPS + "[]", gaps_x_[index]);
        }
        for (int index = 0; index < gaps_y_.size(); index++) {
            reply.set_param(get_name() + '/' + CONFIG_GRID_Y_GAPS + "[]", gaps_y_[index]);
        }
    }

} //namespace FrameProcessor

