#include "dst.hpp"
#include "GuiWindow.hpp"
#include "Picture.hpp"
#include "Video.hpp"
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <filesystem>
#include <ranges>
#include <errno.h>
#include <exiv2/exiv2.hpp>


namespace fs = std::filesystem;
using namespace std::chrono_literals;

/*
// https://forum.arduino.cc/t/rtc-mit-sommerzeit/168068
bool summertime_EU(int year, int month, int day, int hour, int tzHours)
// European Daylight Savings Time calculation by "jurs" for German Arduino Forum
// input parameters: "normal time" for year, month, day, hour and tzHours (0=UTC, 1=MEZ)
// return value: returns true during Daylight Saving Time, false otherwise
{
    if (month<3 || month>10) return false; // keine Sommerzeit in Jan, Feb, Nov, Dez
    if (month>3 && month<10) return true; // Sommerzeit in Apr, Mai, Jun, Jul, Aug, Sep
    if (month==3 && (hour + 24 * day)>=(1 + tzHours + 24*(31 - (5 * year /4 + 4) % 7)) || month==10 && (hour + 24 * day)<(1 + tzHours + 24*(31 - (5 * year /4 + 1) % 7)))
        return true;
    else
        return false;
}

/// @brief Calculate if US daylight saving time is in effect.
/// @param day Day of month (1..31)
/// @param month Month (1..12)
/// @param dow Day of week (1 = monday .. 7 = sunday)
/// @return true if daylight saving time is in effect
bool summertime_US(int day, int month, int dow) {
    //January, february, and december are out.
    if (month < 3 || month > 11) { return false; }
    //April to October are in
    if (month > 3 && month < 11) { return true; }
    int previousSunday = day - dow;
    //In march, we are DST if our previous sunday was on or after the 8th.
    if (month == 3) { return previousSunday >= 8; }
    //In november we must be before the first sunday to be dst.
    //That means the previous sunday must be before the 1st.
    return previousSunday <= 0;
}
/// @brief Daylight saving time type
///
enum class DstType {
    NONE,
    EU,
    US
};
*/

/*
const char *subsampName[TJ_NUMSAMP] = {
    "4:4:4", "4:2:2", "4:2:0", "Grayscale", "4:4:0", "4:1:1"
};

const char *colorspaceName[TJ_NUMCS] = {
    "RGB", "YCbCr", "GRAY", "CMYK", "YCCK"
};
*/


namespace shader {

GLuint compile(std::string const &name, GLenum type, std::string const &source) {
    unsigned int shader = glCreateShader(type);
    const char * src = source.data();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    int isCompiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
    if (GL_FALSE == isCompiled) {
        int error = glGetError();
        int length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::string message(length, '\0');
        glGetShaderInfoLog(shader, length, nullptr, &message.front());
        throw std::runtime_error(
            "Failed to compile shader '" + name + "': " + message + " glError:" + std::to_string(error));
    }
    return shader;
}

GLuint create(std::string const &name, std::string const &vertex, std::string const &fragment) {
    // create shaders
    GLuint vs = compile(name + ".vertex", GL_VERTEX_SHADER, vertex);
    GLuint fs = compile(name + ".fragment", GL_FRAGMENT_SHADER, fragment);

    // create program
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program);

    // delete shaders (program still holds a reference to them)
    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

GLint getUniform(std::string const &programName, GLuint program, std::string const &uniformName) {
    GLint location = glGetUniformLocation(program, uniformName.c_str());
    if (location == -1) {
        throw std::runtime_error(
            "Uniform '" + uniformName + "' does not exist in program '" + programName + "'");
    }
    return location;
}

GLint getVertexInput(std::string const &programName, GLuint program, std::string const &inputName) {
    GLint location = glGetAttribLocation(program, inputName.c_str());
    if (location == -1) {
        throw std::runtime_error(
            "Vertex input '" + inputName + "' does not exist in program '" + programName + "'");
    }
    return location;
}

}


