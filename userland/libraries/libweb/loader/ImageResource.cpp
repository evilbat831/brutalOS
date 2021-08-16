/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

// includes
#include <libgfx/Bitmap.h>
#include <libimagedecoderclient/Client.h>
#include <libweb/loader/ImageResource.h>

namespace Web {

ImageResource::ImageResource(const LoadRequest& request)
    : Resource(Type::Image, request)
{
}

ImageResource::~ImageResource()
{
}

int ImageResource::frame_duration(size_t frame_index) const
{
    decode_if_needed();
    if (frame_index >= m_decoded_frames.size())
        return 0;
    return m_decoded_frames[frame_index].duration;
}

static ImageDecoderClient::Client& image_decoder_client()
{
    static RefPtr<ImageDecoderClient::Client> image_decoder_client;
    if (!image_decoder_client) {
        image_decoder_client = ImageDecoderClient::Client::construct();
        image_decoder_client->on_death = [&] {
            image_decoder_client = nullptr;
        };
    }
    return *image_decoder_client;
}

void ImageResource::decode_if_needed() const
{
    if (!has_encoded_data())
        return;

    if (m_has_attempted_decode)
        return;

    if (!m_decoded_frames.is_empty())
        return;

    NonnullRefPtr decoder = image_decoder_client();
    auto image = decoder->decode_image(encoded_data());

    if (image.has_value()) {
        m_loop_count = image.value().loop_count;
        m_animated = image.value().is_animated;
        m_decoded_frames.resize(image.value().frames.size());
        for (size_t i = 0; i < m_decoded_frames.size(); ++i) {
            auto& frame = m_decoded_frames[i];
            frame.bitmap = image.value().frames[i].bitmap;
            frame.duration = image.value().frames[i].duration;
        }
    }

    m_has_attempted_decode = true;
}

const Gfx::Bitmap* ImageResource::bitmap(size_t frame_index) const
{
    decode_if_needed();
    if (frame_index >= m_decoded_frames.size())
        return nullptr;
    return m_decoded_frames[frame_index].bitmap;
}

void ImageResource::update_volatility()
{
    bool visible_in_viewport = false;
    for_each_client([&](auto& client) {
        if (static_cast<const ImageResourceClient&>(client).is_visible_in_viewport())
            visible_in_viewport = true;
    });

    if (!visible_in_viewport) {
        for (auto& frame : m_decoded_frames) {
            if (frame.bitmap)
                frame.bitmap->set_volatile();
        }
        return;
    }

    bool still_has_decoded_image = true;
    for (auto& frame : m_decoded_frames) {
        if (!frame.bitmap) {
            still_has_decoded_image = false;
        } else {
            bool was_purged = false;
            bool bitmap_has_memory = frame.bitmap->set_nonvolatile(was_purged);
            if (!bitmap_has_memory || was_purged)
                still_has_decoded_image = false;
        }
    }
    if (still_has_decoded_image)
        return;

    m_decoded_frames.clear();
    m_has_attempted_decode = false;
}

ImageResourceClient::~ImageResourceClient()
{
}

}
