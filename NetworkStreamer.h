#include <vector>
#include <string>
#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <cstring>
#include <zmq.hpp>

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
        std::vector<float> depth;
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

        // 1. Configurare URL UDP pentru low-latency (fără buffer pe socket)
        std::string url = "udp://" + target_ip + ":" + std::to_string(port) +
            "?pkt_size=1316&buffer_size=65536&fifo_size=0&overrun_nonfatal=1";

        avformat_alloc_output_context2(&fmt_ctx, nullptr, "mpegts", url.c_str());
        if (!fmt_ctx) {
            throw std::runtime_error("Cannot allocate context for FFmpeg");
        }

        // Elimină buffer-ul de format și forțează trimiterea imediată a pachetelor
        fmt_ctx->flags |= AVFMT_FLAG_NOBUFFER | AVFMT_FLAG_FLUSH_PACKETS;

        const AVCodec* codec = avcodec_find_encoder_by_name("libx264");
        if (!codec) {
            codec = avcodec_find_encoder_by_name("openh264");
        }
        if (!codec) {
            codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        }
        if (!codec) {
            throw std::runtime_error("Nu s-a găsit niciun encoder H.264 compatibil.");
        }

        stream = avformat_new_stream(fmt_ctx, nullptr);
        codec_ctx = avcodec_alloc_context3(codec);

        codec_ctx->width = dst_width;
        codec_ctx->height = dst_height;
        codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;

        codec_ctx->time_base = { 1, fps };
        codec_ctx->framerate = { fps, 1 };

        // 2. Setări critice Low-Delay pentru encoder
        codec_ctx->gop_size = 15; // GOP mic (sau fps) pentru recuperare rapidă
        codec_ctx->max_b_frames = 0; // Elimină decalajul introdus de B-frames
        codec_ctx->flags |= AV_CODEC_FLAG_LOW_DELAY; // Flag FFmpeg pentru ultra-low delay

        // Opțiuni specifice libx264
        av_opt_set(codec_ctx->priv_data, "preset", "ultrafast", 0);
        av_opt_set(codec_ctx->priv_data, "tune", "zerolatency", 0);
        av_opt_set(codec_ctx->priv_data, "x264-params", "no-scenecut=1:repeat-headers=1:sliced-threads=1", 0);

        codec_ctx->bit_rate = 2000000;
        codec_ctx->rc_max_rate = 2000000;
        codec_ctx->rc_buffer_size = 1000000; // Buffer de bitrate redus la jumătate pentru CBR strict

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

        // Setare opțiuni suplimentare pentru MPEG-TS
        AVDictionary* muxer_opts = nullptr;
        av_dict_set(&muxer_opts, "mpegts_flags", "resend_headers", 0);
        av_dict_set(&muxer_opts, "muxdelay", "0", 0);

        avformat_write_header(fmt_ctx, &muxer_opts);
        av_dict_free(&muxer_opts);

        frame_in = av_frame_alloc();
        frame_in->format = AV_PIX_FMT_RGBA;
        frame_in->width = combined_width;
        frame_in->height = src_height;
        av_frame_get_buffer(frame_in, 0);

        frame_out = av_frame_alloc();
        frame_out->format = AV_PIX_FMT_YUV420P;
        frame_out->width = dst_width;
        frame_out->height = dst_height;
        av_frame_get_buffer(frame_out, 0);

        sws_ctx = sws_getContext(
            combined_width, src_height, AV_PIX_FMT_RGBA,
            dst_width, dst_height, AV_PIX_FMT_YUV420P,
            SWS_POINT, nullptr, nullptr, nullptr // SWS_POINT este cel mai rapid scaling
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

            size_t expected_pixels = static_cast<size_t>(src_width) * src_height;
            if (local_frame.color.size() < expected_pixels * 4 || local_frame.depth.size() < expected_pixels) {
                continue;
            }

            // Asigurăm scrierea în frame-ul RGBA de intrare
            if (av_frame_make_writable(frame_in) < 0) continue;

            uint8_t* dest_ptr = frame_in->data[0];
            int dest_stride = frame_in->linesize[0];

            size_t color_bytes_per_row = src_width * 4;

            for (size_t y = 0; y < src_height; ++y) {
                uint8_t* row_dest = dest_ptr + (y * dest_stride);

                const uint8_t* color_src = local_frame.color.data() + (y * color_bytes_per_row);
                std::memcpy(row_dest, color_src, color_bytes_per_row);

                uint8_t* depth_dest = row_dest + color_bytes_per_row;
                const float* depth_src_row = local_frame.depth.data() + (y * src_width);

                for (size_t x = 0; x < src_width; ++x) {
                    float depth_val = depth_src_row[x];

                    constexpr float max_depth = 10.0f;
                    float normalized = depth_val / max_depth;
                    if (normalized < 0.0f) normalized = 0.0f;
                    if (normalized > 1.0f) normalized = 1.0f;

                    uint8_t gray_val = static_cast<uint8_t>(normalized * 255.0f);

                    depth_dest[x * 4 + 0] = gray_val; // Red
                    depth_dest[x * 4 + 1] = gray_val; // Green
                    depth_dest[x * 4 + 2] = gray_val; // Blue
                    depth_dest[x * 4 + 3] = 255;      // Alpha
                }
            }

            // 2. CONVERSIA DIN RGBA ÎN YUV420P
            if (av_frame_make_writable(frame_out) < 0) continue;

            sws_scale(
                sws_ctx,
                frame_in->data, frame_in->linesize, 0, src_height,
                frame_out->data, frame_out->linesize
            );

            // Asetăm timestamp-ul pe frame-ul convertit
            frame_out->pts = frame_pts++;

            // 3. TRIMITEREA FRAME-ULUI YUV420P CĂTRE ENCODER
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
        combined_width = src_width * 2;

        dst_width = combined_width / 2;
        dst_height = src_height / 2;

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

    void PushFrame(const std::vector<unsigned char>& colorData, const std::vector<float>& depthData) {
        std::lock_guard<std::mutex> lock(queue_mutex);
        if (frame_queue.size() >= MAX_QUEUE_SIZE) {
            frame_queue.pop(); // Aruncăm cadrele vechi dacă thread-ul de streaming nu ține pasul
        }
        frame_queue.push(FrameData{ colorData, depthData });
        cv.notify_one();
    }
};



