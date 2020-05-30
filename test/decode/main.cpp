#include <iostream>
#include <fstream>
#include <sstream>

#ifndef WORKPATH
#define WORKPATH "../res/"
#endif

extern "C"{
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
}

void decode_audio() {
    std::string srcPath;
    srcPath += WORKPATH;
    srcPath += "/res/video1.mp4";
    auto formatCtx = avformat_alloc_context();
    if (avformat_open_input(&formatCtx, srcPath.c_str(), nullptr, nullptr) != 0 ) {
        std::cout << "open file " << srcPath << "failed" << std::endl;
        return;
    }
    if(avformat_find_stream_info(formatCtx, nullptr)<0){
        std::cout << "get stream info failed !" << std::endl;
        return;
    }
    int audioIndex = -1;
    for (int i = 0; i < formatCtx->nb_streams; i++) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            std::cout << "stream num = " << i << " is audio stream" << std::endl;
            audioIndex = i;
            break;
        }
    }
    if (audioIndex == -1) {
        std::cout << "cannot find audio stream from " << srcPath << std::endl;
        return;
    }
    auto codecPar = formatCtx->streams[audioIndex]->codecpar;
    auto decoder = avcodec_find_decoder(codecPar->codec_id);
    if (decoder == nullptr) {
        std::cout << "cannot find decoder" << std::endl;
    }
    auto codecCtx = avcodec_alloc_context3(decoder);
    //参数拷贝到Context, 不然解码因为缺失信息会失败
    avcodec_parameters_to_context(codecCtx, codecPar);
    auto ret = avcodec_open2(codecCtx, decoder, nullptr);
    if (ret != 0) {
        std::cout << "open codec failed = " << ret << std::endl;
        return;
    }

    auto packet = av_packet_alloc();
    av_init_packet(packet);
    auto frame = av_frame_alloc();

    int breakFlag = 0;
    bool loopFlag = true;
    auto bytesPerSample = av_get_bytes_per_sample(codecCtx->sample_fmt);
    auto channelCount = codecPar->channels;
    auto frameSize = codecPar->frame_size;
    std::stringstream ss;
    ss << WORKPATH << "/output/decode_data"<< codecPar->sample_rate << "_" << codecPar->channels <<".pcm";
    std::ofstream outFile(ss.str());
    while(loopFlag) {
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
            packet->data = nullptr;
            packet->size = 0;
            // 填充空帧，刷出后面的帧
            avcodec_send_packet(codecCtx, packet);
        }
        auto receiveRet = 0;
        while(receiveRet >= 0) {
            receiveRet = avcodec_receive_frame(codecCtx, frame);
            if (receiveRet == 0) {
                for (int i = 0; i < frameSize; i++) {
                    for (int j=0;j< channelCount;j++){
                        outFile.write((char *)(frame->data[j]+i*bytesPerSample),bytesPerSample);
                    }
                }
                std::cout << "decode frame : " << frame->pts << std::endl;
            } else if (receiveRet == AVERROR_EOF ) {
                loopFlag = false;
                std::cout << "avcodec_receive_frame : receiveRet = " << av_err2str(receiveRet) << ", readRet = " << av_err2str(readRet) << std::endl;
            }
        }
    }
    
    outFile.close();

    av_frame_free(&frame);
    av_packet_free(&packet);

    avcodec_close(codecCtx);
    avcodec_free_context(&codecCtx);
    avformat_close_input(&formatCtx);
    std::cout << "decode audio finished!" << std::endl;
}


void decode_video() {
    std::string srcPath;
    srcPath += WORKPATH;
    srcPath += "/res/video1.mp4";
    auto formatCtx = avformat_alloc_context();
    if (avformat_open_input(&formatCtx, srcPath.c_str(), nullptr, nullptr) != 0 ) {
        std::cout << "open file " << srcPath << "failed" << std::endl;
        return;
    }
    if(avformat_find_stream_info(formatCtx, nullptr)<0){
        std::cout << "get stream info failed !" << std::endl;
        return;
    }
    int videoIndex = -1;
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
    auto codecCtx = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(codecCtx, codecPar);
    auto ret = avcodec_open2(codecCtx, decoder, nullptr);
    if (ret != 0) {
        std::cout << "open codec failed = " << ret << std::endl;
        return;
    }

    auto packet = av_packet_alloc();
    av_init_packet(packet);
    auto frame = av_frame_alloc();

    int breakFlag = 0;
    bool loopFlag = true;
    bool isYuvSave = false;
    std::stringstream ss;
    ss << WORKPATH << "/output/decode_data"<< codecPar->width << "_" << codecPar->height << codecCtx->pix_fmt <<".yuv";
    std::ofstream outFile(ss.str());
    while(loopFlag) {
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
                if (frame->pts > 100 && !isYuvSave && codecCtx->pix_fmt == AV_PIX_FMT_YUV420P) {
                    outFile.write((char *)frame->data[0], codecPar->width * codecPar->height);
                    outFile.write((char *)frame->data[1], (codecPar->width * codecPar->height) >> 2);
                    outFile.write((char *)frame->data[2], (codecPar->width * codecPar->height) >> 2);
                    isYuvSave = true;
                }
                std::cout << "decode frame : " << frame->pts << std::endl;
            } else if (receiveRet == AVERROR_EOF ) {
                loopFlag = false;
                std::cout << "avcodec_receive_frame : receiveRet = " << av_err2str(receiveRet) << ", readRet = " << av_err2str(readRet) << std::endl;
            }
        }
    }
    
    outFile.close();

    av_frame_free(&frame);
    av_packet_free(&packet);

    avcodec_close(codecCtx);
    avcodec_free_context(&codecCtx);
    avformat_close_input(&formatCtx);
    std::cout << "decode video finished!" << std::endl;
}

int main() {
    std::cout << "Hello World!" << WORKPATH << std::endl;
    decode_audio();
    decode_video();
    return 0;
}