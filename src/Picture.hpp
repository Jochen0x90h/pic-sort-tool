#pragma once

#include "Media.hpp"
#include <turbojpeg.h>
#include <filesystem>


namespace fs = std::filesystem;

class Picture : public Media {
public:
    Picture(const fs::path &path);

    ~Picture() override;

    // Media methods
    ImageData getImageData() override;
    MetaData getMetaData() override;

    static void rotate(const fs::path &path, int rotate, int mirror);

protected:
    void setError(char const *action);
    void setError(char const *action, tjhandle tjInstance);

    const char *action_ = nullptr;
    const char *error_ = nullptr;

    int width_;
    int height_;
    int orientation_ = 1;
    unsigned char *imgBuf_ = nullptr;

    MetaData metaData_;
};