struct DronePose {
    float x;
    float y;
    float z;
    //quat
    float qw;
    float qx;
    float qy;
    float qz;
};

struct PythonCommand {
    float x;
    float y;
    float z;
    //quat
    float qw;
    float qx;
    float qy;
    float qz;
    int   flag;
};

class ZmqNode {
public:
    ZmqNode()
        : context_(1),
        pub_socket_(context_, ZMQ_PUB),
        sub_socket_(context_, ZMQ_SUB),
        is_running_(false) {
    }

    ~ZmqNode() {
        stop();
    }

    bool init(const std::string& ip = "0.0.0.0", const std::string& pub_port = "5555", const std::string& sub_port = "5556") {
        try {
            // 1. Configurare Publisher (C++ -> Python)
            std::string pub_addr = "tcp://" + ip + ":" + pub_port;
            pub_socket_.bind(pub_addr);
            std::cout << "[ZMQ PUB] Bound pe " << pub_addr << std::endl;

            // 2. Configurare Subscriber (Python -> C++)
            std::string sub_addr = "tcp://" + ip + ":" + sub_port;
            sub_socket_.bind(sub_addr);
            sub_socket_.set(zmq::sockopt::subscribe, ""); // Abonare la toate mesajele
            sub_socket_.set(zmq::sockopt::rcvtimeo, 200);  // Timeout 200ms pentru deblocare curată la stop()
            std::cout << "[ZMQ SUB] Bound pe " << sub_addr << std::endl;

            return true;
        }
        catch (const zmq::error_t& e) {
            std::cerr << "[ZMQ Error] Initializare esuata: " << e.what() << std::endl;
            return false;
        }
    }

