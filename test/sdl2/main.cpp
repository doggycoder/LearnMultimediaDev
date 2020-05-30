#include <iostream>
#include <SDL2/SDL.h>
#include <chrono>

using namespace std::chrono;

long long getTimeMs() {
    system_clock::duration d = system_clock::now().time_since_epoch();
    auto mic = duration_cast<milliseconds>(d);
    return mic.count();
}

void renderStep(SDL_Renderer* renderer,SDL_Texture* texture) {
    SDL_Rect rect;
    rect.x = 100;
    rect.y = 100;
    rect.w = 100;
    rect.h = 100;
    SDL_SetRenderTarget(renderer, texture);
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); 
    SDL_RenderClear(renderer);//用指定的颜色清空缓冲区
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); //长方形为白色
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderTarget(renderer, nullptr);
}

int main(){
    std::cout << "hello world !" << std::endl;
    SDL_Init(SDL_INIT_VIDEO); 
    auto window = SDL_CreateWindow("My First Window",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              640,
                              480,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE); 
    if (!window) {
        return -1;
    }
    //基于窗口创建渲染器
    auto renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        return -1;
    }
    auto texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 640, 480);

    SDL_Event event;
    while(true) {
        auto startTimeMs = getTimeMs();
        int ret = SDL_PollEvent(&event);
        if (event.type == SDL_QUIT) {
            break;
        }
        renderStep(renderer, texture);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); 
        SDL_RenderClear(renderer);//用指定的颜色清空缓冲区
        SDL_RenderCopy(renderer, texture, NULL, NULL); 
        SDL_RenderPresent(renderer); //将缓冲区中的内容输出到目标窗口上。
        auto stopTimeMs = getTimeMs();
        std::cout << "start : " << startTimeMs << ", stop:" << stopTimeMs << ", durationMs: " << (stopTimeMs - startTimeMs) << std::endl;
        SDL_Delay(std::max<long long>(33 - (stopTimeMs - startTimeMs), 0)); 
    }
    SDL_DestroyRenderer(renderer); //销毁渲染器
    SDL_DestroyWindow(window); //销毁窗口
    SDL_Quit();
    return 0;
}