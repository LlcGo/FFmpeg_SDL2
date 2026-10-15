//// main.cpp : FFmpeg(6.x) + SDL2 简易视频播放器（全部写在 main 里）
//// 功能：选择视频文件 / 播放暂停 / 左右键快退快进 / 鼠标拖动底部进度条跳转
//// 同步：以音频时钟为准，视频跟着音频走；没有音频流时改用系统墙钟
////
//// 快捷键： 空格 = 播放/暂停     ← / → = 快退/快进 10 秒     O = 打开其它文件     Esc = 退出
//// 鼠标：   在底部进度条上按下并拖动，松开即跳转到该位置
//
//#define NOMINMAX
//#define WIN32_LEAN_AND_MEAN
//
//// 本文件是 UTF-8(带 BOM) 保存的，这里让 MSVC 把窄字符串字面量也按 UTF-8 生成。
//// 不写这句的话字面量会被转成 GBK，SDL 的窗口标题接口要的是 UTF-8，中文就会变乱码。
//#pragma execution_character_set("utf-8")
//
//#include <SDL.h>
//
//#include <windows.h>
//#include <commdlg.h>
//#pragma comment(lib, "comdlg32.lib")   // GetOpenFileNameW 所在库，直接链接省得改工程
//
//extern "C" {
//#include <libavformat/avformat.h>
//#include <libavcodec/avcodec.h>
//#include <libavutil/avutil.h>
//#include <libavutil/imgutils.h>
//#include <libavutil/time.h>
//#include <libswscale/swscale.h>
//#include <libswresample/swresample.h>
//}
//
//#include <cstdio>
//#include <cstring>
//#include <cmath>
//#include <cstdlib>
//#include <string>
//#include <vector>
//#include <deque>
//#include <atomic>
//#include <algorithm>
//
//// ---------------------------------------------------------------------------
//// 常量
//// ---------------------------------------------------------------------------
//static const int    BAR_H      = 44;    // 底部进度条高度（像素）
//static const int    MAX_VQ     = 8;     // 视频帧队列上限：控制内存占用和解码节奏
//static const double SEEK_STEP  = 10.0;  // 左右键一次快进/快退多少秒
//static const double DROP_SLOP  = 0.10;  // 落后主时钟超过这么多秒的帧直接丢掉，追进度用
//
//// ---------------------------------------------------------------------------
//// 全局状态（单文件播放器，直接用全局变量，省得到处传参）
//// ---------------------------------------------------------------------------
//// ---- 解封装 / 解码 ----
//static AVFormatContext* g_fmt     = nullptr;
//static int              g_vstream = -1;   // 视频流索引
//static int              g_astream = -1;   // 音频流索引
//static AVCodecContext*  g_vctx    = nullptr;
//static AVCodecContext*  g_actx    = nullptr;
//static SwsContext*      g_sws     = nullptr;
//static SwrContext*      g_swr     = nullptr;
//
//static int    g_width    = 0;      // 视频宽（解码尺寸）
//static int    g_height   = 0;      // 视频高
//static double g_duration = 0.0;    // 总时长（秒），未知为 0
//static std::string g_file;         // 当前文件路径（UTF-8）
//
//// ---- 播放控制 ----
//static bool   g_eof      = false;  // 文件是否已读到结尾
//static bool   g_flushed  = false;  // 解码器是否已送入空包做收尾
//static double g_skipUntil = -1.0;  // 跳转后丢弃 pts 早于该值的帧（秒）
//
//// ---- 墙钟（无音频时的主时钟） ----
//static double  g_wallBasePts   = 0.0;
//static Uint32  g_wallBaseTicks = 0;
//
//// ---- 视频帧队列 ----
//struct VideoFrame {
//    AVFrame* frame;   // 已解码的原始帧（YUV 等，未转码）
//    double   pts;     // 显示时间戳（秒）
//};
//static std::deque<VideoFrame> g_vq;
//
//// ---- SDL 窗口 / 渲染 ----
//static SDL_Window*   g_win = nullptr;
//static SDL_Renderer* g_ren = nullptr;
//static SDL_Texture*  g_tex = nullptr;   // 与视频同分辨率的流式纹理（BGRA）
//static std::vector<uint8_t> g_rgb;      // sws 转换后的 BGRA 像素缓冲
//
//// ---- 音频：环形缓冲 + 时钟 ----
//static SDL_AudioDeviceID g_adev     = 0;
//static int               g_outRate  = 44100;  // 实际输出采样率（取声卡给的值）
//static int               g_frameBytes = 4;    // 一个采样帧的字节数（2 声道 * 2 字节）
//static std::vector<uint8_t> g_aring;          // 环形缓冲
//static size_t            g_arCap = 0;         // 容量
//static size_t            g_arRead = 0;        // 读位置（声卡线程）
//static size_t            g_arWrite = 0;       // 写位置（主线程）
//static std::atomic<bool>        g_clockValid{false};  // 音频时钟是否可用
//static std::atomic<long long>   g_playedFrames{0};    // 声卡已消耗的采样帧总数（单调递增）
//static long long                g_basePlayed = 0;     // 上次锚定时对应的采样帧数
//static double                   g_audioBasePts = 0.0;// 上次锚定时对应的 pts（秒）
//
//// ---------------------------------------------------------------------------
//// 小工具
//// ---------------------------------------------------------------------------
//// 宽字符转 UTF-8（Windows 文件对话框给的是宽字符，FFmpeg 收 UTF-8）
//static std::string wideToUtf8(const wchar_t* w)
//{
//    if (!w) return std::string();
//    int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
//    if (n <= 0) return std::string();
//    std::string s((size_t)n - 1, '\0');
//    WideCharToMultiByte(CP_UTF8, 0, w, -1, &s[0], n, nullptr, nullptr);
//    return s;
//}
//
//// 取文件名（去掉目录）
//static std::string baseName(const std::string& path)
//{
//    size_t p = path.find_last_of("\\/");
//    return (p == std::string::npos) ? path : path.substr(p + 1);
//}
//
//// 秒 -> mm:ss 或 hh:mm:ss
//static std::string fmtTime(double sec)
//{
//    if (sec < 0 || !(sec == sec)) sec = 0;      // 负数 / NaN 一律当 0
//    long total = (long)(sec + 0.5);
//    long h = total / 3600, m = (total % 3600) / 60, s = total % 60;
//    char buf[32];
//    if (h > 0) snprintf(buf, sizeof(buf), "%ld:%02ld:%02ld", h, m, s);
//    else       snprintf(buf, sizeof(buf), "%02ld:%02ld", m, s);
//    return std::string(buf);
//}
//
//// 弹出 Windows 打开文件对话框，返回 UTF-8 路径；取消返回空串
//static std::string pickFile()
//{
//    wchar_t buf[MAX_PATH] = { 0 };
//    OPENFILENAMEW ofn;
//    ZeroMemory(&ofn, sizeof(ofn));
//    ofn.lStructSize = sizeof(ofn);
//    ofn.hwndOwner   = nullptr;
//    // 过滤器每段是"说明\0通配\0"，整串以两个 \0 结束
//    static const wchar_t filter[] =
//        L"视频文件\0*.mp4;*.mkv;*.avi;*.flv;*.mov;*.wmv;*.ts;*.m4v;*.mpg;*.rmvb\0"
//        L"所有文件\0*.*\0";
//    ofn.lpstrFilter = filter;
//    ofn.lpstrFile   = buf;
//    ofn.nMaxFile    = MAX_PATH;
//    ofn.lpstrTitle  = L"选择要播放的视频";
//    ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
//    if (!GetOpenFileNameW(&ofn)) return std::string();
//    return wideToUtf8(buf);
//}
//
//// ---------------------------------------------------------------------------
//// 环形缓冲（读/写在声卡线程，必须用 SDL_LockAudioDevice 串行化）
//// ---------------------------------------------------------------------------
//static size_t ringAvail() { return (g_arWrite + g_arCap - g_arRead) % g_arCap; }
//static size_t ringFree()  { return g_arCap - 1 - ringAvail(); }   // 留 1 字节区分满/空
//
//static size_t ringFreeLocked()
//{
//    if (!g_adev) return 0;
//    SDL_LockAudioDevice(g_adev);
//    size_t f = ringFree();
//    SDL_UnlockAudioDevice(g_adev);
//    return f;
//}
//
//static size_t ringAvailLocked()
//{
//    if (!g_adev) return 0;
//    SDL_LockAudioDevice(g_adev);
//    size_t a = ringAvail();
//    SDL_UnlockAudioDevice(g_adev);
//    return a;
//}
//
//// 写入环形缓冲（调用前必须已锁声卡）
//static void ringWrite(const uint8_t* p, size_t n)
//{
//    size_t f = ringFree();
//    if (n > f) n = f;   // 放不下就丢弃多余部分，简单播放器不做背压
//    for (size_t i = 0; i < n; i++) {
//        g_aring[g_arWrite] = p[i];
//        g_arWrite = (g_arWrite + 1) % g_arCap;
//    }
//}
//
//// 声卡回调：把环形缓冲里的数据喂给声卡，不够补静音
//static void SDLCALL audioCallback(void* /*userdata*/, Uint8* stream, int len)
//{
//    int dst = 0;
//    while (dst < len) {
//        if (g_arRead == g_arWrite) break;   // 缓冲空
//        // 一次尽量拷贝连续的一段（最多到缓冲末端或本次剩余长度）
//        size_t contig = g_arCap - g_arRead;
//        if (g_arWrite > g_arRead) contig = (size_t)(g_arWrite - g_arRead);
//        if (contig > (size_t)(len - dst)) contig = (size_t)(len - dst);
//        memcpy(stream + dst, &g_aring[g_arRead], contig);
//        g_arRead = (g_arRead + contig) % g_arCap;
//        dst += (int)contig;
//    }
//    if (dst < len) memset(stream + dst, 0, (size_t)(len - dst));  // 补静音，别播噪声
//
//    g_playedFrames += dst / g_frameBytes;   // 统计已消耗的采样帧，供音频时钟使用
//    if (g_arRead == g_arWrite) g_clockValid = false;  // 缓冲被读空 -> 下次重新锚定时钟
//}
//
//// 音频时钟：锚点 pts + 已播放采样帧数 / 采样率
//static double audioClock()
//{
//    if (!g_clockValid) return -1.0;
//    return g_audioBasePts + (double)(g_playedFrames.load() - g_basePlayed) / (double)g_outRate;
//}
//
//// 主时钟：有音频且音频时钟有效就用音频，否则退回系统墙钟
//static double masterClock()
//{
//    double a = audioClock();
//    if (a >= 0.0) return a;
//    return g_wallBasePts + (double)(SDL_GetTicks() - g_wallBaseTicks) / 1000.0;
//}
//
//// 把墙钟当前值冻住/重新锚定：暂停、恢复、播放结束都调它。
//// 不冻的话，音频放完（主时钟回退到墙钟）后标题栏的时间会一直往上累加
//static void freezeWallClock()
//{
//    g_wallBasePts   = masterClock();
//    g_wallBaseTicks = SDL_GetTicks();
//}
//
//// ---------------------------------------------------------------------------
//// 视频帧队列
//// ---------------------------------------------------------------------------
//static void clearVideoQueue()
//{
//    for (auto& vf : g_vq) av_frame_free(&vf.frame);
//    g_vq.clear();
//}
//
//static void freeFrontFrame()
//{
//    if (g_vq.empty()) return;
//    av_frame_free(&g_vq.front().frame);
//    g_vq.pop_front();
//}
//
//// 取一帧的显示时间戳（秒）
//static double framePtsSeconds(AVFrame* f, int streamIdx)
//{
//    int64_t ts = f->best_effort_timestamp;
//    if (ts == AV_NOPTS_VALUE) ts = f->pts;
//    if (ts == AV_NOPTS_VALUE) return -1.0;
//    return (double)ts * av_q2d(g_fmt->streams[streamIdx]->time_base);
//}
//
//// ---------------------------------------------------------------------------
//// 解码：把解码器里现成的帧取出来，视频进队列，音频进环形缓冲
//// ---------------------------------------------------------------------------
//static void drainVideo()
//{
//    if (!g_vctx) return;
//    // 一次把解码器里现成的帧全部取走：不能因为队列上限就留着不取，
//    // 否则解码器输入队列会堵住，send_packet 返回 EAGAIN，包就丢了
//    while (true) {
//        AVFrame* f = av_frame_alloc();
//        if (avcodec_receive_frame(g_vctx, f) < 0) { av_frame_free(&f); break; }
//
//        double pts = framePtsSeconds(f, g_vstream);
//        // 跳转后丢弃早于目标点的帧（seek 会落到前面的关键帧）
//        if (g_skipUntil >= 0.0 && pts >= 0.0 && pts < g_skipUntil) { av_frame_free(&f); continue; }
//        if (g_skipUntil >= 0.0 && pts >= g_skipUntil) g_skipUntil = -1.0;  // 追上了，结束丢弃
//
//        VideoFrame vf;
//        vf.frame = f;     // 所有权交给队列
//        vf.pts   = (pts >= 0.0) ? pts : (g_vq.empty() ? 0.0 : g_vq.back().pts + 0.04);
//        g_vq.push_back(vf);
//    }
//}
//
//static void drainAudio()
//{
//    if (!g_actx || !g_swr || !g_adev) return;
//
//    AVFrame* f = av_frame_alloc();
//    while (true) {
//        if (avcodec_receive_frame(g_actx, f) < 0) break;
//
//        double pts = framePtsSeconds(f, g_astream);
//        // 跳转后丢弃早于目标点的音频帧
//        bool skip = (g_skipUntil >= 0.0 && pts >= 0.0 && pts < g_skipUntil);
//
//        if (!skip) {
//            if (pts < 0.0) pts = g_audioBasePts;   // 极少见：拿不到时间戳就沿用当前锚点，别把声音丢了
//            // swr 重采样/转格式 -> S16 双声道 -> 写环形缓冲
//            int outSamples = swr_get_out_samples(g_swr, f->nb_samples);
//            if (outSamples <= 0) outSamples = f->nb_samples + 256;
//
//            static std::vector<uint8_t> tmp;      // 复用，减少频繁分配
//            tmp.resize((size_t)outSamples * g_frameBytes);
//            uint8_t* outPlane[1] = { tmp.data() };
//            int got = swr_convert(g_swr, outPlane, outSamples,
//                                  (const uint8_t**)f->extended_data, f->nb_samples);
//            if (got > 0) {
//                SDL_LockAudioDevice(g_adev);
//                // 缓冲刚被读空后重新开始喂 -> 用这帧的 pts 重新锚定时钟
//                if (!g_clockValid) {
//                    g_audioBasePts = pts;
//                    g_basePlayed   = g_playedFrames.load();
//                    g_clockValid   = true;
//                }
//                ringWrite(tmp.data(), (size_t)got * g_frameBytes);
//                SDL_UnlockAudioDevice(g_adev);
//            }
//        }
//        av_frame_unref(f);
//
//        // 环形缓冲快满了就先停手，剩下的帧留在解码器里下次再取
//        if (ringFreeLocked() < (size_t)(g_outRate * g_frameBytes / 4)) break;
//    }
//    av_frame_free(&f);
//}
//
//// 送包给解码器：解码器输入队列满(EAGAIN)时先把输出取走再重试，
//// 绝对不能直接丢弃，否则 H.264 参考帧缺失会报 "reference picture missing"
//static void sendPacketSafe(AVCodecContext* ctx, AVPacket* pkt, bool isVideo)
//{
//    if (!ctx) return;
//    int r = avcodec_send_packet(ctx, pkt);
//    if (r == AVERROR(EAGAIN)) {
//        if (isVideo) drainVideo(); else drainAudio();   // 腾空输入队列
//        r = avcodec_send_packet(ctx, pkt);              // 重试同一个包
//    }
//    if (r < 0 && r != AVERROR(EAGAIN) && r != AVERROR_EOF) {
//        static int warned = 0;
//        if (warned++ < 5) printf("[警告] avcodec_send_packet 失败: %d (%s)\n", r, isVideo ? "视频" : "音频");
//    }
//}
//
//// 读包 -> 送解码器 -> 取帧；队列满/缓冲满就少读，避免内存失控
//static void pumpDecoder()
//{
//    if (!g_eof) {
//        for (int i = 0; i < 16; i++) {
//            if ((int)g_vq.size() >= MAX_VQ) break;                       // 视频队列满，先不读了
//            if (g_adev && ringFreeLocked() < (size_t)(g_outRate * g_frameBytes / 4)) break; // 音频缓冲快满
//
//            AVPacket* pkt = av_packet_alloc();
//            int r = av_read_frame(g_fmt, pkt);
//            if (r < 0) { g_eof = true; av_packet_free(&pkt); break; }    // 到结尾或出错，都按结尾处理
//
//            // 每送一个包就立刻把解码器的输出取走，让它不积压输入队列
//            if (pkt->stream_index == g_vstream && g_vctx) {
//                sendPacketSafe(g_vctx, pkt, true);
//                drainVideo();
//            } else if (g_actx && pkt->stream_index == g_astream) {
//                sendPacketSafe(g_actx, pkt, false);
//                drainAudio();
//            }
//            av_packet_unref(pkt);
//            av_packet_free(&pkt);
//        }
//
//        // 读到结尾后送空包，让解码器把内部缓存的帧吐干净
//        if (g_eof && !g_flushed) {
//            if (g_vctx) avcodec_send_packet(g_vctx, nullptr);
//            if (g_actx) avcodec_send_packet(g_actx, nullptr);
//            g_flushed = true;
//        }
//    }
//
//    drainVideo();
//    drainAudio();
//}
//
//// ---------------------------------------------------------------------------
//// 跳转
//// ---------------------------------------------------------------------------
//static void doSeek(double sec)
//{
//    if (!g_fmt) return;
//
//    double dur = (g_duration > 0.0) ? g_duration : 1e9;
//    if (sec < 0.0) sec = 0.0;
//    if (sec > dur) sec = dur - 0.05;
//    if (sec < 0.0) sec = 0.0;
//
//    int64_t ts = (int64_t)(sec * AV_TIME_BASE);           // stream_index = -1 时时间基是 AV_TIME_BASE
//    av_seek_frame(g_fmt, -1, ts, AVSEEK_FLAG_BACKWARD);   // 往前找最近的关键帧
//
//    if (g_vctx) avcodec_flush_buffers(g_vctx);
//    if (g_actx) avcodec_flush_buffers(g_actx);
//
//    clearVideoQueue();
//
//    if (g_adev) {
//        SDL_LockAudioDevice(g_adev);
//        g_arRead = g_arWrite = 0;
//        g_clockValid = false;
//        SDL_UnlockAudioDevice(g_adev);
//    }
//
//    g_eof     = false;
//    g_flushed = false;
//    g_skipUntil = sec;                       // 丢弃目标点之前的帧
//    g_wallBasePts   = sec;                   // 墙钟也跟着跳到目标点
//    g_wallBaseTicks = SDL_GetTicks();
//}
//
//// ---------------------------------------------------------------------------
//// 渲染
//// ---------------------------------------------------------------------------
//static void renderVideoFrame(AVFrame* f)
//{
//    if (!g_sws || !g_tex) return;
//    uint8_t* dst[4]        = { g_rgb.data(), nullptr, nullptr, nullptr };
//    int      dstLinesize[4] = { g_width * 4, 0, 0, 0 };
//    sws_scale(g_sws, f->data, f->linesize, 0, g_height, dst, dstLinesize);
//    SDL_UpdateTexture(g_tex, nullptr, g_rgb.data(), g_width * 4);
//}
//
//// 把鼠标 x 位置换算成时间（0..时长）
//static double xToTime(int x, int winW)
//{
//    double trackW = (double)(winW - 20);
//    if (trackW <= 0) return 0.0;
//    double t = (double)(x - 10) / trackW;
//    if (t < 0.0) t = 0.0;
//    if (t > 1.0) t = 1.0;
//    return t * (g_duration > 0 ? g_duration : 0.0);
//}
//
//// 画一帧画面 + 进度条，并刷新窗口标题
//static void presentFrame(bool paused, bool dragging, double dragTime)
//{
//    // 渲染限速：画面+进度条最多约 60fps，别每秒上千次空刷窗口
//    static Uint32 lastPresent = 0;
//    Uint32 nowMs = SDL_GetTicks();
//    if (nowMs - lastPresent < 15) return;
//    lastPresent = nowMs;
//
//    int winW = 0, winH = 0;
//    SDL_GetWindowSize(g_win, &winW, &winH);
//    int areaH = winH - BAR_H; if (areaH < 1) areaH = 1;
//
//    // 背景
//    SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 255);
//    SDL_RenderClear(g_ren);
//
//    // 画面按比例居中缩放
//    if (g_tex && g_width > 0 && g_height > 0) {
//        double vidAR  = (double)g_width / (double)g_height;
//        double areaAR = (double)winW / (double)areaH;
//        SDL_Rect dst;
//        if (areaAR > vidAR) { dst.h = areaH; dst.w = (int)(areaH * vidAR); }
//        else                { dst.w = winW;  dst.h = (int)(winW / vidAR); }
//        dst.x = (winW - dst.w) / 2;
//        dst.y = (areaH - dst.h) / 2;
//        SDL_RenderCopy(g_ren, g_tex, nullptr, &dst);
//    }
//
//    // ---- 进度条 ----
//    SDL_Rect barBg = { 0, areaH, winW, BAR_H };
//    SDL_SetRenderDrawColor(g_ren, 30, 30, 30, 255);
//    SDL_RenderFillRect(g_ren, &barBg);
//
//    double dur = g_duration;
//    double cur = dragging ? dragTime : masterClock();
//    if (cur < 0) cur = 0;
//    if (dur > 0.0 && cur > dur) cur = dur;   // 显示时间夹在总时长以内，不做无意义的累加
//    double ratio = (dur > 0.0) ? cur / dur : 0.0;
//    if (ratio < 0.0) ratio = 0.0;
//    if (ratio > 1.0) ratio = 1.0;
//
//    int trackW = winW - 20;
//    int trackY = areaH + BAR_H / 2 - 3;
//    SDL_Rect track = { 10, trackY, trackW, 6 };
//    SDL_SetRenderDrawColor(g_ren, 80, 80, 80, 255);
//    SDL_RenderFillRect(g_ren, &track);
//
//    SDL_Rect played = { track.x, track.y, (int)(trackW * ratio), 6 };
//    SDL_SetRenderDrawColor(g_ren, 30, 159, 255, 255);   // 蓝色进度
//    SDL_RenderFillRect(g_ren, &played);
//
//    int knobX = track.x + (int)(trackW * ratio);
//    SDL_Rect knob = { knobX - 6, areaH + BAR_H / 2 - 8, 12, 16 };
//    if (dragging) SDL_SetRenderDrawColor(g_ren, 255, 200, 60, 255);
//    else          SDL_SetRenderDrawColor(g_ren, 230, 230, 230, 255);
//    SDL_RenderFillRect(g_ren, &knob);
//
//    SDL_RenderPresent(g_ren);
//
//    // ---- 标题栏显示时间和快捷键（省掉字体渲染依赖）----
//    static Uint32 lastTitle = 0;
//    Uint32 now = SDL_GetTicks();
//    if (now - lastTitle >= 200) {
//        lastTitle = now;
//        char buf[600];
//        snprintf(buf, sizeof(buf),
//                 "%s  -  %s / %s%s    [空格]暂停  [←/→]±10s  [O]打开  [拖进度条]跳转",
//                 baseName(g_file).c_str(),
//                 fmtTime(dragging ? dragTime : masterClock()).c_str(),
//                 fmtTime(g_duration).c_str(),
//                 paused ? "  ‖已暂停" : "");
//        SDL_SetWindowTitle(g_win, buf);
//    }
//}
//
//// ---------------------------------------------------------------------------
//// 打开 / 关闭媒体
//// ---------------------------------------------------------------------------
//static void closeMedia()
//{
//    clearVideoQueue();
//
//    if (g_adev) { SDL_CloseAudioDevice(g_adev); g_adev = 0; }
//    if (g_swr)  { swr_free(&g_swr); }
//    if (g_sws)  { sws_freeContext(g_sws); g_sws = nullptr; }
//    if (g_vctx) { avcodec_free_context(&g_vctx); }
//    if (g_actx) { avcodec_free_context(&g_actx); }
//    if (g_fmt)  { avformat_close_input(&g_fmt); }
//
//    g_aring.clear();
//    g_arCap = g_arRead = g_arWrite = 0;
//    g_rgb.clear();
//    g_vstream = g_astream = -1;
//    g_width = g_height = 0;
//    g_duration = 0.0;
//    g_eof = false;
//    g_flushed = false;
//    g_skipUntil = -1.0;
//    g_clockValid = false;
//    g_playedFrames = 0;
//    g_basePlayed = 0;
//    g_audioBasePts = 0.0;
//}
//
//static bool openMedia(const std::string& path)
//{
//    // 1) 解封装
//    if (avformat_open_input(&g_fmt, path.c_str(), nullptr, nullptr) < 0) {
//        printf("[错误] 打不开文件: %s\n", path.c_str());
//        return false;
//    }
//    if (avformat_find_stream_info(g_fmt, nullptr) < 0) {
//        printf("[错误] 读不到流信息\n");
//        return false;
//    }
//
//    g_file = path;
//
//    // 2) 视频
//    g_vstream = av_find_best_stream(g_fmt, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
//    if (g_vstream < 0) {
//        printf("[错误] 没有视频流\n");
//        return false;
//    }
//    const AVCodec* vdec = avcodec_find_decoder(g_fmt->streams[g_vstream]->codecpar->codec_id);
//    if (!vdec) { printf("[错误] 找不到视频解码器\n"); return false; }
//
//    g_vctx = avcodec_alloc_context3(vdec);
//    if (avcodec_parameters_to_context(g_vctx, g_fmt->streams[g_vstream]->codecpar) < 0 ||
//        avcodec_open2(g_vctx, vdec, nullptr) < 0) {
//        printf("[错误] 视频解码器打开失败\n");
//        return false;
//    }
//    g_vctx->thread_count = 0;   // 0 = 自动按 CPU 核数开多线程
//
//    g_width  = g_vctx->width;
//    g_height = g_vctx->height;
//
//    // 3) 缩放到 BGRA，方便直接喂 SDL 纹理
//    g_sws = sws_getContext(g_width, g_height, g_vctx->pix_fmt,
//                           g_width, g_height, AV_PIX_FMT_BGRA,
//                           SWS_BILINEAR, nullptr, nullptr, nullptr);
//    if (!g_sws) { printf("[错误] sws 初始化失败\n"); return false; }
//    g_rgb.resize((size_t)g_width * g_height * 4);
//
//    // 4) 音频（可选）
//    g_astream = av_find_best_stream(g_fmt, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
//    if (g_astream >= 0) {
//        const AVCodec* adec = avcodec_find_decoder(g_fmt->streams[g_astream]->codecpar->codec_id);
//        if (adec) {
//            g_actx = avcodec_alloc_context3(adec);
//            if (avcodec_parameters_to_context(g_actx, g_fmt->streams[g_astream]->codecpar) < 0 ||
//                avcodec_open2(g_actx, adec, nullptr) < 0) {
//                avcodec_free_context(&g_actx);
//            }
//        }
//    }
//
//    // 5) 打开声卡（用源采样率，尽量让 swr 少做事）
//    if (g_actx) {
//        SDL_AudioSpec want, have;
//        SDL_zero(want);
//        want.freq     = g_actx->sample_rate > 0 ? g_actx->sample_rate : 44100;
//        want.format   = AUDIO_S16SYS;
//        want.channels = 2;
//        want.samples  = 1024;
//        want.callback = audioCallback;
//        g_adev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
//        if (g_adev == 0) {
//            printf("[警告] 声卡打开失败，改为纯视频播放: %s\n", SDL_GetError());
//        } else {
//            g_outRate    = have.freq;
//            g_frameBytes = have.channels * 2;      // S16 = 2 字节
//            g_arCap      = (size_t)g_outRate * g_frameBytes * 2;   // 2 秒缓冲
//            g_aring.assign(g_arCap, 0);
//
//            // 重采样 -> S16 / 双声道 / 声卡采样率
//            AVChannelLayout inLayout;
//            memset(&inLayout, 0, sizeof(inLayout));
//            if (g_actx->ch_layout.nb_channels > 0) inLayout = g_actx->ch_layout;
//            else av_channel_layout_default(&inLayout, 2);
//            AVChannelLayout outLayout = AV_CHANNEL_LAYOUT_STEREO;
//            swr_alloc_set_opts2(&g_swr, &outLayout, AV_SAMPLE_FMT_S16, g_outRate,
//                                &inLayout, g_actx->sample_fmt, g_actx->sample_rate, 0, nullptr);
//            if (!g_swr || swr_init(g_swr) < 0) {
//                printf("[警告] 重采样器初始化失败，改为纯视频播放\n");
//                if (g_swr) swr_free(&g_swr);
//                SDL_CloseAudioDevice(g_adev);
//                g_adev = 0;
//            }
//        }
//    }
//
//    // 6) 总时长
//    if (g_fmt->duration > 0) {
//        g_duration = (double)g_fmt->duration / (double)AV_TIME_BASE;
//    } else {
//        AVRational tb = g_fmt->streams[g_vstream]->time_base;
//        int64_t d = g_fmt->streams[g_vstream]->duration;
//        if (d > 0) g_duration = (double)d * av_q2d(tb);
//    }
//
//    printf("[信息] 打开成功: %s\n", baseName(path).c_str());
//    printf("       分辨率 %dx%d, 时长 %s, 音频 %s\n",
//           g_width, g_height, fmtTime(g_duration).c_str(),
//           g_adev ? "有" : "无");
//    av_dump_format(g_fmt, 0, baseName(path).c_str(), 0);
//    return true;
//}
//
//// ---------------------------------------------------------------------------
//// 窗口 / 渲染器
//// ---------------------------------------------------------------------------
//static void createRenderer()
//{
//    int winW = g_width, winH = g_height + BAR_H;
//    // 太大的视频压到不超过 1280 宽，避免窗口超出屏幕
//    if (winW > 1280) {
//        winH = (int)(1280.0 * g_height / g_width) + BAR_H;
//        winW = 1280;
//    }
//    if (winW < 320) winW = 320;
//    if (winH < 200) winH = 200;
//
//    g_win = SDL_CreateWindow("FFmpeg + SDL2 播放器",
//                             SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
//                             winW, winH, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
//    g_ren = SDL_CreateRenderer(g_win, -1, SDL_RENDERER_ACCELERATED);
//    g_tex = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_ARGB8888,
//                              SDL_TEXTUREACCESS_STREAMING, g_width, g_height);
//}
//
//static void destroyRenderer()
//{
//    if (g_tex) { SDL_DestroyTexture(g_tex); g_tex = nullptr; }
//    if (g_ren) { SDL_DestroyRenderer(g_ren); g_ren = nullptr; }
//    if (g_win) { SDL_DestroyWindow(g_win); g_win = nullptr; }
//}
//
//// ---------------------------------------------------------------------------
//// 播放主循环：返回 0 = 退出程序，1 = 换一个文件
//// ---------------------------------------------------------------------------
//static int playbackLoop()
//{
//    if (g_adev) SDL_PauseAudioDevice(g_adev, 0);   // 开始出声
//
//    g_wallBasePts   = 0.0;
//    g_wallBaseTicks = SDL_GetTicks();
//
//    bool   paused    = false;
//    bool   dragging  = false;
//    double dragTime  = 0.0;
//    bool   quit = false, openOther = false;
//    SDL_Event e;
//
//    while (!quit && !openOther) {
//        // ---- 事件 ----
//        while (SDL_PollEvent(&e)) {
//            if (e.type == SDL_QUIT) { quit = true; }
//            else if (e.type == SDL_KEYDOWN) {
//                SDL_Keycode k = e.key.keysym.sym;
//                if (k == SDLK_ESCAPE)      quit = true;
//                else if (k == SDLK_SPACE) {
//                    paused = !paused;
//                    if (g_adev) SDL_PauseAudioDevice(g_adev, paused ? 1 : 0);
//                    freezeWallClock();   // 暂停时冻住、恢复时重新锚定，暂停期间时间不涨
//                }
//                else if (k == SDLK_o) { openOther = true; }
//                else if (k == SDLK_LEFT)  doSeek(masterClock() - SEEK_STEP);
//                else if (k == SDLK_RIGHT) doSeek(masterClock() + SEEK_STEP);
//            }
//            else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
//                int winW, winH;
//                SDL_GetWindowSize(g_win, &winW, &winH);
//                if (e.button.y >= winH - BAR_H) {          // 落在进度条上
//                    dragging = true;
//                    dragTime = xToTime(e.button.x, winW);
//                }
//            }
//            else if (e.type == SDL_MOUSEMOTION && dragging) {
//                int winW, winH;
//                SDL_GetWindowSize(g_win, &winW, &winH);
//                dragTime = xToTime(e.motion.x, winW);
//            }
//            else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT && dragging) {
//                dragging = false;
//                doSeek(dragTime);                          // 松手才真正跳转
//            }
//        }
//        if (openOther || quit) break;
//
//        // ---- 解码 + 显示 ----
//        if (!paused && !dragging) {
//            pumpDecoder();
//
//            double clock = masterClock();
//
//            // 落后太多的帧直接丢，避免越播越晚
//            while ((int)g_vq.size() > 1 && g_vq.front().pts >= 0.0 &&
//                   g_vq.front().pts < clock - DROP_SLOP) {
//                freeFrontFrame();
//            }
//
//            if (!g_vq.empty() && g_vq.front().pts <= clock + 0.002) {
//                renderVideoFrame(g_vq.front().frame);
//                freeFrontFrame();
//            }
//
//            // 播放结束：文件读完、视频队列空、音频也放完 -> 停在最后一帧
//            if (g_eof && g_vq.empty() && (!g_adev || ringAvailLocked() == 0)) {
//                paused = true;
//                if (g_adev) SDL_PauseAudioDevice(g_adev, 1);
//                freezeWallClock();   // 结束后把时间冻住，标题栏不再往上累加
//            }
//        }
//
//        presentFrame(paused, dragging, dragTime);
//        SDL_Delay(1);
//    }
//
//    return quit ? 0 : 1;
//}
//
//// ---------------------------------------------------------------------------
//int main(int argc, char* argv[])
//{
//    // 0) 控制台按 UTF-8 输出，配合上面的 execution_character_set，中文才不会乱码
//    SetConsoleOutputCP(CP_UTF8);
//
//    // 1) 初始化 SDL（视频 + 音频）
//    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
//        printf("[错误] SDL_Init 失败: %s\n", SDL_GetError());
//        return 1;
//    }
//
//    // 2) 选文件：命令行带了就用命令行的，否则弹对话框
//    std::string path = (argc > 1) ? std::string(argv[1]) : pickFile();
//
//    while (!path.empty()) {
//        if (!openMedia(path)) {          // 打开失败
//            path = pickFile();           // 让用户重新选
//            continue;
//        }
//        createRenderer();
//
//        int rc = playbackLoop();         // 0 = 退出, 1 = 换文件
//
//        destroyRenderer();
//        closeMedia();
//
//        if (rc == 0) break;              // 用户要求退出
//        path = pickFile();               // 用户按了 O
//    }
//
//    SDL_Quit();
//    return 0;
//}
