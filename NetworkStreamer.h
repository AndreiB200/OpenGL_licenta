#include <vector>
#include <string>
#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <cstring>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

class FFmpegStreamer {
private:
    struct FrameData {
        std::vector<unsigned char> color; // RGBA (src_width * src_height * 4)
        std::vector<unsigned char> depth; // GL_UNSIGNED_BYTE (src_width * src_height * 1)
    };

    // FFmpeg structs
    AVFormatContext* fmt_ctx = nullptr;
    AVCodecContext* codec_ctx = nullptr;
    AVStream* stream = nullptr;
    SwsContext* sws_ctx = nullptr;
    AVFrame* frame_in = nullptr;
    AVFrame* frame_out = nullptr;

    size_t src_width;
    size_t src_height;
    size_t combined_width; // src_width * 2 (Lățimea celor două imagini lipite)

    int dst_width;         // Rezoluția finală de streaming
    int dst_height;
    int fps = 30;
    int64_t frame_pts = 0;

    // Threading
    std::thread worker_thread;
    std::mutex queue_mutex;
    std::condition_variable cv;
    std::queue<FrameData> frame_queue;
    bool running = true;
    const size_t MAX_QUEUE_SIZE = 2;

    void InitFFmpeg(const std::string& target_ip, int port) {
        avformat_network_init();

        // Folosim protocolul UDP pentru streaming cu latență minimă
        std::string url = "udp://" + target_ip + ":" + std::to_string(port) + "?pkt_size=1316";

        avformat_alloc_output_context2(&fmt_ctx, nullptr, "mpegts", url.c_str());
        if (!fmt_ctx) {
            throw std::runtime_error("Cannot allocate context for FFmpeg");
        }

        // Căutăm encoderul H.264
        const AVCodec* codec = avcodec_find_encoder_by_name("libx264");
        if (!codec) {
            std::cout << "[Warning] libx264 (software) not found, checking for openh264...\n";
            codec = avcodec_find_encoder_by_name("openh264");
        }
        if (!codec) {
            codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        }

        stream = avformat_new_stream(fmt_ctx, nullptr);
        codec_ctx = avcodec_alloc_context3(codec);

        // Configurare encoder pentru Latență Ultra-Scăzută (Zerolatency)
        codec_ctx->width = dst_width;
        codec_ctx->height = dst_height;
        codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P; // Standardul de compresie video
        codec_ctx->time_base = { 1, fps };
        codec_ctx->framerate = { fps, 1 };
        codec_ctx->gop_size = 60;
        codec_ctx->max_b_frames = 0;

        av_opt_set(codec_ctx->priv_data, "preset", "ultrafast", 0);
        av_opt_set(codec_ctx->priv_data, "tune", "zerolatency", 0);
        av_opt_set(codec_ctx->priv_data, "profile", "baseline", 0);
        codec_ctx->bit_rate = 500000; // ~1.5 Mbps pentru rezoluție dublă

        if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
            codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
            throw std::runtime_error("Nu s-a putut deschide codecul.");
        }

        avcodec_parameters_from_context(stream->codecpar, codec_ctx);

