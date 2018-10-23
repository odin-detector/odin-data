//
// Created by gnx91527 on 22/10/18.
//

#ifndef ODINDATA_IVERSIONEDOBJECT_H
#define ODINDATA_IVERSIONEDOBJECT_H

#include <string>

namespace FrameProcessor
{
/** Interface to provide version information for any odin-data object.
 *
 * All FrameReceiver and FrameProcessor plugins will subclass this interface
 * which enforces the retrieval of version information regarding the plugin
 * to be implemented.  This information will be available through the odin-control
 * interface.
 */
    class IVersionedObject
    {
    public:
        IVersionedObject(){};
        virtual ~IVersionedObject(){};
        virtual int get_version_major() = 0;
        virtual int get_version_minor() = 0;
        virtual int get_version_patch() = 0;
        virtual std::string get_version_short() = 0;
        virtual std::string get_version_long() = 0;
    };

} /* namespace FrameProcessor */

#endif //ODINDATA_IVERSIONEDOBJECT_H
