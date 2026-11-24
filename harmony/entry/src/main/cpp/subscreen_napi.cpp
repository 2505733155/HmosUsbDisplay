#include <ace/xcomponent/native_interface_xcomponent.h>
#include <hilog/log.h>
#include <napi/native_api.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include "net_server.h"
#include "touch_protocol.h"
#include "video_decoder.h"

namespace {

#define LOG(fmt, ...) ((void)OH_LOG_Print(LOG_APP, LOG_INFO, 0x0000, "SubScreen", fmt, ##__VA_ARGS__))
#define LOGE(fmt, ...) ((void)OH_LOG_Print(LOG_APP, LOG_ERROR, 0x0000, "SubScreen", fmt, ##__VA_ARGS__))

constexpr int DEFAULT_PORT = 53517;
constexpr int STATS_INTERVAL_MS = 800;

struct StatusData {
    std::string status;
    int32_t fps = 0;
    double kbps = 0;
    int32_t width = 0;
    int32_t height = 0;
    uint64_t frames = 0;
    uint64_t drops = 0;
    uint64_t errors = 0;
};

napi_threadsafe_function g_statusFn = nullptr;

void CallStatusJs(napi_env env, napi_value jsCb, void * /*context*/, void *data)
{
    auto *s = static_cast<StatusData *>(data);
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);
    napi_value obj = nullptr;
    napi_create_object(env, &obj);

    auto setStr = [&](const char *key, const std::string &value) {
        napi_value val = nullptr;
        napi_create_string_utf8(env, value.c_str(), value.size(), &val);
        napi_set_named_property(env, obj, key, val);
    };
    auto setNum = [&](const char *key, double value) {
        napi_value val = nullptr;
        napi_create_double(env, value, &val);
        napi_set_named_property(env, obj, key, val);
    };
    setStr("status", s->status);
    setNum("fps", s->fps);
    setNum("kbps", s->kbps);
    setNum("width", s->width);
    setNum("height", s->height);
    setNum("frames", static_cast<double>(s->frames));
    setNum("drops", static_cast<double>(s->drops));
    setNum("errors", static_cast<double>(s->errors));

    napi_call_function(env, undefined, jsCb, 1, &obj, nullptr);
    delete s;
}

void PushStatus(const StatusData &s)
{
    if (g_statusFn == nullptr) {
        return;
    }
    auto *copy = new StatusData(s);
    if (napi_call_threadsafe_function(g_statusFn, copy, napi_tsfn_nonblocking) != napi_ok) {
        delete copy;
    }
}

struct Pipeline {
    std::mutex mtx;
    NetServer server;
    VideoDecoder *decoder = nullptr;
    OHNativeWindow *window = nullptr;
    bool windowReady = false;
    bool gotHello = false;
    bool clientConnected = false;
    bool touchEnabled = false;
    bool touchFrames = false;
    std::array<std::optional<int32_t>, TouchFrame{}.size()> touchIds{};
    TouchFrame lastTouch{};
    int32_t streamW = 0;
    int32_t streamH = 0;

    // 统计
    uint64_t framesIn = 0;
    uint64_t rxBytes = 0;
    uint64_t drops = 0;
    uint64_t errors = 0;
};

Pipeline g_pipeline;
std::thread g_statsThread;
std::atomic<bool> g_statsRunning{false};

bool ParseHello(const uint8_t *data, size_t len, int32_t *width, int32_t *height, bool *touchEnabled, bool *touchFrames)
{
    std::string text(reinterpret_cast<const char *>(data), len);
    if (text.find("hello") == std::string::npos) {
        return false;
    }
    auto findNum = [&text](const char *key, int32_t *out) -> bool {
        size_t pos = text.find(key);
        if (pos == std::string::npos) {
            return false;
        }
        pos = text.find(':', pos);
        if (pos == std::string::npos) {
            return false;
        }
        long v = atol(text.c_str() + pos + 1);
        if (v <= 0 || v > 8192) {
            return false;
        }
        *out = static_cast<int32_t>(v);
        return true;
    };
    int32_t touchVersion = 0;
    *touchEnabled = findNum("\"touch\"", &touchVersion) && touchVersion == 1;
    *touchFrames = *touchEnabled && findNum("\"touchFrames\"", &touchVersion) && touchVersion == 1;
    return findNum("\"width\"", width) && findNum("\"height\"", height);
}

