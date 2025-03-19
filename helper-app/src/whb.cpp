/*
 * System Font Replacer - A plugin to temporarily replace the Wii U's system font.
 *
 * Copyright (C) 2025  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdexcept>

#include <sysapp/launch.h>
#include <whb/log_console.h>
#include <whb/log_module.h>
#include <whb/proc.h>

#include "whb.hpp"


namespace whb {

    log_module::log_module()
        noexcept
    {
        WHBLogModuleInit();
    }


    log_module::~log_module()
        noexcept
    {
        WHBLogModuleDeinit();
    }



    console::console()
        noexcept
    {
        WHBLogConsoleInit();
    }


    console::~console()
        noexcept
    {
        WHBLogConsoleFree();
    }


    void
    console::set_color(uint8_t r,
                       uint8_t g,
                       uint8_t b)
        noexcept
    {
        uint32_t color = uint32_t{r} << 24 |
                         uint32_t{g} << 16 |
                         uint32_t{b} << 8;
        WHBLogConsoleSetColor(color);
    }


    void
    console::draw()
        noexcept
    {
        WHBLogConsoleDraw();
    }



    proc::proc()
        noexcept
    {
        WHBProcInit();
    }


    proc::~proc()
        noexcept
    {
        WHBProcShutdown();
    }


    void
    proc::stop()
        noexcept
    {
        SYSLaunchMenu();
    }


    bool
    proc::is_running()
        noexcept
    {
        return WHBProcIsRunning();
    }



} // namespace whb
