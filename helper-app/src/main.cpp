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
#include <ranges>
#include <stdexcept>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include <sys/iosupport.h>

#include <padscore/kpad.h>
#include <sysapp/launch.h>
#include <vpad/input.h>
#include <whb/log.h>
#include <whb/log_console.h>
#include <whb/log_module.h>
#include <whb/proc.h>

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
using std::uint32_t;

using blob_t = std::vector<std::byte>;


struct font_info {
    uint32_t ref_crc = 0;
    std::string name;
    blob_t content;
};


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
            uint32_t color = uint32_t{r} << 24 |
                             uint32_t{g} << 16 |
                             uint32_t{b} << 8;
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


struct kpad {
    kpad() noexcept
    { KPADInit(); }

    ~kpad()
    { KPADShutdown(); }
};


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



using any_button_t = std::variant<VPADButtons,
                                  WPADButton,
                                  WPADNunchukButton,
                                  WPADClassicButton,
                                  WPADProButton>;

any_button_t
wait_for_action_button()
{

    while (whb::proc::is_running()) {

        for (auto channel : {VPAD_CHAN_0, VPAD_CHAN_1}) {
            const auto mask = VPAD_BUTTON_A    | VPAD_BUTTON_B     |
                              VPAD_BUTTON_X    | VPAD_BUTTON_Y     |
                              VPAD_BUTTON_L    | VPAD_BUTTON_R     |
                              VPAD_BUTTON_ZL   | VPAD_BUTTON_ZR    |
                              VPAD_BUTTON_PLUS | VPAD_BUTTON_MINUS;
            VPADStatus buf;
            int r = VPADRead(channel, &buf, 1, nullptr);
            if (r != 1)
                continue;
            if (buf.trigger & mask)
                return static_cast<VPADButtons>(buf.trigger & mask);
        }

        for (int channel = 0; channel < 7; ++channel) {
            KPADStatus buf;
            int r = KPADRead(static_cast<KPADChan>(channel), &buf, 1);
            if (r != 1)
                continue;
            if (buf.error)
                continue;
            switch (buf.extensionType) {
            case WPAD_EXT_NUNCHUK:
            case WPAD_EXT_MPLUS_NUNCHUK:
                {
                    const auto mask = WPAD_NUNCHUK_BUTTON_Z | WPAD_NUNCHUK_BUTTON_C;
                    if (buf.nunchuk.trigger & mask)
                        return static_cast<WPADNunchukButton>(buf.nunchuk.trigger & mask);
                }
                break;
            case WPAD_EXT_CLASSIC:
            case WPAD_EXT_MPLUS_CLASSIC:
                {
                    const auto mask = WPAD_CLASSIC_BUTTON_A    | WPAD_CLASSIC_BUTTON_B     |
                                      WPAD_CLASSIC_BUTTON_X    | WPAD_CLASSIC_BUTTON_Y     |
                                      WPAD_CLASSIC_BUTTON_L    | WPAD_CLASSIC_BUTTON_R     |
                                      WPAD_CLASSIC_BUTTON_ZL   | WPAD_CLASSIC_BUTTON_ZR    |
                                      WPAD_CLASSIC_BUTTON_PLUS | WPAD_CLASSIC_BUTTON_MINUS;
                    if (buf.classic.trigger & mask)
                        return static_cast<WPADClassicButton>(buf.classic.trigger & mask);
                }
                break;
            case WPAD_EXT_PRO_CONTROLLER:
                {
                    const auto mask = WPAD_PRO_BUTTON_A    | WPAD_PRO_BUTTON_B     |
                                      WPAD_PRO_BUTTON_X    | WPAD_PRO_BUTTON_Y     |
                                      WPAD_PRO_TRIGGER_L   | WPAD_PRO_TRIGGER_R    |
                                      WPAD_PRO_TRIGGER_ZL  | WPAD_PRO_TRIGGER_ZR   |
                                      WPAD_PRO_BUTTON_PLUS | WPAD_PRO_BUTTON_MINUS;
                    if (buf.pro.trigger & mask)
                        return static_cast<WPADProButton>(buf.pro.trigger & mask);
                }
                continue; // skip processing core buttons
            }

            const auto mask = WPAD_BUTTON_A    | WPAD_BUTTON_B     |
                              WPAD_BUTTON_1    | WPAD_BUTTON_2     |
                              WPAD_BUTTON_PLUS | WPAD_BUTTON_MINUS;
            if (buf.trigger & mask)
                return static_cast<WPADButton>(buf.trigger & mask);
        }

        whb::console::draw();
    }
    throw whb::proc::quit{};
}


blob_t
apply_patch(const blob_t& bps_patch,
            const std::map<uint32_t, font_info>& sources)
{
    blob_t result;

    auto info = bps::get_info(bps_patch);

    auto src_iter = sources.find(info.crc_in);
    if (src_iter == sources.end())
        throw std::runtime_error{"BPS patch in_crc does not match any source."};

    const auto& src = src_iter->second;

    return bps::apply(bps_patch, src.content);
}


const path sd_fonts_path = "fs:/vol/external01/wiiu/fonts";


void
generate_custom_fonts(const std::map<uint32_t, font_info>& cafe_fonts)
{
    std::vector<path> patch_paths;
    for (const auto& entry : std::filesystem::directory_iterator{sd_fonts_path}) {
        if (!entry.is_regular_file())
            continue;
        if (has_extension(entry, ".bps"))
            patch_paths.push_back(entry.path());
    }
    cout << "Generating fonts..." << endl;
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
            cout << "Error with " << patch_path.filename() << "\n"
                 << e.what()
                 << endl;
        }
    }
}


