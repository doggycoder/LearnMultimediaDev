#include <iostream>
#include <fstream>
#include <sstream>
#include <SDL2/SDL.h>
#include <chrono>
#include <future>
#include <queue>
#include <mutex>
#include <condition_variable>

#ifndef WORKPATH
#define WORKPATH "../res/"
#endif

extern "C"{
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
}

using namespace std::chrono;

long long getTimeMs() {
    system_clock::duration d = system_clock::now().time_since_epoch();
    auto mic = duration_cast<milliseconds>(d);
    return mic.count();
}

class VideoRenderer{
private:
    AVFormatContext* formatCtx{nullptr};
    AVCodecContext* codecCtx{nullptr};
    AVFrame* frame{nullptr};
    AVPacket* packet{nullptr};
    AVRational rational;
    int width{640};
    int height{480};
    int videoIndex{-1};
    bool isDecodeFinished{false};
public:
    VideoRenderer(){
    }

    void open(std::string path){
        std::string srcPath;
        srcPath += WORKPATH;
        srcPath += "/res/video1.mp4";
        formatCtx = avformat_alloc_context();
        if (avformat_open_input(&formatCtx, srcPath.c_str(), nullptr, nullptr) != 0 ) {
            std::cout << "open file " << srcPath << "failed" << std::endl;
            return;
        }
        if(avformat_find_stream_info(formatCtx, nullptr)<0){
            std::cout << "get stream info failed !" << std::endl;
            return;
        }
        for (int i = 0; i < formatCtx->nb_streams; i++) {
            if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                std::cout << "stream num = " << i << " is video stream" << std::endl;
                videoIndex = i;
                break;
            }
        }
        if (videoIndex == -1) {
            std::cout << "cannot find video stream from " << srcPath << std::endl;
            return;
        }
        auto codecPar = formatCtx->streams[videoIndex]->codecpar;
        auto decoder = avcodec_find_decoder(codecPar->codec_id);
        if (decoder == nullptr) {
            std::cout << "cannot find decoder" << std::endl;
        }
        codecCtx = avcodec_alloc_context3(decoder);
        avcodec_parameters_to_context(codecCtx, codecPar);
        auto ret = avcodec_open2(codecCtx, decoder, nullptr);
        if (ret != 0) {
            std::cout << "open codec failed = " << ret << std::endl;
            return;
        }
        width = codecPar->width;
        height = codecPar->height;
        packet = av_packet_alloc();
        av_init_packet(packet);
        frame = av_frame_alloc();
    }

    int64_t decodeFrameToTexture(SDL_Renderer* renderer, SDL_Texture* texture) {
        auto readRet = av_read_frame(formatCtx, packet);
        if (readRet == 0) {
            int sendRet = 0;
            if (packet->stream_index == videoIndex) {
                sendRet = avcodec_send_packet(codecCtx, packet);
                std::cout << "send data to codec : size = " << packet->size 
                    << ", index = " << packet->stream_index 
                    << ", pts = " << packet->pts 
                    << ", pixfmt = " << codecCtx->pix_fmt
                    << ", sendRet = " << av_err2str(sendRet) << std::endl;
            }
        } else {
            packet->data = nullptr;
            packet->size = 0;
            // 填充空帧，刷出后面的帧
            avcodec_send_packet(codecCtx, packet);
        }
        auto receiveRet = 0;
        while(receiveRet >= 0) {
            receiveRet = avcodec_receive_frame(codecCtx, frame);
            if (receiveRet == 0) {
                if (codecCtx->pix_fmt == AV_PIX_FMT_YUV420P) {
                    SDL_UpdateYUVTexture(texture, NULL, frame->data[0], frame->linesize[0], 
                        frame->data[1], frame->linesize[1], 
                        frame->data[2], frame->linesize[2]);
                    return av_rescale_q(frame->pkt_duration, formatCtx->streams[videoIndex]->time_base, {1, AV_TIME_BASE}) / 1000;
                }
                std::cout << "decode frame : " << frame->pts << std::endl;
            } else if (receiveRet == AVERROR_EOF ) {
                isDecodeFinished = true;
                std::cout << "avcodec_receive_frame : receiveRet = " << av_err2str(receiveRet) << ", readRet = " << av_err2str(readRet) << std::endl;
                return 30;
            }
        }
        return decodeFrameToTexture(renderer, texture);
    }

    void close() {
        av_frame_free(&frame);
        av_packet_free(&packet);
        avcodec_close(codecCtx);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
    }

    int getWidth() {
        return width;
    }

    int getHeight() {
        return height;
    }

    ~VideoRenderer(){
    }
};

int main() {
    VideoRenderer videoRenderer;
    std::stringstream srcPath;
    srcPath << WORKPATH << "/res/video1.mp4";
    videoRenderer.open(srcPath.str());

    int width = videoRenderer.getWidth();
    int height = videoRenderer.getHeight();

    float rate = 360.0f / width;
    int windowWidth = 360;
    int windowHeight = (int) (height * rate);


    std::cout << "Hello World!" << WORKPATH << std::endl;
    SDL_Init(SDL_INIT_VIDEO); 
    auto window = SDL_CreateWindow("My First Window",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              windowWidth,
                              windowHeight,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE); 
    if (!window) {
        return -1;
    }
    //基于窗口创建渲染器
    auto renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        return -1;
    }
    auto texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_TARGET, width, height);
    SDL_Event event;
    while(true) {
        auto startTimeMs = getTimeMs();
        int ret = SDL_PollEvent(&event);
        if (event.type == SDL_QUIT) {
            break;
        }
        int time = videoRenderer.decodeFrameToTexture(renderer, texture);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); 
        SDL_RenderClear(renderer);//用指定的颜色清空缓冲区
        SDL_RenderCopy(renderer, texture, NULL, NULL); 
        SDL_RenderPresent(renderer); //将缓冲区中的内容输出到目标窗口上。
        auto stopTimeMs = getTimeMs();
        std::cout << "start : " << startTimeMs << ", stop:" << stopTimeMs << ", durationMs: " << (stopTimeMs - startTimeMs) << ", frame duration:" << time << std::endl;
        SDL_Delay(std::max<long long>(time - (stopTimeMs - startTimeMs), 0)); 
    }
    videoRenderer.close();
    SDL_DestroyRenderer(renderer); //销毁渲染器
    SDL_DestroyWindow(window); //销毁窗口
    SDL_Quit();
    return 0;
}