struct Vertex {
    float x;
    float y;
    float z;
};

struct Texcoord {
    float u;
    float v;
};


class Image {
public:

    Image() {
        // create shader
        std::string shaderName = "Map";
        shader_ = shader::create(shaderName, vertexShaderCode_, fragmentShaderCode_);
        matUniform_ = shader::getUniform(shaderName, shader_, "mat");
        mapUniform_ = shader::getUniform(shaderName, shader_, "map");
        vertexInput_ = shader::getVertexInput(shaderName, shader_, "vertex");
        texcoordInput_ = shader::getVertexInput(shaderName, shader_, "texcoord");

        // set texture index, not necessary because 0 is default
        //glUseProgram(shader_);
        //glUniform1i(mapUniform_, 0);
        //glUseProgram(0);

        // load texture
        glGenTextures(1, &texture_);
        glBindTexture(GL_TEXTURE_2D, texture_);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        // create vertex buffers
        vertexCount_ = int(std::size(vertices_));
        indexCount_ = int(std::size(indices_));
        glGenBuffers(1, &vertexBuffer_);
        glGenBuffers(1, &texcoordBuffer_);
        glGenBuffers(1, &indexBuffer_);

        // create vertex array object
        glGenVertexArrays(1, &vao_);
        glBindVertexArray(vao_);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
        glBufferData(GL_ARRAY_BUFFER, vertexCount_ * sizeof(Vertex), vertices_, GL_STATIC_DRAW);
        glEnableVertexAttribArray(vertexInput_);
        glVertexAttribPointer(vertexInput_, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

        glBindBuffer(GL_ARRAY_BUFFER, texcoordBuffer_);
        glBufferData(GL_ARRAY_BUFFER, vertexCount_ * sizeof(Texcoord), texcoords_, GL_STATIC_DRAW);
        glEnableVertexAttribArray(texcoordInput_);
        glVertexAttribPointer(texcoordInput_, 2, GL_FLOAT, GL_FALSE, sizeof(Texcoord), nullptr);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount_ * sizeof(uint32_t), indices_, GL_STATIC_DRAW);

        glBindVertexArray(0);
    }

    void set(Size<float> size, ImageData image) {
        float mat[4][4] = {};

        float m00 = 1;
        float m11 = 1;
        if (image.orientation <= 4) {
            // orientation 1-4: width and height are not exchanged
            if (size.width * image.height > size.height * image.width) {
                m00 = float(size.height * image.width) / float(size.width * image.height);
            } else {
                m11 = float(size.width * image.height) / float(size.height * image.width);
            }

            //   1       2       3       4
            // 888888  888888      88  88
            // 88          88      88  88
            // 8888      8888    8888  8888
            // 88          88      88  88
            // 88          88  888888  888888
            switch (image.orientation) {
            case 2:
                mat[0][0] = -m00;
                mat[1][1] = m11;
                break;
            case 3:
                mat[0][0] = -m00;
                mat[1][1] = -m11;
                break;
            case 4:
                mat[0][0] = m00;
                mat[1][1] = -m11;
                break;
            default:
                mat[0][0] = m00;
                mat[1][1] = m11;
            }
        } else {
            // orientation 5-8: width and height are exchanged
            if (size.width * image.width > size.height * image.height) {
                m00 = float(size.height * image.height) / float(size.width * image.width);
            } else {
                m11 = float(size.width * image.width) / float(size.height * image.height);
            }

            //     5           6           7           8
            // 8888888888  88                  88  8888888888
            // 88  88      88  88          88  88      88  88
            // 88          8888888888  8888888888          88
            switch (image.orientation) {
            case 5:
                mat[1][0] = -m00;
                mat[0][1] = -m11;
                break;
            case 6:
                mat[1][0] = m00;
                mat[0][1] = -m11;
                break;
            case 7:
                mat[1][0] = m00;
                mat[0][1] = m11;
                break;
            default:
                mat[1][0] = -m00;
                mat[0][1] = m11;
            }
        }
        mat[2][2] = 1;
        mat[3][3] = 1;

        // set matrix
        glUseProgram(shader_);
        glUniformMatrix4fv(matUniform_, 1, false, mat[0]);
        glUseProgram(0);

        // set texture data
        glBindTexture(GL_TEXTURE_2D, texture_);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0,  GL_RGB8, image.width, image.height, 0, GL_RGB, GL_UNSIGNED_BYTE, image.data);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void draw() {
        // set program
        glUseProgram(shader_);

        glBindTexture(GL_TEXTURE_2D, texture_);

        // set vertex array object
        glBindVertexArray(vao_);

        // draw
        //glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);

        // reset
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
    }

protected:
    GLuint shader_;
    GLint matUniform_;
    GLint mapUniform_;
    GLint vertexInput_;
    GLint texcoordInput_;

