/*
 * BloscPlugin.cpp
 *
 *  Created on: 22 Jan 2018
 *      Author: Ulrik Pedersen
 */
#include <BloscPlugin.h>
#include <DataBlockFrame.h>
#include <DebugLevelLogger.h>
#include <blosc.h>
#include <cstdlib>
#include <version.h>

#include <boost/make_shared.hpp>
#include <boost/static_assert.hpp>

namespace FrameProcessor {

BOOST_STATIC_ASSERT_MSG(BLOSC_VERSION_FORMAT == BLOSC_FORMAT_ODIN_USES, "Error: Wrong version of blosc library");
using FPB = BloscPlugin;
struct Mode_map {
    using BPM = BloscPlugin::Mode;
    static const std::unordered_map<std::string, const BPM> mode_map;
    static std::string mode_to_str(BPM mode)
    {
        switch (mode) {
        case BPM::COMPRESS:
            return FPB::BLOSC_COMPRESS_MODE_STR;
        case BPM::DECOMPRESS:
            return FPB::BLOSC_DECOMPRESS_MODE_STR;
        case BPM::OFF:
            return FPB::BLOSC_OFF_MODE_STR;
        }
        return FPB::BLOSC_OFF_MODE_STR;
    }
};

const std::unordered_map<std::string, const Mode_map::BPM> Mode_map::mode_map
    = { { FPB::BLOSC_COMPRESS_MODE_STR, Mode_map::BPM::COMPRESS },
        { FPB::BLOSC_DECOMPRESS_MODE_STR, Mode_map::BPM::DECOMPRESS },
        { FPB::BLOSC_OFF_MODE_STR, Mode_map::BPM::OFF } };

const std::string BloscPlugin::CONFIG_BLOSC_COMPRESSOR = "compressor";
const std::string BloscPlugin::CONFIG_BLOSC_THREADS = "threads";
const std::string BloscPlugin::CONFIG_BLOSC_LEVEL = "level";
const std::string BloscPlugin::CONFIG_BLOSC_SHUFFLE = "shuffle";
const std::string BloscPlugin::CONFIG_BLOSC_MODE = "mode";

// hashmap alias

static std::unordered_map<std::string, const unsigned int> shuffle_str2i { { FPB::BLOSC_NOSHUFFLE_STR, 0 },
                                                                           { FPB::BLOSC_SHUFFLE_STR, 1 },
                                                                           { FPB::BLOSC_BITSHUFFLE_STR, 2 } };
static std::unordered_map<std::string, const unsigned int> compressor_str2i {
    { BLOSC_BLOSCLZ_COMPNAME, 0 }, { BLOSC_LZ4_COMPNAME, 1 },  { BLOSC_LZ4HC_COMPNAME, 2 },
    { BLOSC_SNAPPY_COMPNAME, 3 },  { BLOSC_ZLIB_COMPNAME, 4 }, { BLOSC_ZSTD_COMPNAME, 5 }
};

/**
 * cd_values[7] meaning (see blosc.h):
 *   0: reserved
 *   1: reserved
 *   2: type size
 *   3: uncompressed size
 *   4: compression level
 *   5: 0: shuffle not active, 1: byte shuffle, 2: bit shuffle
 *   6: the actual Blosc compressor to use. See blosc.h
 *
 * @param settings
 * @param cd_values
 */
void create_cd_values(const BloscCompressionSettings& settings, std::vector<unsigned int>& cd_values)
{
    if (cd_values.size() < 7)
        cd_values.resize(7);
    cd_values[0] = 0;
    cd_values[1] = 0;
    cd_values[2] = static_cast<unsigned int>(settings.type_size);
    cd_values[3] = static_cast<unsigned int>(settings.uncompressed_size);
    cd_values[4] = settings.compression_level;
    cd_values[5] = shuffle_str2i[settings.shuffle];
    cd_values[6] = compressor_str2i[settings.blosc_compressor];
}

/**
 * The constructor sets up logging used within the class.
 */
BloscPlugin::BloscPlugin() :
    current_acquisition_(""),
    plugin_mode_ { Mode::COMPRESS }
{
    add_config_param_metadata(
        CONFIG_BLOSC_COMPRESSOR, PMDD::STRING_T, PMDA::READ_WRITE,
        { BLOSC_BLOSCLZ_COMPNAME, BLOSC_LZ4_COMPNAME, BLOSC_LZ4HC_COMPNAME, BLOSC_SNAPPY_COMPNAME, BLOSC_ZLIB_COMPNAME,
          BLOSC_ZSTD_COMPNAME }
    );
    add_config_param_metadata(CONFIG_BLOSC_THREADS, PMDD::UINT_T, PMDA::READ_WRITE, 1);
    add_config_param_metadata(CONFIG_BLOSC_LEVEL, PMDD::UINT_T, PMDA::READ_WRITE, 1, 9);
    add_config_param_metadata(
        CONFIG_BLOSC_SHUFFLE, PMDD::STRING_T, PMDA::READ_WRITE,
        { BLOSC_NOSHUFFLE_STR, BLOSC_SHUFFLE_STR, BLOSC_BITSHUFFLE_STR }
    );
    add_config_param_metadata(
        CONFIG_BLOSC_MODE, PMDD::STRING_T, PMDA::READ_WRITE,
        { FPB::BLOSC_COMPRESS_MODE_STR, FPB::BLOSC_DECOMPRESS_MODE_STR, FPB::BLOSC_OFF_MODE_STR }
    );

    this->commanded_compression_settings_.blosc_compressor = BLOSC_LZ4_COMPNAME;
    this->commanded_compression_settings_.shuffle = BLOSC_BITSHUFFLE_STR;
    this->commanded_compression_settings_.compression_level = 1;
    this->commanded_compression_settings_.type_size = 0;
    this->commanded_compression_settings_.uncompressed_size = 0;
    this->commanded_compression_settings_.threads = 1;
    this->compression_settings_ = this->commanded_compression_settings_;

    // Setup logging for the class
    logger_ = Logger::getLogger("FP.BloscPlugin");
    LOG4CXX_TRACE(logger_, "BloscPlugin constructor. Version: " << this->get_version_long());

    int ret = 0;
    ret = blosc_set_compressor(BLOSC_LZ4_COMPNAME);
    if (ret < 0) {
        LOG4CXX_ERROR(logger_, "Blosc unable to set compressor: " << BLOSC_LZ4_COMPNAME);
    }
    blosc_set_nthreads(this->compression_settings_.threads);
    LOG4CXX_TRACE(logger_, "Blosc Version: " << blosc_get_version_string());
    LOG4CXX_TRACE(logger_, "Blosc list available compressors: " << blosc_list_compressors());
    LOG4CXX_TRACE(logger_, "Blosc current compressor: " << blosc_get_compressor());
}

/**
 * Destructor.
 */
BloscPlugin::~BloscPlugin()
{
    LOG4CXX_DEBUG_LEVEL(3, logger_, "BloscPlugin destructor.");
}

/**
 * Compress one frame, return compressed frame.
 * @param src_frame - source frame to compress
 * @return pair<shared_ptr<Frame>, bool> - shared_ptr to compressed frame and boolean indicating success/fail
 */
std::pair<boost::shared_ptr<Frame>, bool> BloscPlugin::compress_frame(const boost::shared_ptr<Frame>& src_frame)
{
    bool comp_res = false;

    const void* src_data_ptr = static_cast<const void*>(static_cast<const char*>(src_frame->get_image_ptr()));
    size_t type_size = get_size_from_enum(src_frame->get_meta_data().get_data_type());
    int uncompressed_size = src_frame->get_image_size();
    size_t dest_data_size = uncompressed_size + BLOSC_MAX_OVERHEAD;
    boost::shared_ptr<Frame> dest_frame;
    try {
        dest_frame = boost::make_shared<DataBlockFrame>(src_frame->get_meta_data(), dest_data_size);
        dest_frame->meta_data().set_compression_type(blosc);
        LOG4CXX_DEBUG_LEVEL(
            2, logger_,
            "Blosc compression: frame=" << src_frame->get_frame_number() << " acquisition=\""
                                        << src_frame->get_meta_data().get_acquisition_ID() << '\"' << " compressor="
                                        << blosc_get_compressor() << " threads=" << blosc_get_nthreads()
                                        << " clevel=" << compression_settings_.compression_level
                                        << " doshuffle=" << compression_settings_.shuffle << " typesize=" << type_size
                                        << " comp_bytes=" << uncompressed_size << " destsize=" << dest_data_size
                                        << " src=" << src_data_ptr << " dest=" << dest_frame->get_image_ptr()
        );
        int compressed_size = blosc_compress_ctx(
            compression_settings_.compression_level, shuffle_str2i[compression_settings_.shuffle], type_size,
            uncompressed_size, src_data_ptr, dest_frame->get_image_ptr(), dest_data_size, blosc_get_compressor(), 0,
            compression_settings_.threads
        );

        if (compressed_size > 0) {
            comp_res = true;
            dest_frame->set_image_size(compressed_size);
            dest_frame->set_outer_chunk_size(src_frame->get_outer_chunk_size());
            LOG4CXX_DEBUG_LEVEL(
                2, logger_,
                "Blosc compression complete: frame=" << src_frame->get_frame_number()
                                                     << " compressed_size=" << compressed_size << " factor="
                                                     << (double)src_frame->get_image_size() / compressed_size
            );
        } else if (compressed_size < 0) {
            LOG4CXX_ERROR(
                logger_,
                "blosc_compress failed. error="
                    << compressed_size << " compressor=" << blosc_get_compressor()
                    << " threads=" << blosc_get_nthreads() << " clevel=" << compression_settings_.compression_level
                    << " doshuffle=" << compression_settings_.shuffle << " typesize=" << type_size
                    << " comp_bytes=" << uncompressed_size << " destsize=" << dest_data_size
            );
        } else {
            dest_frame.reset();
            LOG4CXX_ERROR(
                logger_,
                "blosc_compress failed, destination buffer not large enough! error=0 "
                    << "frame=" << src_frame->get_frame_number() << " acquisition=\""
                    << src_frame->get_meta_data().get_acquisition_ID() << "\""
            );
        }
    } catch (std::bad_alloc) {
        LOG4CXX_ERROR(logger_, "Failed to allocate memory for compressed frame");
    }
    return { std::move(dest_frame), comp_res };
}

/**
 * Decompress one frame, return decompressed frame.
 * @param src_frame - source frame to decompress
 * @return pair<shared_ptr<Frame>, bool> - shared_ptr to decompressed frame and boolean indicating success/fail
 */
std::pair<boost::shared_ptr<Frame>, bool> BloscPlugin::decompress_frame(const boost::shared_ptr<Frame>& src_frame)
{
    size_t dest_size = 0;
    size_t compressed_size;
    size_t block_size;
    // get destination buffer size - dest_size
    blosc_cbuffer_sizes(src_frame->get_data_ptr(), &dest_size, &compressed_size, &block_size);
    bool decomp_res = false;
    boost::shared_ptr<Frame> dest_frame;
    try {
        dest_frame = boost::make_shared<DataBlockFrame>(src_frame->get_meta_data(), dest_size);

        LOG4CXX_DEBUG_LEVEL(
            2, logger_,
            "Blosc decompression: frame=" << src_frame->get_frame_number() << " threads=" << blosc_get_nthreads()
                                          << " acquisition=\"" << src_frame->get_meta_data().get_acquisition_ID()
                                          << '\"' << " compressed bytes=" << src_frame->get_data_size() << " destsize="
                                          << dest_size << " src=" << static_cast<void*>(src_frame->get_image_ptr())
                                          << " typesize=" << this->compression_settings_.type_size
                                          << " dest=" << dest_frame->get_image_ptr()
        );
        int decompressed_size = blosc_decompress_ctx(
            src_frame->get_image_ptr(), dest_frame->get_image_ptr(), dest_size, this->compression_settings_.threads
        );
        if (decompressed_size > 0) {
            decomp_res = true;
            dest_frame->set_image_size(decompressed_size);
            dest_frame->set_outer_chunk_size(src_frame->get_outer_chunk_size());
            LOG4CXX_DEBUG_LEVEL(
                2, logger_,
                "Blosc decompression complete: frame=" << src_frame->get_frame_number()
                                                       << " decompressed_size=" << decompressed_size << " factor="
                                                       << (double)src_frame->get_image_size() / decompressed_size
            );
        } else {
            LOG4CXX_ERROR(
                logger_,
                "blosc_decompress failed. error="
                    << decompressed_size << ' ' << src_frame->get_frame_number() << " threads=" << blosc_get_nthreads()
                    << " acquisition=\"" << src_frame->get_meta_data().get_acquisition_ID() << '\"'
                    << " compressed bytes=" << src_frame->get_data_size() << " destsize=" << dest_size
            );
        }
    } catch (std::bad_alloc) {
        LOG4CXX_ERROR(logger_, "Failed to allocate memory for decompressed frame");
    }
    return { std::move(dest_frame), decomp_res };
}

/**
 * Update the compression settings
 * The this->mutex_ MUST be held when this function is called!
 */
void BloscPlugin::update_compression_settings()
{
    this->compression_settings_ = this->commanded_compression_settings_;
    const char* p_compressor_name;
    blosc_compcode_to_compname(compressor_str2i[this->compression_settings_.blosc_compressor], &p_compressor_name);
    LOG4CXX_DEBUG_LEVEL(
        1, logger_,
        "Blosc compression settings: " << " acquisition=\"" << this->current_acquisition_ << "\""
                                       << " compressor=" << p_compressor_name << " threads=" << blosc_get_nthreads()
                                       << " clevel=" << this->compression_settings_.compression_level
                                       << " doshuffle=" << this->compression_settings_.shuffle
                                       << " typesize=" << this->compression_settings_.type_size
                                       << " nbytes=" << this->compression_settings_.uncompressed_size
    );
    blosc_set_compressor(p_compressor_name);
    blosc_set_nthreads(this->compression_settings_.threads);
}

/**
 * Perform compression on the frame and output a new, compressed Frame.
 *
 * \param[in] frame - Pointer to a Frame object.
 */
void BloscPlugin::process_frame(boost::shared_ptr<Frame> src_frame)
{
    // Protect this method
    boost::lock_guard<boost::recursive_mutex> lock(mutex_);

    LOG4CXX_DEBUG_LEVEL(3, logger_, "Received a new frame...");

    const std::string& src_frame_acquisition_ID = src_frame->get_meta_data().get_acquisition_ID();

    if (src_frame_acquisition_ID != this->current_acquisition_) {
        LOG4CXX_DEBUG_LEVEL(1, logger_, "New acquisition detected: " << src_frame_acquisition_ID);
        this->current_acquisition_ = src_frame_acquisition_ID;
        this->update_compression_settings();
    }
    std::pair<boost::shared_ptr<Frame>, bool> output_frame;
    output_frame.second = false;
    switch (this->plugin_mode_) {
    case Mode::COMPRESS:
        output_frame = this->compress_frame(src_frame);
        break;
    case Mode::DECOMPRESS:
        output_frame = this->decompress_frame(src_frame);
        break;
    case Mode::OFF:
        output_frame = { std::move(src_frame), true };
        break;
    };

    // if succeeded, output frame!
    if (output_frame.second) {
        LOG4CXX_DEBUG_LEVEL(3, logger_, "Pushing frame");
        this->push(output_frame.first);
    }
}

/** Configure
 * @param config
 * @param reply
 */
void BloscPlugin::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
{
    LOG4CXX_INFO(logger_, config.encode());
    if (config.has_param(BloscPlugin::CONFIG_BLOSC_MODE)) {
        std::string&& mode_str = config.get_param<std::string>(BloscPlugin::CONFIG_BLOSC_MODE);
        if (!Mode_map::mode_map.count(mode_str)) {
            this->plugin_mode_ = Mode::OFF;
            reply.set_param<std::string>("warning: mode", "Set Off");
        } else {
            this->plugin_mode_ = Mode_map::mode_map.at(mode_str);
        }
    }

    if (config.has_param(BloscPlugin::CONFIG_BLOSC_LEVEL)) {
        // NOTE: we don't catch exceptions here because the get_param basically only throws IpcMessagException
        //       if the parameter isn't found - and we've just checked for that in the previous line...
        int blosc_level = config.get_param<int>(BloscPlugin::CONFIG_BLOSC_LEVEL);
        // Range checking: check and cap at upper and lower bounds
        if (blosc_level < 1) {
            LOG4CXX_WARN(logger_, "Commanded blosc level: " << blosc_level << "Capped at lower range: 1");
            blosc_level = 1;
            reply.set_param<std::string>("warning: level", "Capped at lower range: 1");
        } else if (blosc_level > 9) {
            LOG4CXX_WARN(logger_, "Commanded blosc level: " << blosc_level << "Capped at upper range: 9");
            blosc_level = 9;
            reply.set_param<std::string>("warning: level", "Capped at upper range: 9");
        }
        this->commanded_compression_settings_.compression_level = blosc_level;
    }

    if (config.has_param(BloscPlugin::CONFIG_BLOSC_SHUFFLE)) {
        std::string&& blosc_shuffle = config.get_param<std::string>(BloscPlugin::CONFIG_BLOSC_SHUFFLE);
        // Validate shuffle string against lookup table. Invalid strings results in default setting "noshuffle" string
        if (!shuffle_str2i.count(blosc_shuffle)) {
            LOG4CXX_WARN(
                logger_, "Commanded blosc shuffle: " << blosc_shuffle << " is invalid. Disabling SHUFFLE filter"
            );
            blosc_shuffle = BLOSC_NOSHUFFLE_STR;
            reply.set_param<std::string>("warning: shuffle filter", "Disabled");
        }
        this->commanded_compression_settings_.shuffle = std::move(blosc_shuffle);
    }

    if (config.has_param(BloscPlugin::CONFIG_BLOSC_THREADS)) {
        unsigned int blosc_threads = config.get_param<unsigned int>(BloscPlugin::CONFIG_BLOSC_THREADS);
        if (blosc_threads > BLOSC_MAX_THREADS) {
            LOG4CXX_WARN(logger_, "Commanded blosc threads: " << blosc_threads << " is too large. Setting 8 threads.");
            blosc_threads = 8;
            reply.set_param<int>("warning: threads", blosc_threads);
        }
        this->commanded_compression_settings_.threads = blosc_threads;
    }

    if (config.has_param(BloscPlugin::CONFIG_BLOSC_COMPRESSOR)) {
        std::string&& blosc_compressor = config.get_param<std::string>(BloscPlugin::CONFIG_BLOSC_COMPRESSOR);
        // Validate compressor name string against lookup table. Invalid strings result in default setting compressor
        // "lz4" as default
        if (!compressor_str2i.count(blosc_compressor)) {
            LOG4CXX_WARN(
                logger_,
                "Commanded blosc compressor: " << blosc_compressor << " is invalid. Setting compressor: " << BLOSC_LZ4
                                               << "(" << BLOSC_LZ4_COMPNAME << ")"
            );
            blosc_compressor = BLOSC_LZ4_COMPNAME;
            reply.set_param<int>("warning: compressor", BLOSC_LZ4);
        }
        this->commanded_compression_settings_.blosc_compressor = std::move(blosc_compressor);
    }
    // Protect the update_compression_settings() method
    boost::lock_guard<boost::recursive_mutex> lock(mutex_);
    if (this->current_acquisition_ == "") {
        this->update_compression_settings();
    }
}

/** Get configuration settings for the BloscPlugin
 *
 * @param reply - Response IpcMessage.
 */
void BloscPlugin::requestConfiguration(OdinData::IpcMessage& reply)
{
    reply.set_param(
        this->get_name() + '/' + BloscPlugin::CONFIG_BLOSC_COMPRESSOR,
        this->commanded_compression_settings_.blosc_compressor
    );
    reply.set_param(
        this->get_name() + '/' + BloscPlugin::CONFIG_BLOSC_THREADS, this->commanded_compression_settings_.threads
    );
    reply.set_param(
        this->get_name() + '/' + BloscPlugin::CONFIG_BLOSC_SHUFFLE, this->commanded_compression_settings_.shuffle
    );
    reply.set_param(
        this->get_name() + '/' + BloscPlugin::CONFIG_BLOSC_LEVEL,
        this->commanded_compression_settings_.compression_level
    );
    reply.set_param(this->get_name() + '/' + BloscPlugin::CONFIG_BLOSC_MODE, Mode_map::mode_to_str(this->plugin_mode_));
}

int BloscPlugin::get_version_major()
{
    return ODIN_DATA_VERSION_MAJOR;
}

int BloscPlugin::get_version_minor()
{
    return ODIN_DATA_VERSION_MINOR;
}

int BloscPlugin::get_version_patch()
{
    return ODIN_DATA_VERSION_PATCH;
}

std::string BloscPlugin::get_version_short()
{
    return ODIN_DATA_VERSION_STR_SHORT;
}

std::string BloscPlugin::get_version_long()
{
    return ODIN_DATA_VERSION_STR;
}

} /* namespace FrameProcessor */
