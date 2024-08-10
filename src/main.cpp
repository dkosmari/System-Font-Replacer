/*
 * System Font Replacer - A plugin to temporarily replace the Wii U's system font.
 *
 * Copyright (C) 2024  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cstdio>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <utility>              // move()
#include <vector>

#include <coreinit/cosreport.h>
#include <coreinit/memory.h>
#include <whb/log.h>
#include <whb/log_module.h>

#include <wups.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


using blob_t = std::vector<char>;


WUPS_PLUGIN_NAME(PACKAGE_NAME);
WUPS_PLUGIN_DESCRIPTION("Replace the system font.");
WUPS_PLUGIN_VERSION(PACKAGE_VERSION);
WUPS_PLUGIN_AUTHOR("Daniel K. O.");
WUPS_PLUGIN_LICENSE("GPLv3");


WUPS_USE_WUT_DEVOPTAB();

blob_t font_cn;
blob_t font_kr;
blob_t font_std;
blob_t font_tw;


std::optional<blob_t>
try_load_file(const std::filesystem::path& file_path)
{
    try {
        auto file_size = std::filesystem::file_size(file_path);
        if (file_size == 0)
            throw std::runtime_error{"file size is zero!"};

        blob_t data(file_size);

        std::FILE* f = std::fopen(file_path.c_str(), "rb");
        if (!f)
            throw std::runtime_error{"cannot open \"" + file_path.string() + "\""};

        auto res = std::fread(data.data(), 1, file_size, f);
        std::fclose(f);

        if (static_cast<std::uintmax_t>(res) != file_size) {
            WHBLogPrintf("Could not load all of \"%s\": %u/%u\n",
                         file_path.c_str(),
                         static_cast<unsigned>(res),
                         static_cast<unsigned>(file_size));
            return {};
        }

        return data;
    }
    catch (std::exception& e) {
        WHBLogPrintf("Failed to load file: %s\n", e.what());
        return {};
    }
}


INITIALIZE_PLUGIN()
{
    WHBLogModuleInit();

    std::filesystem::path font_path = "fs:/vol/external01/wiiu/fonts";

    if (auto font = try_load_file(font_path / "CafeCn.ttf"))
        font_cn = std::move(*font);

    if (auto font = try_load_file(font_path / "CafeKr.ttf"))
        font_kr = std::move(*font);

    if (auto font = try_load_file(font_path / "CafeStd.ttf"))
        font_std = std::move(*font);

    if (auto font = try_load_file(font_path / "CafeTw.ttf"))
        font_tw = std::move(*font);

    WHBLogModuleDeinit();
}


DEINITIALIZE_PLUGIN()
{
    font_cn.clear();
    font_kr.clear();
    font_std.clear();
    font_tw.clear();

}


DECL_FUNCTION(BOOL,
              OSGetSharedData,
              OSSharedDataType type,
              uint32_t unknown,
              void** buf,
              uint32_t* size)
{
    switch (type) {

    case OS_SHAREDDATATYPE_FONT_CHINESE:
        if (font_cn.empty())
            goto real_function;
        *buf = font_cn.data();
        *size = font_cn.size();
        return true;

    case OS_SHAREDDATATYPE_FONT_KOREAN:
        if (font_kr.empty())
            goto real_function;
        *buf = font_kr.data();
        *size = font_kr.size();
        return true;

    case OS_SHAREDDATATYPE_FONT_STANDARD:
        if (font_std.empty())
            goto real_function;
        *buf = font_std.data();
        *size = font_std.size();
        return true;

    case OS_SHAREDDATATYPE_FONT_TAIWANESE:
        if (font_tw.empty())
            goto real_function;
        *buf = font_tw.data();
        *size = font_tw.size();
        return true;

    default:
        ;

    } // switch (type)

 real_function:
    return real_OSGetSharedData(type, unknown, buf, size);
}


WUPS_MUST_REPLACE_FOR_PROCESS(OSGetSharedData,
                              WUPS_LOADER_LIBRARY_COREINIT,
                              OSGetSharedData,
                              WUPS_FP_TARGET_PROCESS_ALL);
