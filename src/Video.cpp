#include "Video.hpp"

// ffmpeg
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/display.h>
}

// standard library
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>


static int parseIso6709(const char *str, double &lat, double &lon) {
    if (!str || !*str) return -1;

    char *end = NULL;
    double latitude  = strtod(str, &end);
    if (end == str) return -1;          // kein gültiger Start

    double longitude = strtod(end, &end);
    if (end == str) return -1;

    lat = latitude;
    lon = longitude;
    return 0;
}


Video::Video(const fs::path &path) {
    auto &meta = metaData_;

    // open format context
    fmt_ctx = nullptr;
    if (avformat_open_input(&fmt_ctx, path.string().c_str(), nullptr, nullptr) < 0) {
        throw std::runtime_error("Can't open video file");
    }

    // load stream info and meta data
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        throw std::runtime_error("Video stream info not found");
    }

    // get creation time
    {
        const char *keys[] = {
            "creation_time",
            "date"
        };
        for (const char *key : keys) {
            AVDictionaryEntry *tag = av_dict_get(fmt_ctx->metadata, key, NULL, 0);
            if (!tag)
                continue;

            auto in = std::istringstream(tag->value);
            in >> std::chrono::parse("%Y-%m-%dT%H:%M:%S", meta.time);
            meta.localTime = true;
        }
    }

    // get geo location
    {
        const char *keys[] = {
            "location",
            "location-eng",
            "com.apple.quicktime.location.ISO6709"
        };
        for (const char *key : keys) {
            AVDictionaryEntry *tag = av_dict_get(fmt_ctx->metadata, key, NULL, 0);
            if (!tag)
                continue;

            if (parseIso6709(tag->value, meta.latitude, meta.longitude) == 0)
                break;
        }
    }

    // find video stream
    video_stream_idx = -1;
    for (unsigned i = 0; i < fmt_ctx->nb_streams; ++i) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
            break;
        }
    }
    if (video_stream_idx == -1) {
        throw std::runtime_error("No video-stream found");
    }

    AVStream* video_stream = fmt_ctx->streams[video_stream_idx];
    AVCodecParameters* codecpar = video_stream->codecpar;

    // find decoder and open
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        throw std::runtime_error("Video decoder not found");
    }

    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        throw std::runtime_error("Could not allocate video codec context");
    }

    if (avcodec_parameters_to_context(codec_ctx, codecpar) < 0) {
        throw std::runtime_error("Could not copy video parameters");
    }

    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        throw std::runtime_error("Could not open video codec");
    }

    // allocate packet + frame
    packet = av_packet_alloc();
    frame = av_frame_alloc();
    if (!packet || !frame) {
        throw std::runtime_error("Allocation of video packet/frame failed");
    }

    // get orientation
    const AVPacketSideData *sd = av_packet_side_data_get(
        video_stream->codecpar->coded_side_data,
        video_stream->codecpar->nb_coded_side_data,
        AV_PKT_DATA_DISPLAYMATRIX
    );
    if (sd && sd->data) {
        double theta = av_display_rotation_get((const int32_t*)sd->data);
        if (theta > 135)
            orientation_ = 3;
        else if (theta > 45)
            orientation_ = 8;
        else if (theta > -45)
            orientation_ = 0;
        else if (theta > -135)
            orientation_ = 6;
        else
            orientation_ = 3;
    }

    // get meta data
    // todo
    metaData_.frameDuration = 1;
}

Video::~Video() {
    if (rgb_data[0]) {
        av_freep(&rgb_data[0]);
    }
    sws_freeContext(sws_ctx);

    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
}

ImageData Video::getImageData() {
    while (true) {
        // get a frame from the current packet
        int ret = avcodec_receive_frame(codec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            while (true) {
                // read next packet
                av_packet_unref(packet);
                ret = av_read_frame(fmt_ctx, packet);
                if (ret >= 0) {
                    if (packet->stream_index == video_stream_idx) {
                        // send packet to decoder
                        ret = avcodec_send_packet(codec_ctx, packet);
                        if (ret < 0) {
                            std::cerr << "Sending video packet to decoder failed" << std::endl;
                        } else {
                            // we have a new packet, try to read a frame
                            break;
                        }
                    }
                } else if (ret == AVERROR_EOF) {
                    // end of video: skip to beginning
                    ret = av_seek_frame(fmt_ctx, video_stream_idx, 0, AVSEEK_FLAG_BACKWARD);
                    if (ret < 0) {
                        // Fehlerbehandlung
                        char errbuf[AV_ERROR_MAX_STRING_SIZE];
                        av_strerror(ret, errbuf, sizeof(errbuf));
                        std::cerr << "Seek failed: " << errbuf << std::endl;
                    }

                    // clear decoder buffer
                    avcodec_flush_buffers(codec_ctx);
                }
            }
        } else if (ret < 0) {
            std::cerr << "Receiving video frame failed" << std::endl;
        } else {
            break;
        }
    }

    // reallocate scaler
    if (!sws_ctx ||
        sws_ctx->src_w != frame->width ||
        sws_ctx->src_h != frame->height ||
        sws_ctx->src_format != frame->format)
    {
        sws_freeContext(sws_ctx);
        sws_ctx = sws_getContext(
            frame->width, frame->height, AVPixelFormat(frame->format), // source
            frame->width, frame->height, AV_PIX_FMT_RGB24,             // destination
            SWS_BILINEAR,
            nullptr, nullptr, nullptr);

        if (!sws_ctx) {
            // error
            return {};
        }

        // reallocate RGB buffer
        if (rgb_data[0]) {
            av_freep(&rgb_data[0]);
        }
        rgb_bufsize = av_image_alloc(rgb_data, rgb_linesize,
            frame->width, frame->height,
            AV_PIX_FMT_RGB24, 1);
        if (rgb_bufsize < 0) {
            // error
            return {};
        }
    }

    // convert to RGB
    sws_scale(sws_ctx,
        frame->data, frame->linesize, 0, frame->height, // source
        rgb_data, rgb_linesize);                        // destination
    width_ = frame->width;
    height_ = frame->height;

    av_frame_unref(frame);

    return {width_, height_, orientation_, rgb_data[0]};
}

MetaData Video::getMetaData() {
    return metaData_;
}