void StartDecoderLocked()
{
    Pipeline &p = g_pipeline; // 调用方持有 p.mtx
    if (p.decoder != nullptr || !p.windowReady || p.streamW <= 0 || p.streamH <= 0) {
        return;
    }
    auto *dec = new VideoDecoder();
    if (dec->Start(p.streamW, p.streamH, p.window)) {
        p.decoder = dec;
    } else {
        p.errors++;
        delete dec;
    }
}

void StopDecoderLocked()
{
    Pipeline &p = g_pipeline; // 调用方持有 p.mtx
    if (p.decoder != nullptr) {
        p.decoder->Stop();
        delete p.decoder;
        p.decoder = nullptr;
    }
}

void OnPacket(const uint8_t *data, size_t len)
{
    Pipeline &p = g_pipeline;
    std::lock_guard<std::mutex> lock(p.mtx);
    if (!p.gotHello) {
        int32_t w = 0;
        int32_t h = 0;
        if (ParseHello(data, len, &w, &h, &p.touchEnabled, &p.touchFrames)) {
            p.streamW = w;
            p.streamH = h;
            p.gotHello = true;
            StartDecoderLocked();
            LOG("hello: %{public}dx%{public}d touch=%{public}d frames=%{public}d", w, h, p.touchEnabled, p.touchFrames);
        }
        return; // 非 hello 包在握手前一律忽略
    }
    p.framesIn++;
    p.rxBytes += len + 4;
    if (p.decoder != nullptr) {
        p.decoder->Feed(data, len);
    } else {
        p.drops++;
    }
}

void OnClientState(bool connected)
{
    Pipeline &p = g_pipeline;
    std::lock_guard<std::mutex> lock(p.mtx);
    p.clientConnected = connected;
    LOG("client %{public}s", connected ? "connected" : "disconnected");
    if (!connected) {
        p.touchIds = {};
        p.lastTouch = {};
        p.touchEnabled = false;
        p.touchFrames = false;
        StopDecoderLocked();
        p.gotHello = false; // 新连接重新握手
    }
}

void OnSurfaceCreated(void *window)
{
    Pipeline &p = g_pipeline;
    std::lock_guard<std::mutex> lock(p.mtx);
    p.window = static_cast<OHNativeWindow *>(window);
    p.windowReady = true;
    LOG("surface created");
    if (p.gotHello) {
        StartDecoderLocked(); // 退后台后回前台，重建解码器，下一关键帧自动恢复
    }
}

bool SendTouchLocked(Pipeline &p, const TouchFrame &frame)
{
    if (p.touchFrames) {
        const auto packet = EncodeTouchFrame(frame);
        return p.server.SendTouch(packet.data(), packet.size());
    }
    uint8_t packet[TOUCH_SAMPLE_BYTES];
    EncodeTouchSample(frame[0], packet);
    return p.server.SendTouch(packet, sizeof(packet));
}

void CancelTouchLocked(Pipeline &p)
{
    TouchFrame frame{};
    bool active = false;
    for (size_t slot = 0; slot < frame.size(); ++slot) {
        if (!p.touchIds[slot]) continue;
        frame[slot].action = TouchAction::Cancel;
        active = true;
    }
    if (active) SendTouchLocked(p, frame);
    p.touchIds = {};
    p.lastTouch = {};
}

void OnSurfaceDestroyed()
{
    Pipeline &p = g_pipeline;
    std::lock_guard<std::mutex> lock(p.mtx);
    LOG("surface destroyed");
    CancelTouchLocked(p);
    StopDecoderLocked();
    p.window = nullptr;
    p.windowReady = false;
}

