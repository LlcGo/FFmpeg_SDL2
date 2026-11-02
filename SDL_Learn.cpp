#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>
#include <deque>
#include <atomic>
#include <algorithm>
#include <SDL.h>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

#define winW  1280
#define winH  720
#define FRAME_SIZE (winW * winH * 3 / 2)
int  DelayTime = 20;
bool flag = true;
SDL_Window* sdl_window;
SDL_Renderer* sdl_render;
SDL_Texture* texture;

// queue
std::mutex mtx;
std::queue<AVFrame*> q;
std::condition_variable cv;
bool done = false;

/**
* FRAME_SIZE = Y + U + V
           = winW×winH + (winW/2)×(winH/2) + (winW/2)×(winH/2)
           = winW×winH + winW×winH/4 + winW×winH/4
           = winW×winH × (1 + 1/4 + 1/4)
           = winW×winH × 3/2
**/

/**
* SDL 窗口初始化
**/
int SDL_Init()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
		SDL_Log("[error] SDL_Init : %s\n", SDL_GetError());
		return -1;
	}

	sdl_window = SDL_CreateWindow("sdl",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		winW, winH, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (sdl_window == NULL)
	{
		SDL_Log("SDL_CreateWindow ERROR %s", SDL_GetError());
		return -1;
	}

	sdl_render = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_ACCELERATED);
	if (sdl_render == NULL)
	{
		SDL_Log("SDL_CreateRenderer ERROR %s", SDL_GetError());
		return -1;
	}

	// 创建纹理
	texture = SDL_CreateTexture(sdl_render, SDL_PIXELFORMAT_IYUV,
		SDL_TEXTUREACCESS_STREAMING, 1280, 720);
	return 0;
}

int ansyzVideo()
{
	AVFormatContext* avfmt_context = NULL;
	// 读文件信息
	if (avformat_open_input(&avfmt_context, "C:\\Users\\asus\\Desktop\\test.mp4", NULL, NULL) != 0)
	{
		SDL_Log("avformat_open_input ERROR");
		return -1;
	}

	// 查看视频流信息
	if (avformat_find_stream_info(avfmt_context, NULL) < 0)
	{
		SDL_Log("avformat_find_stream_info error");
		return -1;
	}

	// 发现处理的流,并寻找编码器
	const AVCodec* avCodec =NULL;
	int videoIndex;
	videoIndex = av_find_best_stream(avfmt_context, AVMEDIA_TYPE_VIDEO, -1, -1, &avCodec, 0);
	if (avCodec == NULL)
	{
		SDL_Log("av_find_best_stream error");
		return -1;
	}

	// 寻找编解码器
	//const AVCodec* avCodec = avcodec_find_decoder(avfmt_context->streams[videoIndex]->codecpar->codec_id);

	// 打开编解码器
	AVCodecContext* avctx = avcodec_alloc_context3(avCodec);
	// 拷贝流
	if (avcodec_parameters_to_context(avctx, avfmt_context->streams[videoIndex]->codecpar) < 0)
	{
		SDL_Log("avcodec_parameters_to_context error");
		return -1;
	}
	if (avcodec_open2(avctx, avCodec, NULL) != 0)
	{
		SDL_Log("avcodec_open2 error");
		return -1;
	}

	printf("width:%d, height:%d \n", avctx->width, avctx->height);

	// 分配包和帧
	AVPacket* pkt = av_packet_alloc();
	AVFrame* frame = av_frame_alloc();

	// 读取帧   >= 0 和  == 0 才能正常解析
	while (av_read_frame(avfmt_context, pkt) >= 0)
	{
		if (pkt->stream_index == videoIndex)
		{
			avcodec_send_packet(avctx, pkt);
			while (avcodec_receive_frame(avctx, frame) == 0)
			{
				// 异步线程需要重新分配对象
				AVFrame* newframe = av_frame_alloc();
				av_frame_move_ref(newframe, frame);
				{
					std::lock_guard<std::mutex> lk(mtx);
					q.push(newframe);
				}
				cv.notify_one();
			}
		}
		av_packet_unref(pkt);
	}

	return 0;
}

int render()
{
	int quit = 0;
	SDL_Event event;

	while (!quit)
	{
		printf("11111");
		// 处理事件
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				quit = 1;
			}
			if (event.type == SDL_KEYDOWN) {
				switch (event.key.keysym.sym) {
				case SDLK_LEFT:
					printf("← 左键按下\n");
					DelayTime += 20;
					break;
				case SDLK_RIGHT:
					DelayTime -= 20;
					printf("→ 右键按下\n");
					break;
				case SDLK_SPACE:
					printf("空格：切换暂停\n");
					flag = !flag;          // 需要一个 int paused = 0; 变量
					break;
				}
			}
		}
		if (!flag) { SDL_Delay(DelayTime);continue;}
		
		// 尝试获取帧
		AVFrame* frame = NULL;
		{
			std::unique_lock<std::mutex> lk(mtx);
			if (cv.wait_for(lk, std::chrono::milliseconds(5),
				[] { return !q.empty() || done; })) {
				if (!q.empty()) { frame = q.front(); q.pop(); }
			}
		}
		
		// 渲染
		if (frame)
		{

			printf("recv ");
			SDL_UpdateYUVTexture(
				texture,
				NULL,                          // 更新整个纹理
				frame->data[0],                // Y 平面指针  4：满的
				frame->linesize[0],                          // Y 平面 pitch
				frame->data[1],                // U 平面指针（紧跟 Y 之后）
				frame->linesize[1],                           // U 平面 pitch（宽度一半） 这个是一个U的内存长度
				frame->data[2],				  // V 平面指针
				frame->linesize[2]                           // V 平面 pitch 这个是V的内存长度
			);
		}
			
		SDL_RenderClear(sdl_render);
		SDL_RenderCopy(sdl_render, texture, NULL, NULL);
		SDL_RenderPresent(sdl_render);
		SDL_Delay(DelayTime);
	}

	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(sdl_render);
	SDL_DestroyWindow(sdl_window);
	SDL_Quit();

	return 0;
}

int main(int argc, char* argv[])
{

	// SDL初始化 窗口、渲染器
	if (SDL_Init() == -1)
	{
		SDL_Log("SDL INIT ERROR");
		return -1;
	}

	std::thread t0([]() {
		ansyzVideo();
	});

	//std::thread t1([]() {
		render();
	//});

	t0.join();
	//t1.join();

	return 0;
}

// end