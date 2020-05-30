cmake_minimum_required(VERSION 3.4.1)
include(test.cmake)

BaseFFmpegTest(x01Decode ${CMAKE_CURRENT_SOURCE_DIR}/decode)
BaseFFmpegTest(x02Encode ${CMAKE_CURRENT_SOURCE_DIR}/encode)
BaseSDL2Test(x03SDL2 ${CMAKE_CURRENT_SOURCE_DIR}/sdl2)
BasePlay(x04PlayVideo ${CMAKE_CURRENT_SOURCE_DIR}/playvideo)
BasePlay(x05PlayAudio ${CMAKE_CURRENT_SOURCE_DIR}/playaudio)