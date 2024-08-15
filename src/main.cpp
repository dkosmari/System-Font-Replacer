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

#include <wupsxx/bool_item.hpp>
#include <wupsxx/category.hpp>
#include <wupsxx/file_item.hpp>
#include <wupsxx/storage.hpp>
#include <wupsxx/text_item.hpp>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


using blob_t = std::vector<char>;
using std::filesystem::path;


WUPS_PLUGIN_NAME(PACKAGE_NAME);
WUPS_PLUGIN_DESCRIPTION("Redirect the system font to a custom font on the SD card.");
WUPS_PLUGIN_VERSION(PACKAGE_VERSION);
WUPS_PLUGIN_AUTHOR("Daniel K. O.");
WUPS_PLUGIN_LICENSE("GPLv3");

WUPS_USE_WUT_DEVOPTAB();
WUPS_USE_STORAGE(PACKAGE_TARNAME);


#define LOG(msg, ...)                                           \
    WHBLogPrintf("[" PACKAGE_TARNAME "] %s:%d/%s(): " msg,      \
                 __FILE__,                                      \
                 __LINE__,                                      \
                 __func__,                                      \
                 __VA_ARGS__)


blob_t font_cn;
blob_t font_kr;
blob_t font_std;
blob_t font_tw;


namespace cfg {

    namespace labels {
        const char* enabled  = "Enabled";
        const char* path_cn  = "Simpl. Chinese";
        const char* path_kr  = "Korean";
        const char* path_std = "Standard";
        const char* path_tw  = "Trad. Chinese";
    }


    namespace defaults {
        bool enabled  = true;
        path path_cn  = "fs:/vol/external01/wiiu/fonts";
        path path_kr  = "fs:/vol/external01/wiiu/fonts";
        path path_std = "fs:/vol/external01/wiiu/fonts";
        path path_tw  = "fs:/vol/external01/wiiu/fonts";
    }


    bool enabled  = defaults::enabled;
    path path_cn  = defaults::path_cn;
    path path_kr  = defaults::path_kr;
    path path_std = defaults::path_std;
    path path_tw  = defaults::path_tw;


    void
    load()
    {
        try {
#define LOAD(x) wups::storage::load_or_init(#x, x, defaults::x)
            LOAD(enabled);
            LOAD(path_cn);
            LOAD(path_kr);
            LOAD(path_std);
            LOAD(path_tw);
#undef LOAD
        }
        catch (std::exception& e) {
            LOG("exception caugt: %s\n", e.what());
        }
    }


    void
    save()
    {
        try {
#define STORE(x) wups::storage::store(#x, x)
            STORE(enabled);
            STORE(path_cn);
            STORE(path_kr);
            STORE(path_std);
            STORE(path_tw);
#undef STORE
            wups::storage::save();
        }
        catch (std::exception& e) {
            LOG("exception caught: %s\n", e.what());
        }
    }


} // namespace cfg


WUPSConfigAPICallbackStatus
menu_open(WUPSConfigCategoryHandle root_handle)
{
    try {

        wups::config::category root{root_handle};

        root.add(wups::config::text_item::create("NOTE: reboot for changes to take effect correctly."));

        root.add(wups::config::bool_item::create(cfg::labels::enabled,
                                                 cfg::enabled,
                                                 cfg::defaults::enabled,
                                                 "yes", "no"));

        root.add(wups::config::file_item::create(cfg::labels::path_std,
                                                 cfg::path_std,
                                                 cfg::defaults::path_std,
                                                 40));

        root.add(wups::config::file_item::create(cfg::labels::path_cn,
                                                 cfg::path_cn,
                                                 cfg::defaults::path_cn,
                                                 40));

        root.add(wups::config::file_item::create(cfg::labels::path_kr,
                                                 cfg::path_kr,
                                                 cfg::defaults::path_kr,
                                                 40));

        root.add(wups::config::file_item::create(cfg::labels::path_tw,
                                                 cfg::path_tw,
                                                 cfg::defaults::path_tw,
                                                 40));

        root.add(wups::config::text_item::create("Website",
                                                 PACKAGE_URL));

        return WUPSCONFIG_API_CALLBACK_RESULT_SUCCESS;
    }
    catch (std::exception& e) {
        LOG("exception caught: %s\n", e.what());
        return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
    }
}


