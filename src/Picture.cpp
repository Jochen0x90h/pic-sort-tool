#include "Picture.hpp"
//#include "TinyEXIF.h" // https://github.com/cdcseacave/TinyEXIF
#include <exiv2/exiv2.hpp>
#include <filesystem>
#include <format>
#include <fstream>


namespace {

    double convertToDecimal(const Exiv2::Value& value) {
        if (value.count() < 3) return 0.0;

        // EXIF GPS-Daten bestehen aus 3 rationalen Werten: [0]=Grad, [1]=Minuten, [2]=Sekunden
        double degrees = value.toRational(0).first / double(value.toRational(0).second);
        double minutes = value.toRational(1).first / double(value.toRational(1).second);
        double seconds = value.toRational(2).first / double(value.toRational(2).second);

        return degrees + (minutes / 60.0) + (seconds / 3600.0);
    }

} // namespace

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
    auto image = Exiv2::ImageFactory::open(path.string());
    image->readMetadata();
    auto& exif = image->exifData();

    // get image orientation
    Exiv2::Exifdatum& o = exif["Exif.Image.Orientation"];
    if (o.count() > 0)
        orientation_ = o.value().toUint32();

    // get date/time
    Exiv2::Exifdatum& dateTime = exif["Exif.Photo.DateTimeOriginal"];
    if (dateTime.count() > 0) {
        auto in = std::istringstream(dateTime.value().toString());
        in >> std::chrono::parse("%Y:%m:%d %H:%M:%S", meta.time);
        meta.localTime = true;
    }

    // get geo location (convert from degrees/minutes/seconds to decimal)
    Exiv2::Exifdatum& lat = exif["Exif.GPSInfo.GPSLatitude"];
    Exiv2::Exifdatum& latRef = exif["Exif.GPSInfo.GPSLatitudeRef"];
    Exiv2::Exifdatum& lon = exif["Exif.GPSInfo.GPSLongitude"];
    Exiv2::Exifdatum& lonRef = exif["Exif.GPSInfo.GPSLongitudeRef"];
    if (lat.count() > 0 && latRef.count() > 0 && lon.count() > 0 && lonRef.count() > 0) {
        double latitude = convertToDecimal(lat.value());
        double longitude = convertToDecimal(lon.value());
        if (latRef.value().toString() == "S")
            latitude = -latitude;
        if (lonRef.value().toString() == "W")
            longitude = -longitude;
        meta.latitude = latitude;
        meta.longitude = longitude;
    }

    /*TinyEXIF::EXIFInfo exif(jpegBuf, jpegSize);
    if (exif.Fields) {
        // get image orientation
        if (exif.Orientation != 0)
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
    if (meta.time == std::chrono::time_point<std::chrono::file_clock>()) {
        // no date/time in exif, use file time
        meta.time = fs::last_write_time(path);
        meta.localTime = false;
    }*/

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

void Picture::rotate(const fs::path &path, int rotate, int mirror) {
    auto image = Exiv2::ImageFactory::open(path.string());
    image->readMetadata();
    auto& exif = image->exifData();

    // get image orientation
    int orientation = 1;
    Exiv2::Exifdatum& o = exif["Exif.Image.Orientation"];
    if (o.count() > 0)
        orientation = o.value().toUint32();

    // set image orienation
    exif["Exif.Image.Orientation"] = rotateOrientation(orientation, rotate, mirror);

    // also rotate thumbnail if it exists
    Exiv2::ExifThumb exifThumb(exif);
    Exiv2::DataBuf thumbBuffer = exifThumb.copy();
    if (thumbBuffer.size() > 0) {
        // get thumbnail orientation
        int orientation = 1;
        Exiv2::Exifdatum& o = exif["Exif.Thumbnail.Orientation"];
        if (o.count() > 0)
            orientation = o.value().toUint32();

        // set thumbnail orientation
        exif["Exif.Thumbnail.Orientation"] = rotateOrientation(orientation, rotate, mirror);
    }

    image->writeMetadata();
}
