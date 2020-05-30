function(AddFFmpegDependence)
    include_directories(${CMAKE_SOURCE_DIR}/thirdpart/ffmpeg/include)
    link_directories(${CMAKE_SOURCE_DIR}/thirdpart/ffmpeg/${CMAKE_SYSTEM_NAME}/lib)
    set(FFmpegLibs z m -pthread iconv lzma bz2
    "-framework AudioToolbox" 
    "-framework VideoToolbox" 
    "-framework CoreFoundation" 
    "-framework CoreMedia" 
    "-framework CoreVideo" 
    "-framework CoreAudio" 
    "-framework CoreServices"
    "-framework CoreGraphics"
    "-framework CoreImage"
    "-framework OpenGL"
    "-framework AppKit"
    "-framework Security"
    "-framework CoreServices"
    avformat 
    avcodec
    avdevice
    avfilter
    avutil
    swscale
    swresample PARENT_SCOPE)
endfunction()

function(AddSDL2Dependence)
    include_directories(${CMAKE_SOURCE_DIR}/thirdpart/sdl2/include)
    link_directories(${CMAKE_SOURCE_DIR}/thirdpart/sdl2/${CMAKE_SYSTEM_NAME}/lib)
    set(SDL2Libs m iconv objc
        "-framework CoreAudio" 
        "-framework AudioToolbox" 
        "-framework ForceFeedback" 
        "-framework CoreVideo" 
        "-framework Cocoa"
        "-framework Carbon" 
        "-framework IOKit" 
        "-weak_framework QuartzCore" 
        "-weak_framework Metal" SDL2 PARENT_SCOPE)
endfunction()

function(BaseFFmpegTest testName dir)
    project(${testName})

    set(Verbose ON)
    set(CMAKE_CXX_STANDARD 14)
    message("Dir = " ${CMAKE_SYSTEM_NAME})
    add_definitions(-DWORKPATH="${CMAKE_SOURCE_DIR}")
    AddFFmpegDependence()
    include_directories(${dir})
    aux_source_directory(${dir} SRC_MAIN)
    # set(FFmpegDir ${CMAKE_SOURCE_DIR}/thirdpart/ffmpeg/${CMAKE_SYSTEM_NAME}/lib)
    # message("FFmpeg Dir = " ${FFmpegDir})
    message("FFMpegLibs :" ${FFmpegLibs})
    add_executable(${testName} ${SRC_MAIN})
    target_link_libraries(${testName} ${FFmpegLibs})
endfunction()

function(BaseSDL2Test testName dir)
    project(${testName})

    set(Verbose ON)
    set(CMAKE_CXX_STANDARD 14)
    message("Dir = " ${CMAKE_SYSTEM_NAME})
    add_definitions(-DWORKPATH="${CMAKE_SOURCE_DIR}")
    AddSDL2Dependence()
    include_directories(${dir})
    aux_source_directory(${dir} SRC_MAIN)
    add_executable(${testName} ${SRC_MAIN})
    target_link_libraries(${testName} ${SDL2Libs})
endfunction()

function(BasePlay testName dir)
    project(${testName})

    set(Verbose ON)
    set(CMAKE_CXX_STANDARD 14)
    message("Dir = " ${CMAKE_SYSTEM_NAME})
    add_definitions(-DWORKPATH="${CMAKE_SOURCE_DIR}")
    AddSDL2Dependence()
    AddFFmpegDependence()
    include_directories(${dir})
    aux_source_directory(${dir} SRC_MAIN)
    add_executable(${testName} ${SRC_MAIN})
    target_link_libraries(${testName} ${SDL2Libs} ${FFmpegLibs})
endfunction()

