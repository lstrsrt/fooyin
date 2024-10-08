/*
 * Fooyin
 * Copyright © 2023, Luke Taylor <LukeT1@proton.me>
 *
 * Fooyin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Fooyin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Fooyin.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#if defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#if defined(__GNUG__)
#pragma GCC diagnostic pop
#elif defined(__clang__)
#pragma clang diagnostic pop
#endif

#include <memory>

namespace Fooyin {
struct CodecContextDeleter
{
    void operator()(AVCodecContext* context) const
    {
        if(context) {
            avcodec_free_context(&context);
        }
    }
};
using CodecContextPtr = std::unique_ptr<AVCodecContext, CodecContextDeleter>;

class Codec
{
public:
    Codec() = default;
    Codec(CodecContextPtr context, AVStream* stream);

    Codec(Codec&& other) noexcept;
    Codec& operator=(Codec&& other) noexcept;

    Codec(const Codec& other)            = delete;
    Codec& operator=(const Codec& other) = delete;

    [[nodiscard]] bool isValid() const;

    [[nodiscard]] AVCodecContext* context() const;
    [[nodiscard]] AVStream* stream() const;
    [[nodiscard]] int streamIndex() const;
    [[nodiscard]] bool isPlanar() const;

private:
    CodecContextPtr m_context;
    AVStream* m_stream;
};
} // namespace Fooyin
