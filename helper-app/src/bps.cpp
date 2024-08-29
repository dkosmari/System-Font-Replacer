/*
 * System Font Replacer - A plugin to temporarily replace the Wii U's system font.
 *
 * Copyright (C) 2024  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// Loosely inspired by the code from Flips

#include <cstring>
#include <stdexcept>
#include <string>

#include "bps.hpp"
#include "crc32.hpp"

using std::byte;
using std::intmax_t;
using std::span;
using std::to_string;
using std::uint32_t;
using std::uint64_t;
using std::uint8_t;
using std::uintmax_t;


using span_size_t = span<byte>::size_type;

using namespace std::literals;


namespace bps {


    error::error(const char* msg) :
        std::runtime_error{"BPS error: "s + msg}
    {}


    error::error(const std::string& msg) :
        std::runtime_error{"BPS error: "s + msg}
    {}



    template<typename T>
    bool
    safe_assign_lshift(T& a, unsigned b)
    {
        if (b >= 8 * sizeof a) {
            a = 0;
            return false;
        }
        T sa = a << b;
        // if shifting back produces a different result, bits were lost
        if ((sa >> b) != a) {
            a = sa;
            return false;
        }
        a = sa;
        return true;
    }


    template<typename T>
    bool
    safe_assign_add(T& a, T b)
    {
        T c;
        if (__builtin_add_overflow(a, b, &c)) {
            a = c;
            return false;
        }
        a = c;
        return true;
    }



    struct byte_istream {

        span<const byte> data_span;
        span_size_t pos = 0;

        byte_istream(span<const byte> data_span) :
            data_span{data_span}
        {}


        void
        seek(span_size_t new_pos)
        {
            pos = new_pos;
        }


        void
        advance(span_size_t delta)
        {
            pos += delta;
        }


        void
        rewind(span_size_t delta)
        {
            if (delta > pos)
                throw std::out_of_range{"seeking to negative offset"};
            pos -= delta;
        }


        uint8_t
        read()
        {
            if (pos >= data_span.size())
                throw std::out_of_range{"read() pos="
                                        + to_string(pos)};
            return to_integer<uint8_t>(data_span[pos++]);
        }


        uint8_t
        read_from(span_size_t idx)
        {
            if (idx >= data_span.size())
                throw std::out_of_range{"read_from("
                                        + to_string(idx)
                                        + ")"};
            return to_integer<uint8_t>(data_span[idx]);
        }


        span<const byte>
        read(span_size_t size)
        {
            if (pos + size > data_span.size())
                throw std::out_of_range{"read("
                                        + to_string(size)
                                        + ") pos="
                                        + to_string(pos)};
            auto result = data_span.subspan(pos, size);
            pos += size;
            return result;
        }


        span<const byte>
        read_from(span_size_t idx, span_size_t size)
        {
            if (idx + size > data_span.size())
                throw std::out_of_range{"read_from("
                                        + to_string(idx)
                                        + ", "
                                        + to_string(size)
                                        + ")"};
            return data_span.subspan(idx, size);
        }



        uint32_t
        read_le32()
        {
            try {
                uint32_t val =  read();
                val |= uint32_t{read()} << 8;
                val |= uint32_t{read()} << 16;
                val |= uint32_t{read()} << 24;
                return val;
            }
            catch (std::out_of_range& e) {
                throw std::out_of_range{"read_le32(): "
                                        + std::string(e.what())};
            }
        }


        uintmax_t
        read_varint()
        {
            try {
                unsigned shifted = 0;
                uintmax_t result = 0;
                while (true) {
                    uint8_t next = read();
                    uintmax_t val = next & 0x7f;
                    if (shifted)
                        ++val;
                    if (!safe_assign_lshift(val, shifted))
                        throw std::runtime_error{"read_varint(): incorrect varint encoding"};
                    if (!safe_assign_add(result, val))
                        throw std::overflow_error{"read_varint(): overflow reading varint"};
                    if (next & 0x80)
                        break;
                    shifted += 7;
                }
                return result;
            }
            catch (std::out_of_range& e) {
                throw std::out_of_range{"read_varint(): "
                                        + std::string(e.what())};
            }
        }


        bool
        eof()
            const noexcept
        {
            return pos >= data_span.size();
        }

    };


    struct byte_ostream {

        std::vector<byte>& data_vec;

        byte_ostream(std::vector<byte>& data_vec) :
            data_vec(data_vec)
        {}


        void
        write(byte b)
        {
            data_vec.push_back(b);
        }


        void
        write(uint8_t u)
        {
            write(byte{u});
        }


        void
        write(span<const byte> blob)
        {
            for (auto b : blob)
                write(b);
        }

    };


    struct byte_stream : byte_ostream, byte_istream {

        byte_stream(std::vector<byte>& data_vec) :
            byte_ostream{data_vec},
            byte_istream{data_vec}
        {}


        void
        write(byte b)
        {
            byte_ostream::write(b);
            data_span = data_vec;
        }


        void
        write(uint8_t u)
        {
            write(byte{u});
        }


        void
        write(span<const byte> blob)
        {
            byte_ostream::write(blob);
            data_span = data_vec;
        }

    };


    info
    get_info(span<const byte> patch)
    {
        /*
         *  Must be big enough to contain:
         *    - magic (4 bytes)
         *    - 3 varints (3+ bytes)
         *    - 3 crc32 (12 bytes) at the end
         */
        if (patch.size() < 4 + 3 + 12)
            throw error{"broken BPS: incomplete"};

        byte_istream stream{patch};

        auto patch_magic = stream.read(4);
        if (std::memcmp(patch_magic.data(), "BPS1", 4))
            throw error{"broken BPS: bad magic"};

        info result;

        try {
            result.size_in  = stream.read_varint();
            result.size_out = stream.read_varint();
            auto meta_size  = stream.read_varint();
            result.meta_start = stream.pos;
            result.data_start = stream.pos + meta_size;
        }
        catch (std::exception& e) {
            throw error{"invalid size in BPS"};
        }

        stream.seek(patch.size() - 12);
        result.crc_in    = stream.read_le32();
        result.crc_out   = stream.read_le32();
        result.crc_patch = stream.read_le32();

        return result;
    }



    enum action : uint8_t {
        source_read,
        target_read,
        source_copy,
        target_copy,
    };


    std::vector<byte>
    apply(span<const byte> patch,
          span<const byte> input)
    {
        info pinfo = get_info(patch);

        uint32_t patch_crc = calc_crc32(patch.subspan(0, patch.size() - 4));

        if (patch_crc != pinfo.crc_patch)
            throw error{"broken BPS: CRC32 mismatch"};

        if (input.size() != pinfo.size_in)
            throw error{"bad input: size mismatch"};

        uint32_t input_crc = calc_crc32(input);
        if (input_crc != pinfo.crc_in)
            throw error{"bad input: CRC32 mismatch"};

        std::vector<byte> output;
        output.reserve(pinfo.size_out);

        // Note: BPS patch is allowed to use 2 of the the CRC32s at the end, as extra
        // usable data
        byte_istream patch_stream{patch.subspan(pinfo.data_start,
                                                patch.size() - pinfo.data_start - 4)};
        // Note: patch itself cannot read into the CRC32 area.
        auto patch_data_size = patch.size() - pinfo.data_start - 12;

        byte_istream source_stream{input};
        byte_stream target_stream{output};

        uintmax_t act_idx = 0;

        while (patch_stream.pos < patch_data_size) {
            auto instr = patch_stream.read_varint();
            const action act = static_cast<action>(instr & 3);
            auto length = (instr >> 2) + 1;
            uintmax_t delta;

            switch (act) {

            case action::source_read:
                try {
                    auto pos = output.size();
                    target_stream.write(source_stream.read_from(pos, length));
                }
                catch (std::exception& e) {
                    throw error{"patch.pos=" + to_string(patch_stream.pos)
                                + ", idx=" + to_string(act_idx)
                                + ", action=SourceRead"
                                + ", length=" + to_string(length)
                                + ", source.pos=" + to_string(source_stream.pos)
                                + ", target.size=" + to_string(target_stream.data_vec.size())
                                + ", what=" + std::string(e.what())};
                }
                break;

            case action::target_read:
                try {
                    target_stream.write(patch_stream.read(length));
                }
                catch (std::exception& e) {
                    throw error{"patch.pos=" + to_string(patch_stream.pos)
                                + ", idx=" + to_string(act_idx)
                                + ", action=TargetRead"
                                + ", length=" + to_string(length)
                                + ", target.size=" + to_string(target_stream.data_vec.size())
                                + ", what=" + std::string(e.what())};
                }
                break;

            case action::source_copy:
                try {
                    delta = 0;
                    delta = patch_stream.read_varint();
                    if (delta & 1)
                        source_stream.rewind(delta >> 1);
                    else
                        source_stream.advance(delta >> 1);
                    target_stream.write(source_stream.read(length));
                }
                catch (std::exception& e) {
                    intmax_t rel = (delta >> 1);
                    if (delta & 1)
                        rel = -rel;
                    throw error{"patch.pos=" + to_string(patch_stream.pos)
                                + ", idx=" + to_string(act_idx)
                                + ", action=SourceCopy"
                                + ", length=" + to_string(length)
                                + ", source.pos=" + to_string(source_stream.pos)
                                + ", rel=" + to_string(rel)
                                + ", target.size=" + to_string(target_stream.data_vec.size())
                                + ", what=" + std::string(e.what())};
                }
                break;

            case action::target_copy:
                try {
                    delta = 0;
                    delta = patch_stream.read_varint();
                    if (delta & 1)
                        target_stream.rewind(delta >> 1);
                    else
                        target_stream.advance(delta >> 1);
                    // Note: we can't work with spans, because we might read a byte we
                    // just wrote.
                    while (length--)
                        target_stream.write(target_stream.read());
                }
                catch (std::exception& e) {
                    intmax_t rel = (delta >> 1);
                    if (delta & 1)
                        rel = -rel;
                    throw error{"patch.pos=" + to_string(patch_stream.pos)
                                + ", idx=" + to_string(act_idx)
                                + ", action=TargetCopy"
                                + ", length=" + to_string(length)
                                + ", target.pos=" + to_string(target_stream.pos)
                                + ", rel=" + to_string(rel)
                                + ", target.size=" + to_string(target_stream.data_vec.size())
                                + ", what=" + std::string(e.what())};
                }
                break;

            } // switch (act)

            ++act_idx;
        }


        if (output.size() != pinfo.size_out)
            throw error{"broken BPS: output size mismatch"};

        uint32_t output_crc = calc_crc32(output);
        if (output_crc != pinfo.crc_out)
            throw error{"input mismatch"};

        return output;
    }

} // namespace bps
