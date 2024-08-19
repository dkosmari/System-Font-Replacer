/*
 * System Font Replacer - A plugin to temporarily replace the Wii U's system font.
 *
 * Copyright (C) 2024  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CRC32_HPP
#define CRC32_HPP

#include <cstdint>
#include <cstddef>
#include <span>


std::uint32_t
calc_crc32(std::span<const std::byte> data,
           std::uint32_t crc32 = 0);

#endif