    void sendHistogram(const std::vector<float>& histogram) {
        if (histogram.empty()) return;

        std::string topic = "HIST";
        zmq::message_t topic_msg(topic.data(), topic.size());
        pub_socket_.send(topic_msg, zmq::send_flags::sndmore);

        size_t bytes = histogram.size() * sizeof(float);
        zmq::message_t data_msg(bytes);
        std::memcpy(data_msg.data(), histogram.data(), bytes);

        pub_socket_.send(data_msg, zmq::send_flags::dontwait);
    }

    void sendVoxelData(const std::vector<glm::vec3>& voxelGridData)
    {
        if (voxelGridData.empty()) return;

        std::string topic = "VOXEL";
        zmq::message_t topic_msg(topic.data(), topic.size());
        pub_socket_.send(topic_msg, zmq::send_flags::sndmore);

        zmq::message_t message(voxelGridData.data(), voxelGridData.size() * sizeof(glm::vec3));
        pub_socket_.send(message, zmq::send_flags::none);
    }

    void start(const DronePose& pose_to_send, PythonCommand& cmd_to_receive, int pub_delay_ms = 20) {
        if (is_running_) return;

        is_running_ = true;
        pub_thread_ = std::thread(&ZmqNode::pubWorkerLoop, this, std::ref(pose_to_send), pub_delay_ms);
        sub_thread_ = std::thread(&ZmqNode::subWorkerLoop, this, std::ref(cmd_to_receive));
    }

    void stop() {
        if (is_running_) {
            is_running_ = false;

            if (pub_thread_.joinable()) pub_thread_.join();
            if (sub_thread_.joinable()) sub_thread_.join();

            std::cout << "[ZMQ] Ambele thread-uri au fost oprite." << std::endl;
        }
    }

private:
    void pubWorkerLoop(const DronePose& pose, int delay_ms) {
        while (is_running_) {
            std::string topic = "POSE";
            zmq::message_t topic_msg(topic.data(), topic.size());
            pub_socket_.send(topic_msg, zmq::send_flags::sndmore);

            zmq::message_t msg(&pose, sizeof(DronePose));
            pub_socket_.send(msg, zmq::send_flags::none);

            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }
    }

    void subWorkerLoop(PythonCommand& cmd_target) {
        while (is_running_) {
            zmq::message_t msg;
            auto res = sub_socket_.recv(msg, zmq::recv_flags::none);

            if (res.has_value() && msg.size() == sizeof(PythonCommand)) {
                std::memcpy(&cmd_target, msg.data(), sizeof(PythonCommand));
            }
            else
            {
                cmd_target = PythonCommand{ 0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0 };
            }
        }
    }

    zmq::context_t context_;
    zmq::socket_t pub_socket_;
    zmq::socket_t sub_socket_;

    std::thread pub_thread_;
    std::thread sub_thread_;
    std::atomic<bool> is_running_;
};

class VFHZmqVisualizer {
public:
    VFHZmqVisualizer(const std::string& endpoint = "tcp://127.0.0.1:5559")
        : m_context(1), m_socket(m_context, zmq::socket_type::pub)
    {
        m_socket.bind(endpoint);
    }

    void sendHistogram(const std::vector<float>& histogram) {
        // Trimitem direct array-ul de float-uri ca pachet de octeți
        size_t bytes = histogram.size() * sizeof(float);
        zmq::message_t message(bytes);
        std::memcpy(message.data(), histogram.data(), bytes);

        m_socket.send(message, zmq::send_flags::dontwait);
    }

private:
    zmq::context_t m_context;
    zmq::socket_t m_socket;
};