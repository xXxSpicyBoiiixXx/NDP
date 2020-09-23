
#include <edge/interop.h>
#include "edge/fault_msg.h"

namespace edge {

fault_msg::fault_msg(const uffd_msg &raw)
        : raw_(raw)
        , page_size_(edge::interop::page_size())
{}

}