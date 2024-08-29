/*
 * System Font Replacer - A plugin to temporarily replace the Wii U's system font.
 *
 * Copyright (C) 2024  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cstdio>
#include <cstring>              // strcmp
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <utility>              // move()
#include <vector>

#include <coreinit/debug.h>
#include <coreinit/memory.h>
#include <coreinit/thread.h>
#include <coreinit/title.h>
#include <sysapp/switch.h>
#include <whb/log.h>
#include <whb/log_module.h>
#include <whb/log_udp.h>

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


struct log_manager {

    bool module_init = false;
    bool udp_init = false;


    log_manager()
    {
        // Note: only use UDP if Logging Module failed to init.
        if (!(module_init = WHBLogModuleInit()))
            udp_init = WHBLogUdpInit();
    }


    ~log_manager()
    {
        if (module_init)
            WHBLogModuleDeinit();
        if (udp_init)
            WHBLogUdpDeinit();
    }

};


// This is alive between application start and end hooks.
std::optional<log_manager> app_log_mgr;


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
        const char* enabled   = "Enabled";
        const char* only_menu = "Use custom fonts only for Wii U Menu";
        const char* path_cn   = "Cn Font";
        const char* path_kr   = "Kr Font";
        const char* path_std  = "Std Font";
        const char* path_tw   = "Tw Font";
    }


    namespace defaults {
        bool enabled   = true;
        bool only_menu = true;
        path path_cn   = "fs:/vol/external01/wiiu/fonts";
        path path_kr   = "fs:/vol/external01/wiiu/fonts";
        path path_std  = "fs:/vol/external01/wiiu/fonts";
        path path_tw   = "fs:/vol/external01/wiiu/fonts";
    }


    bool enabled   = defaults::enabled;
    bool only_menu = defaults::only_menu;
    path path_cn   = defaults::path_cn;
    path path_kr   = defaults::path_kr;
    path path_std  = defaults::path_std;
    path path_tw   = defaults::path_tw;


    void
    load()
    {
        try {
#define LOAD(x) wups::storage::load_or_init(#x, x, defaults::x)
            LOAD(enabled);
            LOAD(only_menu);
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
            STORE(only_menu);
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

        root.add(wups::config::text_item::create("NOTE: Changes might NOT take effect until the next boot."));

        root.add(wups::config::bool_item::create(cfg::labels::enabled,
                                                 cfg::enabled,
                                                 cfg::defaults::enabled,
                                                 "yes", "no"));

        root.add(wups::config::file_item::create(cfg::labels::path_std,
                                                 cfg::path_std,
                                                 cfg::defaults::path_std,
                                                 40,
                                                 {".ttf"}));

        root.add(wups::config::file_item::create(cfg::labels::path_cn,
                                                 cfg::path_cn,
                                                 cfg::defaults::path_cn,
                                                 40,
                                                 {".ttf"}));

        root.add(wups::config::file_item::create(cfg::labels::path_kr,
                                                 cfg::path_kr,
                                                 cfg::defaults::path_kr,
                                                 40,
                                                 {".ttf"}));

        root.add(wups::config::file_item::create(cfg::labels::path_tw,
                                                 cfg::path_tw,
                                                 cfg::defaults::path_tw,
                                                 40,
                                                 {".ttf"}));

        root.add(wups::config::bool_item::create(cfg::labels::only_menu,
                                                 cfg::only_menu,
                                                 cfg::defaults::only_menu,
                                                 "yes", "no"));

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
try_load_font(const path& font_path)
{
    FILE* f = nullptr;
    try {
        // silently exits if file doesn't exist, or is not a file
        if (!exists(font_path) || !is_regular_file(font_path))
            return {};

        auto size = file_size(font_path);
        // too small file is probably a mistake; corrupted FS or broken FTP transfer
        if (size < 8)
            throw std::runtime_error{"font file size is too small!"};

        f = std::fopen(font_path.c_str(), "rb");
        if (!f)
            throw std::runtime_error{"cannot open \"" + font_path.string() + "\""};

        const char ttf_magic[4] = {0x00, 0x01, 0x00, 0x00};
        char file_magic[4];
        auto res = std::fread(file_magic, 1, 4, f);
        if (res != 4)
            throw std::runtime_error{"cannot read TTF magic!"};
        if (std::memcmp(ttf_magic, file_magic, 4))
            throw std::runtime_error{"no TTF magic in font file!"};

        std::rewind(f);

        blob_t content(size);
        res = std::fread(content.data(), 1, size, f);
        if (static_cast<std::uintmax_t>(res) != size)
            throw std::runtime_error{"could not load entire font file"};

        std::fclose(f);
        f = nullptr;

        return { std::move(content) };
    }
    catch (std::exception& e) {
        if (f)
            std::fclose(f);
        LOG("failed to load font file \"%s\": %s\n",
            font_path.c_str(), e.what());
        return {};
    }
}


INITIALIZE_PLUGIN()
{
    log_manager log_guard;

    WUPSConfigAPIOptionsV1 options{ .name = PACKAGE_NAME };
    auto status = WUPSConfigAPI_Init(options, menu_open, menu_close);
    if (status != WUPSCONFIG_API_RESULT_SUCCESS) {
        LOG("WUPSConfigAPI_Init() failed: %s\n", WUPSConfigAPI_GetStatusStr(status));
        return;
    }

    cfg::load();

    if (!cfg::enabled)
        return;

    if (auto font = try_load_font(cfg::path_cn))
        font_cn = std::move(*font);

    if (auto font = try_load_font(cfg::path_kr))
        font_kr = std::move(*font);

    if (auto font = try_load_font(cfg::path_std))
        font_std = std::move(*font);

    if (auto font = try_load_font(cfg::path_tw))
        font_tw = std::move(*font);

}


ON_APPLICATION_START()
{
    app_log_mgr.emplace();
}


ON_APPLICATION_ENDS()
{
    app_log_mgr.reset();
}


DECL_FUNCTION(BOOL,
              OSGetSharedData,
              OSSharedDataType type,
              uint32_t unused,
              void** buf,
              uint32_t* size)
{
    if (unused == 0xefface) {
        /*
         * efface
         *
         * transitive verb: To cause to disappear (as anything impresses or inscribed upon
         * a surface) by rubbing out, striking out, etc.; to erase; to render illegible or
         * indiscernible.
         */
        unused = 0;
        goto real_function;
    }

    if (!cfg::enabled)
        goto real_function;

    if (cfg::only_menu) {

#if 0
        /*
          This fragment will be enabled if/when:
          - https://github.com/wiiu-env/WiiUPluginLoaderBackend/pull/86
          - https://github.com/wiiu-env/WiiUPluginSystem/pull/76
         */

        // Avoid when inside WUPS config menu.
        BOOL isMenuOpen = false;
        WUPSConfigAPI_GetMenuOpen(&isMenuOpen);
        if (isMenuOpen)
            goto real_function;
#endif

        // Avoid when not inside the Wii U Menu.
        const std::uint64_t wii_u_menu_id = 0x0005001010040000;
        const std::uint64_t region_mask   = 0xfffffffffffffcff;
        const std::uint64_t title = OSGetTitleID();
        if ((title & region_mask) != wii_u_menu_id)
            goto real_function;

        // Avoid when using the on-screen keyboard inside the Wii U Menu.
        OSThread* th_id = OSGetCurrentThread();
        const char* th_name = OSGetThreadName(th_id);
        if (th_name && !strcmp("MenSwkbdCalculator_Create", th_name))
            goto real_function;
    }

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
    return real_OSGetSharedData(type, unused, buf, size);
}


WUPS_MUST_REPLACE_FOR_PROCESS(OSGetSharedData,
                              WUPS_LOADER_LIBRARY_COREINIT,
                              OSGetSharedData,
                              WUPS_FP_TARGET_PROCESS_ALL);
