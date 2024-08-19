/*
 * System Font Replacer - A plugin to temporarily replace the Wii U's system font.
 *
 * Copyright (C) 2024  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <sys/iosupport.h>

#include <vpad/input.h>
#include <whb/log.h>
#include <whb/log_console.h>
#include <whb/log_module.h>
#include <whb/proc.h>
#include <sysapp/launch.h>

#include <mocha/mocha.h>

#include "bps.hpp"
#include "crc32.hpp"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


using namespace std::literals;
using std::filesystem::path;
using std::cout;
using std::endl;

using blob_t = std::vector<std::byte>;


struct guard_base {
    bool valid = false;

    operator bool()
        const noexcept
    {
        return valid;
    }
};


namespace mocha {

    struct error : std::runtime_error {

        error(MochaUtilsStatus status) :
            std::runtime_error{Mocha_GetStatusStr(status)}
        {}

    };


    struct init_guard : guard_base {

        init_guard()
        {
            auto status = Mocha_InitLibrary();
            if (status != MOCHA_RESULT_SUCCESS)
                throw error{status};
            valid = true;
        }

        ~init_guard()
        {
            if (valid)
                Mocha_DeInitLibrary();
        }

    };


    struct mount_guard : guard_base {

        const std::string name;

        mount_guard(const std::string& name,
                    const std::optional<path>& dev_path,
                    const path& mnt_path) :
            name{name}
        {
            auto status = Mocha_MountFS(name.c_str(),
                                        dev_path ? dev_path->c_str() : nullptr,
                                        mnt_path.c_str());
            if (status != MOCHA_RESULT_SUCCESS)
                throw error{status};

            valid = true;
            cout << "Mounted " << name << endl;
        }

        ~mount_guard()
        {
            if (valid) {
                Mocha_UnmountFS(name.c_str());
                cout << "Unmounted " << name << endl;
            }
        }

    };

} // namespace mocha


namespace whb {

    struct log_module : guard_base {

        log_module()
        {
            valid = WHBLogModuleInit();
        }


        ~log_module()
        {
            if (valid)
                WHBLogModuleDeinit();
        }

    };


    struct console : guard_base {


        console()
        {
            valid = WHBLogConsoleInit();
        }


        ~console()
        {
            if (valid)
                WHBLogConsoleFree();
        }


        void
        set_color(std::uint8_t r, std::uint8_t g, std::uint8_t b)
        {
            std::uint32_t color = std::uint32_t{r} << 24 |
                                  std::uint32_t{g} << 16 |
                                  std::uint32_t{b} << 8;
            WHBLogConsoleSetColor(color);
        }


        static
        void
        draw()
        {
            WHBLogConsoleDraw();
        }

    };


    struct proc {

        proc() noexcept
        { WHBProcInit(); }

        ~proc() noexcept
        { WHBProcShutdown(); }

        static
        void
        stop() noexcept
        {
            SYSLaunchMenu();
        }

        static
        bool
        is_running() noexcept
        {
            return WHBProcIsRunning();
        }


        struct quit {};

    };

} // namespace whb


enum class Stage {

    Mounting,
    List,
    Wait,
    Apply,

};



ssize_t
write_to_log(_reent*, void*, const char* ptr, size_t len)
{
    try {
        // only way to guarantee it's null-terminated
        std::string buf{ptr, len};
        if (!WHBLogWrite(buf.c_str()))
            return -1;
        whb::console::draw();
        return buf.size();
    }
    catch (...) {
        return -1;
    }
}


__attribute__((__constructor__))
void
init_stdio()
{
    static devoptab_t dev_out;
    dev_out.name = "stdout";
    dev_out.write_r = write_to_log;
    devoptab_list[STD_OUT] = &dev_out;
}


std::string
upper(const std::string& str)
{
    std::string result = str;
    for (auto& c : result)
        if (c >= 'a' && c <= 'z')
            c -= 'a' - 'A';
    return result;
}


bool
has_extension(const path& p, const std::string& ext)
{
    return upper(p.extension().string()) == upper(ext);
}


blob_t
load_file(const path& file_path)
{
    std::uintmax_t size = file_size(file_path);

    std::filebuf fb;
    if (!fb.open(file_path, std::ios::in | std::ios::binary))
        throw std::runtime_error{"unable to open for reading"};

    blob_t result(size);
    auto read = fb.sgetn(reinterpret_cast<char*>(result.data()), result.size());
    if (read <= 0)
        throw std::runtime_error{"error reading file"};
    if (static_cast<std::uintmax_t>(read) != size)
        throw std::runtime_error{"unable to read all data"};

    return result;
}


void
save_file(const path& file_path, const blob_t& data)
{
    std::filebuf fb;
    if (!fb.open(file_path, std::ios::out | std::ios::binary))
        throw std::runtime_error{"unable to open for writing"};

    auto written = fb.sputn(reinterpret_cast<const char*>(data.data()), data.size());
    if (written <= 0)
        throw std::runtime_error{"error writing file"};
    if (static_cast<blob_t::size_type>(written) != data.size())
        throw std::runtime_error{"unable to write all data"};
}


std::uint32_t
wait_button_press()
{
    const auto mask = VPAD_BUTTON_A    | VPAD_BUTTON_B |
                      VPAD_BUTTON_X    | VPAD_BUTTON_Y |
                      VPAD_BUTTON_L    | VPAD_BUTTON_R |
                      VPAD_BUTTON_ZL   | VPAD_BUTTON_ZR |
                      VPAD_BUTTON_PLUS | VPAD_BUTTON_MINUS;

    while (whb::proc::is_running()) {
        VPADStatus buf;
        int r = VPADRead(VPAD_CHAN_0, &buf, 1, nullptr);
        if (r == 1) {
            if (buf.trigger & mask)
                return buf.trigger;
        }
        whb::console::draw();
    }
    throw whb::proc::quit{};
}


blob_t
apply_patch(const blob_t& bps_patch,
            const std::map<std::uint32_t, blob_t>& sources)
{
    blob_t result;

    auto info = bps::get_info(bps_patch);

    auto src_iter = sources.find(info.crc_in);
    if (src_iter == sources.end())
        throw std::runtime_error{"BPS patch in_crc does not match any source."};

    const auto& src = src_iter->second;

    return bps::apply(bps_patch, src);
}


int main()
{
    whb::log_module log_guard;
    whb::proc proc;
    whb::console console;

    console.set_color(80, 32, 0);

    cout << "Helper program for " PACKAGE_STRING << endl;
    cout << PACKAGE_URL << endl;

    try {
        // Look up system fonts by crc32.
        std::map<std::uint32_t, blob_t> cafe_fonts;

        {
            mocha::init_guard mocha_init;
            mocha::mount_guard mount_mlc_guard{"storage_mlc", {}, "/vol/storage_mlc01"};

            const path cafe_base_path = "storage_mlc:/sys/title/0005001b/10042400/content";
            for (auto cafe_name : {"CafeCn.ttf", "CafeKr.ttf", "CafeStd.ttf", "CafeTw.ttf"}) {
                path cafe_font_path = cafe_base_path / cafe_name;
                try {
                    blob_t cafe_font = load_file(cafe_font_path);
                    auto crc32 = calc_crc32(cafe_font);
                    cout << "Loaded "
                         << std::setw(7) << cafe_name << ": crc32 = "
                         << std::hex << std::setw(8) << std::setfill('0')
                         << crc32
                         << std::dec
                         << endl;
                    cafe_fonts[crc32] = std::move(cafe_font);
                }
                catch (std::exception& e) {
                    cout << "Error with \"" << cafe_name << ":\n"
                         << e.what()
                         << endl;
                }
            }
        }

        const path sd_fonts_path = "fs:/vol/external01/wiiu/fonts";
        if (!exists(sd_fonts_path))
            throw std::runtime_error{"\"wiiu/fonts/\" not found in SD card!"};


        std::vector<path> patch_paths;
        for (const auto& entry : std::filesystem::directory_iterator{sd_fonts_path}) {
            if (!entry.is_regular_file())
                continue;
            if (has_extension(entry, ".bps"))
                patch_paths.push_back(entry.path());
        }

        cout << "BPS patches found: " << patch_paths.size() << ".\n"
             << endl;

        cout << "Press A to generate fonts from .bps patches,\n"
             << "or any other button to cancel."
             << endl;
        cout << "**This safe, it will NOT modify your NAND.**" << endl;

        auto btn = wait_button_press();
        if ((btn & VPAD_BUTTON_A) == 0)
            throw std::runtime_error{"Canceled by user."};

        cout << "\nGenerating fonts..." << endl;
        unsigned num_errors = 0;
        for (const auto& patch_path : patch_paths) {
            try {
                path output_path = patch_path;
                output_path.replace_extension(".ttf");
                if (exists(output_path)) {
                    cout << "Skipped: "
                         << output_path.filename()
                         << " already exists."
                         << endl;
                    continue;
                }

                blob_t patch = load_file(patch_path);
                cout << "Processing " << patch_path.filename() << endl;
                blob_t output = apply_patch(patch, cafe_fonts);
                save_file(output_path, output);
                cout << "Saved " << output_path.filename() << endl;
            }
            catch (std::exception& e) {
                cout << "Error in " << patch_path.filename() << "\n"
                     << e.what()
                     << endl;
                ++num_errors;
            }
        }

        cout << "Finished." << "\n\n"
             << "Press HOME and close this app." << endl;

        while (proc.is_running())
            console.draw();

    }
    catch (whb::proc::quit) {
        cout << "Quitting..." << endl;
    }
    catch (std::exception& e) {
        cout << "ERROR:\n"
             << e.what() << "\n\n"
             << "Press HOME and close this app."
             << endl;

        while (proc.is_running())
            console.draw();
    }

}