    GLuint texture_;

    int vertexCount_;
    int indexCount_;
    GLuint vertexBuffer_;
    GLuint texcoordBuffer_;
    GLuint indexBuffer_;

    GLuint vao_;

    static char const *vertexShaderCode_;
    static char const *fragmentShaderCode_;
    static Vertex const vertices_[4];
    static Texcoord const texcoords_[4];
    static uint32_t const indices_[6];
};

char const *Image::vertexShaderCode_ = R"SHADER(#version 330
uniform mat4 mat;
in vec4 vertex;
in vec2 texcoord;
out vec2 uv;
void main() {
    gl_Position = mat * vertex;
    uv = texcoord;
})SHADER";

char const *Image::fragmentShaderCode_ = R"SHADER(#version 330
uniform sampler2D map;
in vec2 uv;
out vec4 pixel;
void main() {
    pixel = texture(map, uv);
})SHADER";



Vertex const Image::vertices_[4] = {
    {-1, -1, 0},
    { 1, -1, 0},
    {-1,  1, 0},
    { 1,  1, 0}
};

Texcoord const Image::texcoords_[4] = {
    {0, 1},
    {1, 1},
    {0, 0},
    {1, 0}
};

uint32_t const Image::indices_[6] = {
    0, 1, 2,
    3, 2, 1
};


// get sorted file list
std::vector<fs::path> getList(fs::path const &dir) {
    std::vector<fs::path> list;
    //for (auto &entry : boost::make_iterator_range(fs::directory_iterator(dir), {})) {
    for (auto &entry : std::ranges::subrange(fs::directory_iterator(dir), {})) {
        fs::path const &path = entry.path();

        std::error_code ec;
        if (fs::is_directory(path, ec))
            list.push_back(path.filename());
    }
    std::sort(list.begin(), list.end());
    return list;
}

/// @brief Media type
///
enum class MediaType {
    NONE,
    PICTURE,
    VIDEO
};


MediaType getMediaType(fs::path const &path) {
    auto ext = path.extension().string();
    if (ext == ".jpg" || ext == ".jpeg" || ext == ".JPG")
        return MediaType::PICTURE;
    else if (ext == ".mp4")
        return MediaType::VIDEO;
    else
        return MediaType::NONE;
}

// MainWindow

class MainWindow : public GuiWindow {
public:
    MainWindow(int width, int height, char const *title, int timeShift, DstType dstType)
        : GuiWindow(width, height, title), timeShift_(timeShift), dstType_(dstType)
    {
        fs::path dir = ".";

        // get list of directories in initial target directory
        targetDir_ = fs::canonical(dir);
        std::vector<fs::path> targetList = getList(targetDir_);

        // read current directory to get list of imge files
        for (auto &entry : std::ranges::subrange(fs::directory_iterator(fs::path(dir)), {})) {
            fs::path const &path = entry.path();

            // collect pictures and videos
            if (getMediaType(path) != MediaType::NONE)
                files_.push_back(path);
        }
        if (files_.empty()) {
            std::cerr << "No input files";
            return;
        }
        std::sort(files_.begin(), files_.end());


        // create picture from first file in list
        if (!files_.empty()) {
            loadMedia(dir / files_[0]);
        } else {
            // clear input field for new directory
            newDirectoryBuffer_[0] = 0;
        }
    }

