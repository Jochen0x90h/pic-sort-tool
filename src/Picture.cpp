#include "Picture.hpp"

// jpeg
#include "TinyEXIF.h" // https://github.com/cdcseacave/TinyEXIF

// standard library
#include <filesystem>
#include <format>
#include <fstream>


Picture::Picture(const fs::path &path) {
    auto &meta = metaData_;

    // determine jpeg size
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    int jpegSize = int(file.tellg());
    file.seekg(0);

    // allocate jpeg buffer
    unsigned char *jpegBuf = NULL;
    if ((jpegBuf = (unsigned char *)tjAlloc(jpegSize)) == NULL) {
        throw std::runtime_error("Can't allocate JPEG buffer");
    }

    // read jpeg into buffer
    file.read(reinterpret_cast<char *>(jpegBuf), jpegSize);
    file.close();

    // read exif
    TinyEXIF::EXIFInfo exif(jpegBuf, jpegSize);
    if (exif.Fields) {
        // get image orientation
        orientation_ = exif.Orientation;

        // get date/time
        if (!exif.DateTime.empty()) {
            auto in = std::istringstream(exif.DateTime);
            in >> std::chrono::parse("%Y:%m:%d %H:%M:%S", meta.time);
            meta.localTime = true;
        }

        // get GPS coordinates
        if (exif.GeoLocation.hasLatLon()) {
            meta.latitude = exif.GeoLocation.Latitude;
            meta.longitude = exif.GeoLocation.Longitude;
        }
    }

    // init decompressor
    tjhandle tjInstance = NULL;int selectedTarget = -1;
    if ((tjInstance = tjInitDecompress()) == NULL) {
        setError("initializing decompressor", tjInstance);
        return;
    }

    // decompress header
    int inSubsamp, inColorspace;
    if (tjDecompressHeader3(tjInstance, jpegBuf, jpegSize, &width_, &height_, &inSubsamp, &inColorspace) < 0) {
        setError("reading JPEG header", tjInstance);
        return;
    }

    // allocate image
    int pixelFormat = TJPF_RGB;
    if ((this->imgBuf_ = (unsigned char *)tjAlloc(width_ * height_ * tjPixelSize[pixelFormat])) == NULL) {
        setError("allocating uncompressed image buffer");
        return;
    }

    // decompress image
    int flags = TJFLAG_FASTDCT | TJFLAG_FASTUPSAMPLE;
    if (tjDecompress2(tjInstance, jpegBuf, jpegSize, imgBuf_, width_, 0, height_,
        pixelFormat, flags) < 0)
    {
        setError("decompressing JPEG image", tjInstance);
    }

    // free
    tjFree(jpegBuf);
    tjDestroy(tjInstance);
}

Picture::~Picture() {
    tjFree(imgBuf_);
}

ImageData Picture::getImageData() {
    return {width_, height_, orientation_, imgBuf_};
}

MetaData Picture::getMetaData() {
    return metaData_;
}

void Picture::setError(char const *action) {
    action_ = action;
    error_ = strerror(errno);
}

void Picture::setError(char const *action, tjhandle tjInstance) {
    action_ = action;
    error_ = tjGetErrorStr2(tjInstance);
}