void
export_system_fonts(const std::map<uint32_t, font_info>& cafe_fonts)
{
    cout << "Exporting system fonts..." << endl;
    for (auto [crc, info] : cafe_fonts) {
        try {
            path out_path = sd_fonts_path / info.name;
            if (exists(out_path)) {
                cout << "Skipped " << info.name << ": already exists" << endl;
                continue;
            }
            save_file(out_path, info.content);
            cout << "Exported " << info.name;
            if (crc != info.ref_crc)
                cout << " (wrong crc32)";
            cout << endl;
        }
        catch (std::exception& e) {
            cout << "Error with " << info.name << "\n"
                 << e.what()
                 << endl;
        }
    }
}


template<typename... Ts>
struct overloaded : Ts... {
    using Ts::operator() ...;
};


int main()
{
    kpad kpad_guard;
    whb::log_module log_guard;
    whb::proc proc;
    whb::console console;

    WPADEnableURCC(true);

    console.set_color(80, 32, 0);

    cout << "Helper program for " PACKAGE_STRING << endl;
    cout << PACKAGE_URL << endl;

    try {
        // Look up system fonts by crc32.
        std::map<uint32_t, font_info> cafe_fonts;
        {
            mocha::init_guard mocha_init;
            mocha::mount_guard mount_mlc_guard{"storage_mlc", {}, "/vol/storage_mlc01"};

            const auto cafe_names = {
                "CafeCn.ttf",
                "CafeKr.ttf",
                "CafeStd.ttf",
                "CafeTw.ttf",
            };
            const auto cafe_crcs = {
                0X14c7272fu, // CafeCn.ttf
                0Xa2a1a55au, // CafeKr.ttf
                0Xf1252709u, // CafeStd.ttf
                0Xe5a938cdu, // CafeTw.ttf
            };
            const path cafe_base_path = "storage_mlc:/sys/title/0005001b/10042400/content";
            for (auto [name, ref_crc] : std::views::zip(cafe_names, cafe_crcs)) {
                path cafe_font_path = cafe_base_path / name;
                try {
                    blob_t content = load_file(cafe_font_path);
                    auto real_crc = calc_crc32(content);
                    const char* crc_match = real_crc == ref_crc ? "OK" : "wrong crc32";
                    cout << "Loaded "
                         << std::setw(7) << name
                         << " (" << crc_match << ")"
                         << endl;
                    auto& info = cafe_fonts[real_crc];
                    info.ref_crc = ref_crc;
                    info.name    = name;
                    info.content = std::move(content);
                }
                catch (std::exception& e) {
                    cout << "Error with \"" << name << "\":\n"
                         << e.what()
                         << endl;
                }
            }
        }

        if (!exists(sd_fonts_path))
            throw std::runtime_error{"\"SD:/wiiu/fonts/\" not found!"};

        cout << "\nWaiting for user input:\n"
             << "  - press A button to generate fonts.\n"
             << "  - press + button to export the system fonts.\n"
             << "  - press any other button to exit."
             << endl;
        cout << "\n**This safe, it will NOT modify your NAND.**" << endl;

        auto btn = wait_for_action_button();

        auto handle_vpad = [&cafe_fonts](VPADButtons btn)
        {
            if (btn & VPAD_BUTTON_A) {
                generate_custom_fonts(cafe_fonts);
                return;
            }
            if (btn & VPAD_BUTTON_PLUS) {
                export_system_fonts(cafe_fonts);
                return;
            }
            throw std::runtime_error{"Canceled by user."};
        };
        auto handle_wpad = [&cafe_fonts](WPADButton btn)
        {
            if (btn & WPAD_BUTTON_A) {
                generate_custom_fonts(cafe_fonts);
                return;
            }
            if (btn & WPAD_BUTTON_PLUS) {
                export_system_fonts(cafe_fonts);
                return;
            }
            throw std::runtime_error{"Canceled by user."};
        };
        auto handle_wpad_nunchuk = [&cafe_fonts](WPADNunchukButton)
        {
            throw std::runtime_error{"Canceled by user."};
        };
        auto handle_wpad_classic = [&cafe_fonts](WPADClassicButton btn)
        {
            if (btn & WPAD_CLASSIC_BUTTON_A) {
                generate_custom_fonts(cafe_fonts);
                return;
            }
            if (btn & WPAD_CLASSIC_BUTTON_PLUS) {
                export_system_fonts(cafe_fonts);
                return;
            }
            throw std::runtime_error{"Canceled by user."};
        };
        auto handle_wpad_pro = [&cafe_fonts](WPADProButton btn)
        {
            if (btn & WPAD_PRO_BUTTON_A) {
                generate_custom_fonts(cafe_fonts);
                return;
            }
            if (btn & WPAD_PRO_BUTTON_PLUS) {
                export_system_fonts(cafe_fonts);
                return;
            }
            throw std::runtime_error{"Canceled by user."};
        };
        visit(overloaded{handle_vpad,
                         handle_wpad,
                         handle_wpad_nunchuk,
                         handle_wpad_classic,
                         handle_wpad_pro},
            btn);

        cout << "\nFinished." << "\n"
             << "Press HOME and close this app." << endl;

        while (proc.is_running())
            console.draw();

    }
    catch (whb::proc::quit) {
        cout << "Quitting..." << endl;
    }
    catch (std::exception& e) {
        cout << "\nERROR!\n"
             << e.what() << "\n\n"
             << "Press HOME and close this app."
             << endl;

        while (proc.is_running())
            console.draw();
    }
}
