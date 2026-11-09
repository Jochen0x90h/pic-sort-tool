# pic-sort-tool: GUI tool for sorting pictures
add_executable(pic-sort-tool
    glad/glad.c
    glad/glad.h
    glad/khrplatform.h
    imgui/imgui_impl_opengl3.cpp
    imgui/imgui_impl_opengl3.h
    imgui/Fonts.hpp
    GuiWindow.cpp
    GuiWindow.hpp
    Media.hpp
    pic-sort-tool.cpp
    Picture.cpp
    Picture.hpp
    #TinyEXIF.cpp
    #TinyEXIF.h
    Video.cpp
    Video.hpp
)
target_link_libraries(pic-sort-tool
    PRIVATE
        glfw
        imgui::imgui
        JPEG::JPEG
        #libjpeg-turbo::turbojpeg
        libjpeg-turbo::turbojpeg-static
        #tinyxml2::tinyxml2
        ffmpeg::ffmpeg
        exiv2::exiv2
)
if(WIN32)
    target_link_libraries(pic-sort-tool
        PRIVATE
            ws2_32 secur32 bcrypt ncrypt crypt32
    )
endif()

# pic-date-tool: Command line tool for fixing the file date from exif data
add_executable(pic-date-tool
    pic-date-tool.cpp
    #TinyEXIF.cpp
    #TinyEXIF.h
)
target_link_libraries(pic-date-tool
    PRIVATE
        #tinyxml2::tinyxml2
        exiv2::exiv2
)


# install
install(TARGETS pic-sort-tool pic-date-tool)
