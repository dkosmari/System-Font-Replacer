/*
 * System Font Replacer - A plugin to temporarily replace the Wii U's system font.
 *
 * Copyright (C) 2025  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <iostream>

#include "mocha.hpp"


namespace mocha {

    error::error(MochaUtilsStatus status) :
        std::runtime_error{Mocha_GetStatusStr(status)}
    {}



    lib_init::lib_init()
    {
        auto status = Mocha_InitLibrary();
        if (status != MOCHA_RESULT_SUCCESS)
            throw error{status};
    }


    lib_init::~lib_init()
    {
        Mocha_DeInitLibrary();
    }



    mount::mount(const std::string& name,
                 const std::optional<std::filesystem::path>& dev_path,
                 const std::filesystem::path& mnt_path) :
        name{name}
    {
        auto status = Mocha_MountFS(name.c_str(),
                                    dev_path ? dev_path->c_str() : nullptr,
                                    mnt_path.c_str());
        if (status != MOCHA_RESULT_SUCCESS)
            throw error{status};

        std::cout << "Mounted " << name << std::endl;
    }


    mount::~mount()
    {
        Mocha_UnmountFS(name.c_str());
        std::cout << "Unmounted " << name << std::endl;
    }


} // namespace mocha
