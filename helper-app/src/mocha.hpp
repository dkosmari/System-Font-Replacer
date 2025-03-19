/*
 * System Font Replacer - A plugin to temporarily replace the Wii U's system font.
 *
 * Copyright (C) 2025  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef MOCHA_HPP
#define MOCHA_HPP

#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>

#include <mocha/mocha.h>


namespace mocha {

    struct error : std::runtime_error {
        error(MochaUtilsStatus status);
    };


    struct lib_init {

        lib_init();

        ~lib_init();

    };


    struct mount {

        const std::string name;

        mount(const std::string& name,
              const std::optional<std::filesystem::path>& dev_path,
              const std::filesystem::path& mnt_path);

        ~mount();

    };

} // namespace mocha

#endif
