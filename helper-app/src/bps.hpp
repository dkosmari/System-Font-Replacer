/*
 * System Font Replacer - A plugin to temporarily replace the Wii U's system font.
 *
 * Copyright (C) 2024  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef BPS_HPP
#define BPS_HPP

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>


namespace bps {

    struct error : std::runtime_error {

        error(const char* msg);
        error(const std::string& msg);

    };


    struct info {
        std::uintmax_t size_in;
        std::uintmax_t size_out;
        std::uintmax_t meta_start;
        std::uintmax_t data_start;

        std::uint32_t crc_in;
        std::uint32_t crc_out;
        std::uint32_t crc_patch;
    };


    info get_info(std::span<const std::byte> patch);


    std::vector<std::byte>
    apply(std::span<const std::byte> patch,
          std::span<const std::byte> input);


} // namespace bps

#endif