    ~MainWindow() override {
        delete media_;
    }

    bool empty() {return files_.empty();}

    bool isVideo() {
        return media_ && media_->getMetaData().frameDuration > 0;
    }

protected:

    void updateUtcTime() {
        auto meta = media_->getMetaData();

        if (meta.localTime) {
            // convert date/time to UTC
            auto time = meta.time - timeShift_ * 1h;

            // detect daylight saving time
            auto systemTime = std::chrono::clock_cast<std::chrono::system_clock>(time);
            auto tt = std::chrono::system_clock::to_time_t(systemTime);
            tm t = *gmtime(&tt); // UTC
            //tm t = *localtime(&tt);
            if (isDst(dstType_, 1900 + t.tm_year, t.tm_mon + 1, t.tm_mday, (t.tm_wday == 0) ? 7 : t.tm_wday)) {
                time -= 1h;
            }
            utcTime_ = time;
        } else {
            utcTime_ = meta.time;
        }
        utcTimeString_ = std::format("{0:%F} {0:%R}", utcTime_);
    }

    void loadMedia(const fs::path &path) {
        // delete old media
        delete media_;
        media_ = nullptr;
        newDirectoryBuffer_[0] = 0;

        // load new media
        try {
            switch (getMediaType(path)) {
            case MediaType::PICTURE:
                media_ = new Picture(path);
                break;
            case MediaType::VIDEO:
                media_ = new Video(path);
                break;
            default:
                return;
            }
        } catch (std::exception &e) {
            // failed to load media file
            std::cerr << "Error: " << e.what() << std::endl;
            return;
        }

        // get meta data
        auto meta = media_->getMetaData();

        // convert date/time
        // https://omegaup.com/docs/cpp/en/cpp/chrono/format.html
        // %F = %Y-%m-%d
        // %R = %H:%M
        // %T = %H:%M:%S
        metaTimeString_ = std::format("{0:%F} {0:%R}", meta.time);
        updateUtcTime();

        // pre-set input field for new directory with date of picture
        strncpy((char *)newDirectoryBuffer_, metaTimeString_.c_str(), 10);
        newDirectoryBuffer_[10] = 0;

        // copy geo location to clipboard
        std::stringstream geo;
        if (meta.latitude != 0.0 && meta.longitude != 0.0)
            geo << meta.latitude << ", " << meta.longitude;
        setClipboard(geo.str());
    }

