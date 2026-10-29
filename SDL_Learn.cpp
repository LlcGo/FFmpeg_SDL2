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

#define winW  1280
#define winH  720
#define FRAME_SIZE (winW * winH * 3 / 2)
int  DelayTime = 20;

/**
* FRAME_SIZE = Y + U + V
           = winW×winH + (winW/2)×(winH/2) + (winW/2)×(winH/2)
           = winW×winH + winW×winH/4 + winW×winH/4
           = winW×winH × (1 + 1/4 + 1/4)
           = winW×winH × 3/2
**/


int main(int argc, char* argv[])
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
		SDL_Log("[error] SDL_Init : %s\n", SDL_GetError());
		 return 1;
	}

	SDL_Window* sdl_window = SDL_CreateWindow("sdl",
		                             SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		                             winW, winH, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (sdl_window == NULL)
	{
		SDL_Log("SDL_CreateWindow ERROR %s", SDL_GetError());
		return -1;
	}

	SDL_Renderer* sdl_render = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_ACCELERATED);
	if (sdl_render == NULL)
	{
		SDL_Log("SDL_CreateRenderer ERROR %s", SDL_GetError());
		return -1;
	}

	// 创建纹理
	SDL_Texture* texture = SDL_CreateTexture(sdl_render, SDL_PIXELFORMAT_IYUV,
		                              SDL_TEXTUREACCESS_STREAMING, 1280, 720);

	// 读取YUV文件
	FILE * fp = fopen("avFrame.yuv", "rb");
	if (!fp)
	{
		SDL_Log("error avFrame.yuv: %s", strerror(errno));
		return -1;
	}

	// 分配一帧的缓冲区
	Uint8* yuv_data = (Uint8*)malloc(FRAME_SIZE);
	if (!yuv_data)
	{
		SDL_Log("error malloc");
		fclose(fp);
		return -1;
	}
   
	int quit = 0;
	SDL_Event event;

	while (!quit)
	{
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
				}
			}
		}
		size_t n = fread(yuv_data, 1, FRAME_SIZE, fp);

		if (n < FRAME_SIZE) {
			// 读到文件末尾，回到开头循环播放
			if (feof(fp)) {
				fseek(fp, 0, SEEK_SET);
				n = fread(yuv_data, 1, FRAME_SIZE, fp);
			}
			if (n < FRAME_SIZE) {
				SDL_Log("not fill");
				break;
			}
		}

		SDL_UpdateYUVTexture(
			texture,
			NULL,                          // 更新整个纹理
			yuv_data,                      // Y 平面指针  4：满的
			1280,                          // Y 平面 pitch
			yuv_data + 1280 * 720,         // U 平面指针（紧跟 Y 之后）
			640,                           // U 平面 pitch（宽度一半） 这个是一个U的内存长度
			yuv_data + 1280 * 720 * 5 / 4, // V 平面指针
			640                            // V 平面 pitch 这个是V的内存长度
		);
		SDL_RenderClear(sdl_render);
		SDL_RenderCopy(sdl_render, texture, NULL, NULL);
		SDL_RenderPresent(sdl_render);
		SDL_Delay(DelayTime);
	}

	free(yuv_data);
	fclose(fp);
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(sdl_render);
	SDL_DestroyWindow(sdl_window);
	SDL_Quit();
}