void
menu_close()
{
    try {
        cfg::save();
    }
    catch (std::exception& e) {
        LOG("exception caught: %s\n", e.what());
    }
}


std::optional<blob_t>
try_load_file(const std::filesystem::path& file_path)
{
    FILE* f = nullptr;
    try {
        // silently exits if file doesn't exist, or is not a file
        if (!exists(file_path) || !is_regular_file(file_path))
            return {};

        auto size = file_size(file_path);
        // too small file is probably a mistake; corrupted FS or broken FTP transfer
        if (size < 8)
            throw std::runtime_error{"file size is too small!"};

        f = std::fopen(file_path.c_str(), "rb");
        if (!f)
            throw std::runtime_error{"cannot open \"" + file_path.string() + "\""};

        const char ttf_magic[4] = {0x00, 0x01, 0x00, 0x00};
        char file_magic[4];
        auto res = std::fread(file_magic, 1, 4, f);
        if (res != 4)
            throw std::runtime_error{"cannot read TTF magic!"};
        if (std::memcmp(ttf_magic, file_magic, 4))
            throw std::runtime_error{"not TTF magic!"};

        std::rewind(f);

        blob_t content(size);
        res = std::fread(content.data(), 1, size, f);
        if (static_cast<std::uintmax_t>(res) != size)
            throw std::runtime_error{"could not load entire file"};

        std::fclose(f);
        f = nullptr;

        return { std::move(content) };
    }
    catch (std::exception& e) {
        if (f)
            std::fclose(f);
        LOG("failed to load file \"%s\": %s\n",
            file_path.c_str(), e.what());
        return {};
    }
}


INITIALIZE_PLUGIN()
{
    WHBLogModuleInit();

    WUPSConfigAPIOptionsV1 options{ .name = PACKAGE_NAME };
    auto status = WUPSConfigAPI_Init(options, menu_open, menu_close);
    if (status != WUPSCONFIG_API_RESULT_SUCCESS) {
        LOG("WUPSConfigAPI_Init() failed: %s\n", WUPSConfigAPI_GetStatusStr(status));
        return;
    }

    cfg::load();

    if (!cfg::enabled)
        return;

    if (auto font = try_load_file(cfg::path_cn))
        font_cn = std::move(*font);

    if (auto font = try_load_file(cfg::path_kr))
        font_kr = std::move(*font);

    if (auto font = try_load_file(cfg::path_std))
        font_std = std::move(*font);

    if (auto font = try_load_file(cfg::path_tw))
        font_tw = std::move(*font);

}


DEINITIALIZE_PLUGIN()
{
    WHBLogModuleDeinit();
}


DECL_FUNCTION(BOOL,
              OSGetSharedData,
              OSSharedDataType type,
              uint32_t unknown,
              void** buf,
              uint32_t* size)
{
    if (!cfg::enabled)
        goto real_function;

    switch (type) {

    case OS_SHAREDDATATYPE_FONT_CHINESE:
        if (font_cn.empty())
            goto real_function;
        *buf  = font_cn.data();
        *size = font_cn.size();
        return true;

    case OS_SHAREDDATATYPE_FONT_KOREAN:
        if (font_kr.empty())
            goto real_function;
        *buf  = font_kr.data();
        *size = font_kr.size();
        return true;

    case OS_SHAREDDATATYPE_FONT_STANDARD:
        if (font_std.empty())
            goto real_function;
        *buf  = font_std.data();
        *size = font_std.size();
        return true;

    case OS_SHAREDDATATYPE_FONT_TAIWANESE:
        if (font_tw.empty())
            goto real_function;
        *buf  = font_tw.data();
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
