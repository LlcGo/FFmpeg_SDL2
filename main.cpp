#include <SDL.h>
#include <iostream>

int main(int argc, char* argv[]) {
    // 1. 初始化视频子系统
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // 2. 创建窗口：标题、居中、800x600、显示
    SDL_Window* window = SDL_CreateWindow(
        "SDL2 Window",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        800, 600,
        SDL_WINDOW_SHOWN
    );

    if (window == nullptr) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // 3. 事件循环：保持窗口打开，直到用户关闭
    bool quit = false;
    SDL_Event event;
    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {  // 点击关闭按钮
                quit = true;
            }
        }
        SDL_Delay(10); // 避免 CPU 占满，约 100fps
    }

    // 4. 清理资源（顺序：后创建的先销毁）
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}