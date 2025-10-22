#include <opencv2/opencv.hpp>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <condition_variable>

static GMainLoop *loop;
static std::atomic<bool> is_running(true);
static std::mutex frame_mutex;
static std::condition_variable frame_cv;
static cv::Mat latest_frame;
static bool new_frame_available = false;
static GstAppSrc *appsrc = nullptr;
static GstClockTime timestamp = 0;

std::string generate_filename() {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm *ltm = std::localtime(&time);
    char filename[100];
    std::strftime(filename, sizeof(filename), "%Y-%m-%d_%H-%M-%S.mp4", ltm);
    return std::string(filename);
}

static void need_data(GstElement *appsrc, guint unused, gpointer user_data) {
    std::lock_guard<std::mutex> lock(frame_mutex);
    if (!latest_frame.empty()) {
        cv::Mat yuv_frame;
        cv::cvtColor(latest_frame, yuv_frame, cv::COLOR_BGR2YUV_I420);
        
        GstBuffer *buffer = gst_buffer_new_allocate(NULL, yuv_frame.total(), NULL);
        if (!buffer) {
            g_print("Failed to create buffer\n");
            return;
        }

        GstMapInfo map;
        if (!gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
            g_print("Failed to map buffer\n");
            gst_buffer_unref(buffer);
            return;
        }

        memcpy(map.data, yuv_frame.data, yuv_frame.total());
        gst_buffer_unmap(buffer, &map);
        
        GST_BUFFER_PTS(buffer) = timestamp;
        GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 15);
        timestamp += GST_BUFFER_DURATION(buffer);
        
        GstFlowReturn ret;
        g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
        
        if (ret != GST_FLOW_OK) {
            g_print("Push buffer returned %d\n", ret);
        }
        
        gst_buffer_unref(buffer);
    }
}

static void media_configure(GstRTSPMediaFactory *factory, GstRTSPMedia *media, gpointer user_data) {
    GstElement *element = gst_rtsp_media_get_element(media);
    appsrc = GST_APP_SRC(gst_bin_get_by_name_recurse_up(GST_BIN(element), "source"));
    
    g_object_set(G_OBJECT(appsrc),
                "format", GST_FORMAT_TIME,
                "is-live", TRUE,
                "do-timestamp", FALSE,
                NULL);
                
    gst_app_src_set_stream_type(appsrc, GST_APP_STREAM_TYPE_STREAM);
    
    timestamp = 0;
    
    GstCaps *caps = gst_caps_new_simple("video/x-raw",
                                       "format", G_TYPE_STRING, "I420",
                                       "width", G_TYPE_INT, 800,
                                       "height", G_TYPE_INT, 600,
                                       "framerate", GST_TYPE_FRACTION, 15, 1,
                                       NULL);
    gst_app_src_set_caps(appsrc, caps);
    gst_caps_unref(caps);
    
    g_signal_connect(appsrc, "need-data", G_CALLBACK(need_data), NULL);
    
    gst_object_unref(element);
}

void start_video_saving_thread() {
    std::thread([&]() {
        while (is_running) {
            std::string filename = generate_filename();
            g_print("Saving video to file: %s\n", filename.c_str());

            cv::VideoWriter writer(filename, cv::VideoWriter::fourcc('a', 'v', 'c', '1'), 15, cv::Size(800, 600));
            
            auto start_time = std::chrono::steady_clock::now();
            while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start_time).count() < 60) {
                std::unique_lock<std::mutex> lock(frame_mutex);
                frame_cv.wait(lock, [] { return new_frame_available; });
                
                if (!latest_frame.empty()) {
                    writer.write(latest_frame);
                }
                new_frame_available = false;
            }
            
            writer.release();
            g_print("Video saved: %s\n", filename.c_str());
        }
    }).detach();
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);
    
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open webcam!" << std::endl;
        return -1;
    }
    
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 800);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 600);
    cap.set(cv::CAP_PROP_FPS, 15);
    
    GstRTSPServer *server = gst_rtsp_server_new();
    g_object_set(server, "service", "8554", NULL);
    
    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);
    
    std::string launch_str = 
        "( appsrc name=source is-live=true block=true format=time ! "
        "videoconvert ! "
        "x264enc tune=zerolatency speed-preset=ultrafast bitrate=2000 key-int-max=15 ! "
        "h264parse ! "
        "rtph264pay config-interval=1 name=pay0 pt=96 )";
    
    GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory, launch_str.c_str());
    gst_rtsp_media_factory_set_shared(factory, TRUE);
    gst_rtsp_media_factory_set_latency(factory, 0);
    
    g_signal_connect(factory, "media-configure", G_CALLBACK(media_configure), NULL);
    
    gst_rtsp_mount_points_add_factory(mounts, "/test", factory);
    g_object_unref(mounts);
    
    gst_rtsp_server_attach(server, NULL);
    g_print("Stream ready at rtsp://127.0.0.1:8554/test\n");
    
    loop = g_main_loop_new(NULL, FALSE);
    
    std::thread streaming_thread([&]() {
        cv::Mat frame;
        using namespace std::chrono;
        auto next_frame_time = steady_clock::now();
        const auto frame_duration = duration_cast<nanoseconds>(duration<double>(1.0/15.0));
        
        while (is_running) {
            cap >> frame;
            if (frame.empty()) {
                std::cerr << "Error: Could not capture frame!" << std::endl;
                break;
            }
            
            {
                std::lock_guard<std::mutex> lock(frame_mutex);
                latest_frame = frame.clone();
                new_frame_available = true;
                frame_cv.notify_one();
            }
            
            next_frame_time += frame_duration;
            std::this_thread::sleep_until(next_frame_time);
        }
    });
    
    start_video_saving_thread();
    
    g_main_loop_run(loop);
    
    is_running = false;
    streaming_thread.join();
    
    if (appsrc) {
        gst_object_unref(appsrc);
    }
    g_main_loop_unref(loop);
    
    return 0;
}
