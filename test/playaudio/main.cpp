#include <iostream>
#include <SDL2/SDL.h>
#include <sstream>
#include <streambuf>
#include <istream>
#include <ostream>
#include <queue>
#include <cstring>
#include <mutex>
#include <condition_variable>

#ifndef WORKPATH
#define WORKPATH "../res/"
#endif

extern "C"{
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
}


class Sample{
public:
    char * data{nullptr};
    Sample(int size){
        data = new char[size];
    }

    ~Sample(){
        delete[] data;
    }
};

class AudioRenderer {
private:
    AVFormatContext* formatCtx{nullptr};
    AVCodecContext* codecCtx{nullptr};
    AVCodecParameters* codecPar{nullptr};
    AVFrame* frame{nullptr};
    AVPacket* packet{nullptr};
    int audioIndex{-1};
    int bytesPerSample{-1};
    std::queue<Sample*> tempData;
    std::mutex m;
    std::condition_variable cv;
public:
    void open(std::string path) {
        formatCtx = avformat_alloc_context();
        if (avformat_open_input(&formatCtx, path.c_str(), nullptr, nullptr) != 0 ) {
            std::cout << "open file " << path << "failed" << std::endl;
            return;
        }
        if(avformat_find_stream_info(formatCtx, nullptr)<0){
            std::cout << "get stream info failed !" << std::endl;
            return;
        }
        for (int i = 0; i < formatCtx->nb_streams; i++) {
            if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                std::cout << "stream num = " << i << " is audio stream" << std::endl;
                audioIndex = i;
                break;
            }
        }
        if (audioIndex == -1) {
            std::cout << "cannot find audio stream from " << path << std::endl;
            return;
        }
        codecPar = formatCtx->streams[audioIndex]->codecpar;
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
        bytesPerSample = av_get_bytes_per_sample(codecCtx->sample_fmt);
       
        packet = av_packet_alloc();
        av_init_packet(packet);
        frame = av_frame_alloc();
        std::cout << "open file successed" << std::endl;
    }

    int getChannelCount(){
        return codecPar->channels;
    }

    int getSampleRate() {
        return codecPar->sample_rate;
    }

    int getFrameSize(){
        return codecPar->frame_size;
    }

    int getFormat() {
        return codecCtx->sample_fmt;
    }

    static void audioFillCallback(void* userdata, Uint8 *stream, int len){
        auto renderer = reinterpret_cast<AudioRenderer*>(userdata);
        std::unique_lock<std::mutex> lck(renderer->m);
        if (!renderer->tempData.empty()){
            auto s = renderer->tempData.front();
            SDL_memcpy(stream, s->data, len);
            renderer->tempData.pop();
            delete s;
            renderer->cv.notify_all();
        }
    }

    bool decodeAudioFrame(){
        auto readRet = av_read_frame(formatCtx, packet);
        if (readRet == 0) {
            int sendRet = 0;
            if (packet->stream_index == audioIndex) {
                sendRet = avcodec_send_packet(codecCtx, packet);
                std::cout << "send data to codec : size = " << packet->size 
                    << ", index = " << packet->stream_index 
                    << ", pts = " << packet->pts 
                    << ", sendRet = " << av_err2str(sendRet) << std::endl;
            }
        } else {
            std::cout << "av_read_frame readRet = " << av_err2str(readRet) << std::endl;
            packet->data = nullptr;
            packet->size = 0;
            // 填充空帧，刷出后面的帧
            avcodec_send_packet(codecCtx, packet);
        }
        auto receiveRet = 0;
        while(receiveRet >= 0) {
            receiveRet = avcodec_receive_frame(codecCtx, frame);
            if (receiveRet == 0) {
                auto sample = new Sample(codecPar->frame_size * codecPar->channels * bytesPerSample);
                int offset = 0;
                for (int i = 0; i < codecPar->frame_size; i++) {
                    for (int j=0;j< codecPar->channels;j++){
                        offset = i * codecPar->channels * bytesPerSample + j * bytesPerSample;
                        memcpy(&sample->data[offset], (char *)(frame->data[j]+i*bytesPerSample), bytesPerSample);
                    }
                }
                std::unique_lock<std::mutex> lck(m);
                if (tempData.size() > 3) {
                    cv.wait(lck);
                }
                tempData.push(sample);
                std::cout << "decode frame : " << frame->pts << std::endl;
            } else if (receiveRet == AVERROR_EOF ) {
                std::cout << "avcodec_receive_frame : receiveRet = " << av_err2str(receiveRet) << ", readRet = " << av_err2str(readRet) << std::endl;
                return false;
            }
        }
        return true;
    }

    void close() {
        av_frame_free(&frame);
        av_packet_free(&packet);

        avcodec_close(codecCtx);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        std::cout << "decode audio finished!" << std::endl;

        std::unique_lock<std::mutex> lck(m);
        while(!tempData.empty()) {
            auto s = tempData.front();
            tempData.pop();
            delete s;
        }
    }
};


int main() { 
    std::stringstream srcPath;
    srcPath << WORKPATH << "/res/video1.mp4";
    AudioRenderer audioRenderer;
    audioRenderer.open(srcPath.str());

    SDL_Init(SDL_INIT_AUDIO);
    SDL_AudioSpec spec;
    spec.channels = audioRenderer.getChannelCount();
    spec.freq = audioRenderer.getSampleRate();
    spec.samples = audioRenderer.getFrameSize();
    switch (audioRenderer.getFormat()) {
        case AV_SAMPLE_FMT_FLT:
        case AV_SAMPLE_FMT_FLTP:
            spec.format = AUDIO_F32;
            break;
        case AV_SAMPLE_FMT_S16:
        case AV_SAMPLE_FMT_S16P:
            spec.format = AUDIO_S16;
            break;
        case AV_SAMPLE_FMT_U8:
        case AV_SAMPLE_FMT_U8P:
            spec.format = AUDIO_U8;
            break;
        default:
            break;
    }
    spec.callback = &AudioRenderer::audioFillCallback;
    spec.userdata = &audioRenderer;

    if(SDL_OpenAudio(&spec, nullptr) == 0) {
        SDL_PauseAudio(0);
    }
    
    while(audioRenderer.decodeAudioFrame()) {
    }

    audioRenderer.close();
    std::cout << "audio renderer closed" << std::endl;
    return 0;
}