    bool onKey(ImGuiKey key, int scancode, int action, int modifiers, bool neededByGui) override {
        if (action == GLFW_PRESS) {
            // esc: exit
            if (key == ImGuiKey::ImGuiKey_Escape) {
                setShouldClose(true);
                return true;
            }

            if (!neededByGui && !files_.empty()) {
                // select next/pevious image
                int step = 0;
                int count = int(files_.size());
                if (key == ImGuiKey::ImGuiKey_DownArrow || key == ImGuiKey::ImGuiKey_RightArrow)
                    step = 1;
                else if (key == ImGuiKey::ImGuiKey_UpArrow || key == ImGuiKey::ImGuiKey_LeftArrow)
                    step = -1;
                else if (key == ImGuiKey::ImGuiKey_PageDown)
                    step = 10;
                else if (key == ImGuiKey::ImGuiKey_PageUp)
                    step = -10;
                else if (key == ImGuiKey::ImGuiKey_Home)
                    step = -fileIndex_;
                else if (key == ImGuiKey::ImGuiKey_End)
                    step = count - fileIndex_ - 1;
                if (step != 0) {
                    if (step > count)
                        step = count;
                    else if (step < -count)
                        step = -count;
                    fileIndex_ = (fileIndex_ + count + step) % count;
                    rotate_ = 0;
                    mirror_ = 0;

                    // show next/previous picture
                    loadMedia(files_[fileIndex_]);

                    return true;
                } if (key == ImGuiKey::ImGuiKey_Space /*&& (modifiers & GLFW_MOD_SHIFT) != 0*/) {
                    // space: move image

                    // clear media
                    delete media_;
                    media_ = nullptr;

                    fs::path src = files_[fileIndex_];
                    fs::path dst = targetDir_ / src.filename();

                    // move file
                    fs::rename(src, dst);

                    // apply rotation
                    if (rotate_ != 0 || mirror_ != 0) {
                        if (getMediaType(dst) == MediaType::PICTURE)
                            Picture::rotate(dst, rotate_, mirror_);
                    }

                    // set date
                    fs::last_write_time(dst, utcTime_);

                    // erase from list
                    files_.erase(files_.begin() + fileIndex_);
                    fileIndex_ = std::min(fileIndex_, int(files_.size()) - 1);

                    // show next picture
                    if (!files_.empty())
                        loadMedia(files_[fileIndex_]);

                    return true;
                } else if (key == ImGuiKey::ImGuiKey_R) {
                    // rotate
                    rotate_ = (rotate_ + 1) & 3;
                } else if (key == ImGuiKey::ImGuiKey_M) {
                    // mirror
                    mirror_ ^= 1;
                }
            }
        }
        return false;
    }

