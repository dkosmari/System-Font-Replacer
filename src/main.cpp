/*
 * System Font Replacer - A plugin to temporarily replace the Wii U's system font.
 *
 * Copyright (C) 2025-2026  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>              // strcmp
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>              // move()
#include <vector>

#include <coreinit/debug.h>
#include <coreinit/memory.h>
#include <coreinit/thread.h>
#include <coreinit/title.h>
#include <sysapp/switch.h>

#include <wups.h>

#include <wupsxx/bool_item.hpp>
#include <wupsxx/category.hpp>
#include <wupsxx/file_item.hpp>
#include <wupsxx/init.hpp>
#include <wupsxx/storage.hpp>
#include <wupsxx/text_item.hpp>
#include <wupsxx/logger.hpp>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


WUPS_PLUGIN_NAME(PACKAGE_NAME);
WUPS_PLUGIN_DESCRIPTION("Redirect the system font to a custom font on the SD card.");
WUPS_PLUGIN_VERSION(PACKAGE_VERSION);
WUPS_PLUGIN_AUTHOR("Daniel K. O.");
WUPS_PLUGIN_LICENSE("GPLv3");

WUPS_USE_WUT_DEVOPTAB();
WUPS_USE_STORAGE(PACKAGE_TARNAME);


using std::filesystem::path;

using namespace std::literals;

namespace logger = wups::logger;


namespace cfg {

    WUPSXX_OPTION("Enabled",
                  bool, enabled, true);

    WUPSXX_OPTION("  └ Only on Wii U Menu",
                  bool, only_menu, true);

    WUPSXX_OPTION("Std Font",
                  path, path_std, "");

    WUPSXX_OPTION("Cn Font",
                  path, path_cn, "");

    WUPSXX_OPTION("Kr Font",
                  path, path_kr, "");

    WUPSXX_OPTION("Tw Font",
                  path, path_tw, "");


    std::vector<wups::option_base*> all_options{
        &enabled,
        &only_menu,
        &path_std,
        &path_cn,
        &path_kr,
        &path_tw,
    };


    void
    load()
    {
        for (auto& opt : all_options)
            opt->load();
    }


    void
    save()
    {
        try {
            for (const auto& opt : all_options)
                opt->store();
            wups::save();
        }
        catch (std::exception& e) {
            logger::printf("Error saving settings: %s\n", e.what());
        }
    }

} // namespace cfg


void
menu_open(wups::category& root)
{
    logger::guard guard;

    using wups::make_item;
    root.add(make_item("NOTE"s, "Restart the app/game for the font to change."s, 60));

    wups::bool_item::specs bool_opts;
    bool_opts.true_label = "yes";
    bool_opts.false_label = "no";
    root.add(make_item(cfg::enabled, bool_opts));
    root.add(make_item(cfg::only_menu, bool_opts));

    using wups::file_item;
    file_item::specs file_opts;
    file_opts.extensions = {".ttf"};
    file_opts.valid = file_item::type::regular;
    file_opts.max_width = 40;
    std::filesystem::path fonts_path = "fs:/vol/external01/wiiu/fonts";
    root.add(make_item(cfg::path_std, fonts_path, file_opts));
    root.add(make_item(cfg::path_cn,  fonts_path, file_opts));
    root.add(make_item(cfg::path_kr,  fonts_path, file_opts));
    root.add(make_item(cfg::path_tw,  fonts_path, file_opts));

    root.add(make_item("Website"s, std::string(PACKAGE_URL)));
}


void
menu_close()
{
    logger::guard guard;

    // libwupsxx changes: if the font selected isn't a regular file, just clear it

    if (!cfg::path_std.value.empty()
        && (!exists(cfg::path_std.value) || !is_regular_file(cfg::path_std.value)))
        cfg::path_std.value.clear();

    if (!cfg::path_cn.value.empty()
        && (!exists(cfg::path_cn.value) || !is_regular_file(cfg::path_cn.value)))
        cfg::path_cn.value.clear();

    if (!cfg::path_kr.value.empty()
        && (!exists(cfg::path_kr.value) || !is_regular_file(cfg::path_kr.value)))
        cfg::path_kr.value.clear();

    if (!cfg::path_tw.value.empty()
        && (!exists(cfg::path_tw.value) || !is_regular_file(cfg::path_tw.value)))
        cfg::path_tw.value.clear();

    cfg::save();
}


struct blob_t {
    std::unique_ptr<char[]> data_ptr;
    std::size_t data_size = 0;

    constexpr
    blob_t() noexcept = default;

    blob_t(std::size_t sz)
    {
        data_ptr = std::make_unique<char[]>(sz);
        data_size = sz;
    }

    blob_t(blob_t&& other)
        noexcept :
        data_ptr{std::move(other.data_ptr)},
        data_size{other.data_size}
    {
        other.data_size = 0;
    }

    blob_t&
    operator =(blob_t&& other)
        noexcept
    {
        if (this != &other) {
            data_ptr = std::move(other.data_ptr);
            data_size = other.data_size;
            other.data_size = 0;
        }
        return *this;
    }

    void
    clear()
        noexcept
    {
        data_ptr.reset();
        data_size = 0;
    }

    const char*
    data()
        const noexcept
    {
        return data_ptr.get();
    }

    char*
    data()
        noexcept
    {
        return data_ptr.get();
    }

    std::size_t
    size()
        const noexcept
    {
        return data_size;
    }

    bool
    empty()
        const noexcept
    {
        return !data_ptr;
    }

};


blob_t font_cn;
blob_t font_kr;
blob_t font_std;
blob_t font_tw;


void
unload_all_fonts()
{
    font_cn.clear();
    font_kr.clear();
    font_std.clear();
    font_tw.clear();
}


std::optional<blob_t>
try_load_font(const path& font_path)
{
    using std::runtime_error;

    FILE* f = nullptr;
    try {
        // silently exits if file doesn't exist, or is not a file
        if (font_path.empty() || !exists(font_path) || !is_regular_file(font_path))
            return {};

        auto size = file_size(font_path);
        // too small file is probably a mistake; corrupted FS or broken FTP transfer
        if (size < 8)
            throw runtime_error{"font file size is too small!"};

        f = std::fopen(font_path.c_str(), "rb");
        if (!f)
            throw runtime_error{"cannot open \"" + font_path.string() + "\"!"};

        const char ttf_magic[4] = {0x00, 0x01, 0x00, 0x00};
        char file_magic[4];
        auto res = std::fread(file_magic, 1, 4, f);
        if (res != 4)
            throw runtime_error{"cannot read TTF magic!"};
        if (std::memcmp(ttf_magic, file_magic, 4))
            throw runtime_error{"no TTF magic in font file!"};

        std::rewind(f);

        blob_t content(size);
        res = std::fread(content.data(), 1, size, f);
        if (static_cast<std::uintmax_t>(res) != size)
            throw runtime_error{"could not load entire font file!"};

        std::fclose(f);
        f = nullptr;

        return { std::move(content) };
    }
    catch (std::exception& e) {
        if (f)
            std::fclose(f);
        // Note: we can't use WHBLog* inside applets
        OSReport("[%s] Failed to load font file \"%s\": %s\n",
                 PACKAGE_NAME,
                 font_path.c_str(),
                 e.what());
        return {};
    }
}


bool
from_wups_menu()
    noexcept
{
    WUPSConfigAPIMenuStatus menu_status = WUPSCONFIG_API_MENU_STATUS_CLOSED;
    WUPSConfigAPI_Menu_GetStatus(&menu_status);
    return menu_status == WUPSCONFIG_API_MENU_STATUS_OPENED;
}


bool
from_wiiu_menu()
    noexcept
{
    switch (OSGetTitleID()) {
        case 0x00050010'10040000: // JPN
        case 0x00050010'10040100: // USA
        case 0x00050010'10040200: // EUR
            return true;
        default:
            return false;
    }
}


bool
from_wiiu_menu_swkbd()
    noexcept
{
    OSThread* th_id = OSGetCurrentThread();
    const char* th_name = OSGetThreadName(th_id);
    if (!th_name)
        return false;
    if (strcmp("MenSwkbdCalculator_Create", th_name))
        return false;
    return true;
}


INITIALIZE_PLUGIN()
{
    logger::set_prefix(PACKAGE_NAME);
    logger::guard guard;

    try {
        wups::init(PACKAGE_NAME, menu_open, menu_close);
        cfg::load();
    }
    catch (std::exception& e) {
        logger::printf("Init error: %s\n", e.what());
    }
}


ON_APPLICATION_ENDS()
{
    unload_all_fonts();
}


DECL_FUNCTION(BOOL,
              OSGetSharedData,
              OSSharedDataType type,
              uint32_t unused,
              void** out_data,
              uint32_t* out_size)
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

    if (!cfg::enabled.value)
        goto real_function;

    // Never replace the font in the WUPS config menu.
    if (from_wups_menu())
        goto real_function;

    if (cfg::only_menu.value) {

        // Avoid when not inside the Wii U Menu.
        if (!from_wiiu_menu())
            goto real_function;

        // Avoid when using the on-screen keyboard inside the Wii U Menu.
        if (from_wiiu_menu_swkbd())
            goto real_function;
    }

    {
        auto handle_font = [&out_data, &out_size](blob_t& font_blob,
                                                  const path& font_path)
        {
            if (font_blob.empty()) {
                if (auto font = try_load_font(font_path))
                    font_blob = std::move(*font);
                else
                    return false;
            }
            *out_data  = font_blob.data();
            *out_size = font_blob.size();
            return true;
        };

        switch (type) {

            case OS_SHAREDDATATYPE_FONT_CHINESE:
                if (!handle_font(font_cn, cfg::path_cn.value))
                    goto real_function;
                return true;

            case OS_SHAREDDATATYPE_FONT_KOREAN:
                if (!handle_font(font_kr, cfg::path_kr.value))
                    goto real_function;
                return true;

            case OS_SHAREDDATATYPE_FONT_STANDARD:
                if (!handle_font(font_std, cfg::path_std.value))
                    goto real_function;
                return true;

            case OS_SHAREDDATATYPE_FONT_TAIWANESE:
                if (!handle_font(font_tw, cfg::path_tw.value))
                    goto real_function;
                return true;

            default:
                ;

        } // switch (type)
    }

 real_function:
    return real_OSGetSharedData(type, unused, out_data, out_size);
}


WUPS_MUST_REPLACE_FOR_PROCESS(OSGetSharedData,
                              WUPS_LOADER_LIBRARY_COREINIT,
                              OSGetSharedData,
                              WUPS_FP_TARGET_PROCESS_ALL);