void StatsLoop()
{
    using clock = std::chrono::steady_clock;
    auto lastTs = clock::now();
    uint64_t lastOut = 0;
    uint64_t lastRx = 0;
    while (g_statsRunning.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(STATS_INTERVAL_MS));
        if (!g_statsRunning.load()) {
            break;
        }
        auto now = clock::now();
        double sec = std::chrono::duration<double>(now - lastTs).count();
        if (sec <= 0) {
            continue;
        }
        lastTs = now;

        StatusData s;
        uint64_t curOut = 0;
        {
            Pipeline &p = g_pipeline;
            std::lock_guard<std::mutex> lock(p.mtx);
            s.width = p.streamW;
            s.height = p.streamH;
            s.frames = p.framesIn;
            s.drops = p.drops;
            s.errors = p.errors;
            if (p.decoder != nullptr) {
                curOut = p.decoder->framesDecoded.load();
                uint32_t err = p.decoder->lastError.load();
                if (err != 0) {
                    s.errors = p.errors + 1;
                }
                s.status = "streaming";
            } else {
                s.status = p.clientConnected ? "connected" : "listening";
            }
        }
        s.fps = static_cast<double>(curOut - lastOut) / sec;
        lastOut = curOut;

        Pipeline &p = g_pipeline;
        double kbps = 0.0;
        {
            std::lock_guard<std::mutex> lock(p.mtx);
            kbps = static_cast<double>(p.rxBytes - lastRx) * 8 / sec / 1000.0;
            lastRx = p.rxBytes;
        }
        s.kbps = kbps;
        PushStatus(s);
    }
}

napi_value Start(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2] = {nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    int32_t port = DEFAULT_PORT;
    if (argc >= 1) {
        napi_get_value_int32(env, args[0], &port);
    }
    if (argc >= 2 && g_statusFn == nullptr) {
        napi_value resourceName = nullptr;
        napi_create_string_utf8(env, "subscreenStatus", NAPI_AUTO_LENGTH, &resourceName);
        napi_create_threadsafe_function(env, args[1], nullptr, resourceName, 2, 1, nullptr, nullptr, nullptr,
                                        CallStatusJs, &g_statusFn);
    }

    {
        Pipeline &p = g_pipeline;
        std::lock_guard<std::mutex> lock(p.mtx);
        p.framesIn = 0;
        p.rxBytes = 0;
        p.drops = 0;
        p.errors = 0;
    }
    bool ok = g_pipeline.server.Start(static_cast<uint16_t>(port), OnPacket, OnClientState);
    if (ok && !g_statsRunning.exchange(true)) {
        g_statsThread = std::thread(StatsLoop);
    }
    LOG("server start %{public}s on port %{public}d", ok ? "ok" : "failed", port);

    napi_value result = nullptr;
    napi_get_boolean(env, ok, &result);
    return result;
}

napi_value Stop(napi_env env, napi_callback_info info)
{
    g_statsRunning.store(false);
    if (g_statsThread.joinable()) {
        g_statsThread.join();
    }
    g_pipeline.server.Stop();
    {
        Pipeline &p = g_pipeline;
        std::lock_guard<std::mutex> lock(p.mtx);
        StopDecoderLocked();
        p.gotHello = false;
        p.clientConnected = false;
    }
    if (g_statusFn != nullptr) {
        napi_release_threadsafe_function(g_statusFn, napi_tsfn_release);
        g_statusFn = nullptr;
    }
    LOG("server stopped");
    return nullptr;
}

// ---- XComponent Surface 生命周期 ----

void OnSurfaceCreatedCb(OH_NativeXComponent *component, void *window)
{
    OnSurfaceCreated(window);
}

void OnSurfaceChangedCb(OH_NativeXComponent *component, void *window) {}

void OnSurfaceDestroyedCb(OH_NativeXComponent *component, void *window)
{
    OnSurfaceDestroyed();
}