    void onDraw(State const &state) override {
        // target directory selector
        {
            std::u8string target = targetDir_.filename().u8string() + u8"###target";
            if (ImGui::Begin((char *)target.c_str(), nullptr, 0)) {
                // input for new directory
                if (ImGui::InputText("New Directory", (char *)newDirectoryBuffer_, std::size(newDirectoryBuffer_),
                    ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    // create and enter new subdirectory
                    fs::path newDirectory = newDirectoryBuffer_;
                    fs::create_directory(targetDir_ / newDirectory);
                    newDirectoryBuffer_[0] = 0;
                    targetDir_ /= newDirectory;
                    targetList_ = getList(targetDir_);
                }

                // list box containing subdirectories
                std::vector<fs::path> newTargetList;
                bool applyTargetList = false;
                int selectedTarget = -1;
                ImGui::PushItemWidth(-1);
                if (ImGui::BeginListBox("##list", ImVec2(-FLT_MIN, -FLT_MIN))) {
                    // parent directory
                    if (ImGui::Selectable("..", false)) {
                        fs::path currentDirectory = targetDir_.filename();

                        // exit to parent directory
                        targetDir_ = targetDir_.parent_path();
                        newTargetList = getList(targetDir_);
                        applyTargetList = true;

                        // get index of the directory that we just exited
                        for (int i = 0; i < newTargetList.size(); ++i) {
                            auto const target = newTargetList[i];
                            if (target == currentDirectory) {
                                selectedTarget = i;
                                break;
                            }
                        }
                    }

                    // subdirectories
                    for (int i = 0; i < targetList_.size(); ++i) {
                        std::u8string path = targetList_[i].u8string();
                        if (ImGui::Selectable((char *)path.c_str(), false)) {
                            // enter subdirectory
                            targetDir_ /= targetList_[i];
                            newTargetList = getList(targetDir_);
                            applyTargetList = true;
                        }

                        // check if we exited a directory and we have to scroll to its location
                        if (i == selectedTarget_) {
                            ImGui::SetScrollHereY();
                            selectedTarget_ = -1;
                        }
                    }
                    ImGui::EndListBox();
                }
                ImGui::PopItemWidth();

                // apply new list of directories in target directory when a directory was selected by the user
                if (applyTargetList)
                    targetList_.swap(newTargetList);
                selectedTarget_ = selectedTarget;
            }
            ImGui::End();
        }

        // clear screen
        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (media_ != nullptr) {
            // get image data (pixel data is owned by media)
            auto imageData = media_->getImageData();

            // image info
            std::string info = metaTimeString_.substr(0, 10) + "###info";
            //std::string info = picture->name + "###info";
            if (ImGui::Begin(info.c_str(), nullptr, 0)) {
                // image size
                std::string size = std::to_string(imageData.width) + " x " + std::to_string(imageData.height);
                ImGui::LabelText("Size", "%s", size.c_str());

                // exists in target directory (by file name)?
                bool exists = fs::exists(targetDir_ / files_[fileIndex_].filename());
                ImGui::LabelText("Exists", "%s", exists ? "true" : "false");

                // ISO date
                ImGui::LabelText("Media Time", "%s", metaTimeString_.c_str());
                ImGui::LabelText("UTC Time", "%s", utcTimeString_.c_str());

                // time shift
                if (ImGui::InputInt("Time Shift (h)", &timeShift_)) {
                    updateUtcTime();
                }
                static const char* dstItems[] = {"None", "EU", "US"};
                if (ImGui::BeginCombo("DST", dstItems[int(dstType_)])) {
                    for (int i = 0; i < std::size(dstItems); ++i) {
                        if (ImGui::Selectable(dstItems[i], dstType_ == DstType(i))) {
                            dstType_ = DstType(i);
                            updateUtcTime();
                        }
                    }
                    ImGui::EndCombo();
                }
            }
            ImGui::End();

            // rotate image
            imageData.orientation = rotateOrientation(imageData.orientation, rotate_, mirror_);

            // render image
            image_.set(state.framebufferSize, imageData);
            image_.draw();
        }

        ImGui::Render();

        drawGui();
    }


    // source images
    std::vector<fs::path> files_;
    int fileIndex_ = 0;

    // picture or video
    Media *media_ = nullptr;
    std::chrono::time_point<std::chrono::file_clock> utcTime_;
    std::string metaTimeString_;
    std::string utcTimeString_;
    int timeShift_;
    DstType dstType_;

    // target directory and list of directories in target directory
    fs::path targetDir_;
    std::vector<fs::path> targetList_;
    int selectedTarget_ = -1;

    // image that is rendered onto the screen
    Image image_;

    // rotation of image (0, 90, 180, 270 degrees)
    int rotate_ = 0;
    int mirror_ = 0;

    char8_t newDirectoryBuffer_[64];
};


/// Usage: pic=sort-tool.exe [timeShift] [dstType]
///   timeShift: Time shift of local time to UTC in hours (e.g., Berlin: 1, Las Vegas: -8)
///   dstType: Daylight saving time type (NONE, EU, US)
/// Example: picsort.exe 1 EU
/// There must be images in the current working directory
int main(int argc, const char **argv) {
    int timeShift = 0;
    DstType dstType = DstType::NONE;
    if (argc > 1)
        timeShift = std::atoi(argv[1]);
    if (argc > 2) {
        auto arg = std::string_view(argv[2]);
        if (arg == "EU")
            dstType = DstType::EU;
        else if (arg == "US")
            dstType = DstType::US;
    }
    MainWindow window(1200, 1000, "PicSorter", timeShift, dstType);

    // main loop
    int frameCount = 0;
    auto start = std::chrono::steady_clock::now();
    int count = 5;
    while (!window.shouldClose()) {
        auto frameStart = std::chrono::steady_clock::now();

        // process events
        if (window.isVideo()) {
            glfwPollEvents();
        } else if (count > 0) {
            glfwPollEvents();
            --count;
        } else {
            glfwWaitEvents();
            count = 5;
        }
        //glfwWaitEventsTimeout(0.03);

        // exit if all files sorted
        if (window.empty())
            break;

        window.draw();

        // show frames per second
        auto now = std::chrono::steady_clock::now();
        ++frameCount;
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
        if (duration.count() > 1000) {
            //std::cout << frameCount * 1000 / duration.count() << "fps" << std::endl;
            frameCount = 0;
            start = std::chrono::steady_clock::now();
        }
    }

    return 0;
}