        if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
            if (avio_open(&fmt_ctx->pb, url.c_str(), AVIO_FLAG_WRITE) < 0) {
                throw std::runtime_error("Nu s-a putut deschide socket-ul UDP către " + url);
            }
        }

        avformat_write_header(fmt_ctx, nullptr);

        // Alocăm frame-ul de input pe lățimea COMBINATĂ (RGBA)
        frame_in = av_frame_alloc();
        frame_in->format = AV_PIX_FMT_RGBA;
        frame_in->width = combined_width;
        frame_in->height = src_height;
        av_frame_get_buffer(frame_in, 0);

        // Alocăm frame-ul de output pe lățimea COMBINATĂ (YUV420P)
        frame_out = av_frame_alloc();
        frame_out->format = AV_PIX_FMT_YUV420P;
        frame_out->width = dst_width;
        frame_out->height = dst_height;
        av_frame_get_buffer(frame_out, 0);

        // Inițializăm contextul de conversie din RGBA (dublu) în YUV420P (dublu)
        sws_ctx = sws_getContext(
            combined_width, src_height, AV_PIX_FMT_RGBA,
            dst_width, dst_height, AV_PIX_FMT_YUV420P,
            SWS_FAST_BILINEAR, nullptr, nullptr, nullptr
        );
    }

    void WorkerLoop() {
        while (running) {
            FrameData local_frame;

            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                cv.wait(lock, [this] { return !frame_queue.empty() || !running; });

                if (!running && frame_queue.empty()) break;

                local_frame = std::move(frame_queue.front());
                frame_queue.pop();
            }

            av_frame_make_writable(frame_in);

            uint8_t* dest_ptr = frame_in->data[0];
            int dest_stride = frame_in->linesize[0];

            int color_row_stride = src_width * 4; // Culoarea are 4 canale (RGBA)

            // LIPIREA CELOR DOUĂ IMAGINI LINIE CU LINIE
            for (size_t y = 0; y < src_height; ++y) {
                uint8_t* row_dest = dest_ptr + (y * dest_stride);

                // 1. Copiem linia Y din imaginea COLOR pe partea STÂNGĂ
                uint8_t* color_src = local_frame.color.data() + (y * color_row_stride);
                std::memcpy(row_dest, color_src, color_row_stride);

                uint8_t* depth_dest = row_dest + color_row_stride;
                uint8_t* depth_src = local_frame.depth.data() + (y * color_row_stride);
                std::memcpy(depth_dest, depth_src, color_row_stride);
            }

            // Realizăm conversia de culoare și scalarea din RGBA în YUV420P
            av_frame_make_writable(frame_out);
            sws_scale(
                sws_ctx, frame_in->data, frame_in->linesize, 0, src_height,
                frame_out->data, frame_out->linesize
            );

            frame_out->pts = frame_pts++;

            // Trimiterea frame-ului către encoder
            int ret = avcodec_send_frame(codec_ctx, frame_out);
            if (ret < 0) continue;

            AVPacket* pkt = av_packet_alloc();
            while (ret >= 0) {
                ret = avcodec_receive_packet(codec_ctx, pkt);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                }
                else if (ret < 0) {
                    break;
                }

                av_packet_rescale_ts(pkt, codec_ctx->time_base, stream->time_base);
                pkt->stream_index = stream->index;

                av_interleaved_write_frame(fmt_ctx, pkt);
                av_packet_unref(pkt);
            }
            av_packet_free(&pkt);
        }
    }

public:
    FFmpegStreamer(const std::string& target_ip, int port, size_t width, size_t height)
        : src_width(width), src_height(height)
    {
        // Lățimea imaginii combinate este dublul lățimii inițiale
        combined_width = src_width * 2;

        // Păstrăm raportul pixelilor și în stream-ul de destinație
        dst_width = combined_width/2;
        dst_height = src_height/2;

        InitFFmpeg(target_ip, port);
        worker_thread = std::thread(&FFmpegStreamer::WorkerLoop, this);
        std::cout << "[FFmpeg Streamer] Stream deschis cu succes către " << target_ip << ":" << port << "\n";
        std::cout << "[FFmpeg Streamer] Rezoluție transmisie: " << dst_width << "x" << dst_height << "\n";
    }
    FFmpegStreamer(){}

    ~FFmpegStreamer() {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            running = false;
        }
        cv.notify_one();
        if (worker_thread.joinable()) {
            worker_thread.join();
        }

        // Curățare resurse FFmpeg
        if (fmt_ctx) {
            av_write_trailer(fmt_ctx);
        }
        av_frame_free(&frame_in);
        av_frame_free(&frame_out);
        sws_freeContext(sws_ctx);
        avcodec_free_context(&codec_ctx);
        if (fmt_ctx && !(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&fmt_ctx->pb);
        }
        avformat_free_context(fmt_ctx);
    }

    void PushFrame(const std::vector<unsigned char>& colorData, const std::vector<unsigned char>& depthData) {
        std::lock_guard<std::mutex> lock(queue_mutex);
        if (frame_queue.size() >= MAX_QUEUE_SIZE) {
            frame_queue.pop(); // Aruncăm cadrele vechi dacă thread-ul de streaming nu ține pasul
        }
        frame_queue.push(FrameData{ colorData, depthData });
        cv.notify_one();
    }
};