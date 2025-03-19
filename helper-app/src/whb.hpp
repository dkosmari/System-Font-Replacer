/*
 * System Font Replacer - A plugin to temporarily replace the Wii U's system font.
 *
 * Copyright (C) 2025  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WHB_HPP
#define WHB_HPP

#include <cstdint>


namespace whb {

    struct log_module {

        log_module()
            noexcept;

        ~log_module()
            noexcept;

    };


    struct console {

        console()
            noexcept;

        ~console()
            noexcept;

        void
        set_color(std::uint8_t r,
                  std::uint8_t g,
                  std::uint8_t b)
            noexcept;

        static
        void
        draw()
            noexcept;

    };


    struct proc {

        proc()
            noexcept;

        ~proc()
            noexcept;

        static
        void
        stop()
            noexcept;

        static
        bool
        is_running()
            noexcept;


        struct quit {};

    };

} // namespace whb


#endif
