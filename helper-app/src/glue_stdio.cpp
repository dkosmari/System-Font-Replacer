#include <string>

#include <sys/iosupport.h>

#include <whb/log.h>

#include "whb.hpp"


ssize_t
write_to_log(_reent*, void*, const char* ptr, size_t len)
{
    try {
        // only way to guarantee it's null-terminated
        std::string buf{ptr, len};
        if (!WHBLogWrite(buf.c_str()))
            return -1;
        whb::console::draw();
        return buf.size();
    }
    catch (...) {
        return -1;
    }
}


__attribute__((__constructor__))
void
init_stdio()
{
    static devoptab_t dev_out;
    dev_out.name = "stdout";
    dev_out.write_r = write_to_log;
    devoptab_list[STD_OUT] = &dev_out;
}