void DispatchTouchCb(OH_NativeXComponent *component, void *window)
{
    OH_NativeXComponent_TouchEvent event{};
    uint64_t width = 0, height = 0;
    if (OH_NativeXComponent_GetTouchEvent(component, window, &event) != 0 ||
        OH_NativeXComponent_GetXComponentSize(component, window, &width, &height) != 0 ||
        width == 0 || height == 0) return;

    auto &p = g_pipeline;
    std::lock_guard<std::mutex> lock(p.mtx);
    if (!p.touchEnabled || !p.gotHello || !p.decoder || !p.decoder->framesDecoded.load()) return;
    const double scale = std::min(static_cast<double>(width) / p.streamW, static_cast<double>(height) / p.streamH);
    const double contentW = p.streamW * scale, contentH = p.streamH * scale;
    if (event.type == OH_NATIVEXCOMPONENT_CANCEL) {
        CancelTouchLocked(p);
        return;
    }
    const auto normalize = [&](float x, float y, TouchSample &sample) {
        const double nx = (x - (width - contentW) / 2) / contentW;
        const double ny = (y - (height - contentH) / 2) / contentH;
        if (!std::isfinite(nx) || !std::isfinite(ny)) return false;
        sample.x = static_cast<uint16_t>(std::clamp(nx, 0.0, 1.0) * 65535);
        sample.y = static_cast<uint16_t>(std::clamp(ny, 0.0, 1.0) * 65535);
        return nx >= 0 && nx < 1 && ny >= 0 && ny < 1;
    };
    const auto end = p.touchIds.begin() + (p.touchFrames ? p.touchIds.size() : 1);
    auto changed = std::find(p.touchIds.begin(), end, std::optional<int32_t>(event.id));
    TouchSample down{TouchAction::Down};
    if (event.type == OH_NATIVEXCOMPONENT_DOWN) {
        if (changed != end || (!p.touchFrames && event.numPoints != 1) || !normalize(event.x, event.y, down)) return;
        changed = std::find(p.touchIds.begin(), end, std::nullopt);
        if (changed == end) return;
        *changed = event.id;
    } else if (event.type == OH_NATIVEXCOMPONENT_UP) {
        if (changed == end) return;
    } else if (event.type != OH_NATIVEXCOMPONENT_MOVE || (!p.touchFrames && changed == end)) {
        return;
    }

    TouchFrame frame{};
    bool active = false;
    for (auto id = p.touchIds.begin(); id != end; ++id) {
        if (!*id) continue;
        const size_t slot = id - p.touchIds.begin();
        auto &sample = frame[slot];
        active = true;
        if (id == changed && event.type == OH_NATIVEXCOMPONENT_DOWN) {
            sample = down;
        } else if (id == changed && event.type == OH_NATIVEXCOMPONENT_UP) {
            sample.action = TouchAction::Up;
            id->reset();
        } else {
            sample = p.lastTouch[slot];
            sample.action = TouchAction::Move;
            if (id == changed) {
                normalize(event.x, event.y, sample);
            } else {
                for (size_t i = 0; i < std::min<size_t>(event.numPoints, std::size(event.touchPoints)); ++i) {
                    const auto &point = event.touchPoints[i];
                    if (point.id == **id && point.isPressed) {
                        normalize(point.x, point.y, sample);
                        break;
                    }
                }
            }
        }
    }
    if (!active) return;
    if (SendTouchLocked(p, frame)) {
        p.lastTouch = frame;
    } else {
        p.touchIds = {};
        p.lastTouch = {};
    }
}

napi_value Init(napi_env env, napi_value exports)
{
    // XComponent(libraryname) 加载本库时，exports 携带 OH_NATIVE_XCOMPONENT_OBJ
    napi_value exportInstance = nullptr;
    OH_NativeXComponent *nativeXComponent = nullptr;
    if (napi_get_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ, &exportInstance) == napi_ok &&
        napi_unwrap(env, exportInstance, reinterpret_cast<void **>(&nativeXComponent)) == napi_ok &&
        nativeXComponent != nullptr) {
        static OH_NativeXComponent_Callback surfaceCb = {OnSurfaceCreatedCb, OnSurfaceChangedCb,
                                                         OnSurfaceDestroyedCb, DispatchTouchCb};
        OH_NativeXComponent_RegisterCallback(nativeXComponent, &surfaceCb);
    }

    napi_property_descriptor desc[] = {
        {"start", nullptr, Start, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"stop", nullptr, Stop, nullptr, nullptr, nullptr, napi_default, nullptr},
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    return exports;
}

} // namespace

static napi_module subscreenModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "subscreen",
    .nm_priv = nullptr,
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterSubscreenModule(void)
{
    napi_module_register(&subscreenModule);
}
