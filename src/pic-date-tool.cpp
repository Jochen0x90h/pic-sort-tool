#include "dst.hpp"
#include <exiv2/exiv2.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <filesystem>
#include <ranges>
#include <errno.h>


namespace fs = std::filesystem;
using namespace std::chrono_literals;

// https://forum.arduino.cc/t/rtc-mit-sommerzeit/168068
/*bool summertime_EU(int year, int month, int day, int hour, int tzHours)
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
}*/


void fix(const fs::path &path, int timeShift, DstType dstType) {
    // read exif
    auto image = Exiv2::ImageFactory::open(path.string());
    image->readMetadata();
    auto& exif = image->exifData();

    // get date/time
    Exiv2::Exifdatum& dateTime = exif["Exif.Photo.DateTimeOriginal"];
    if (dateTime.count() > 0) {
        struct {
            std::chrono::time_point<std::chrono::file_clock> time;
        } meta;
        
        auto in = std::istringstream(dateTime.value().toString());
        in >> std::chrono::parse("%Y:%m:%d %H:%M:%S", meta.time);
    
       // convert date/time to UTC
        auto time = meta.time - timeShift * 1h;

        // detect daylight saving time
        auto systemTime = std::chrono::clock_cast<std::chrono::system_clock>(time);
        auto tt = std::chrono::system_clock::to_time_t(systemTime);
        tm t = *gmtime(&tt); // UTC
        //tm t = *localtime(&tt);
        if (isDst(dstType, 1900 + t.tm_year, t.tm_mon + 1, t.tm_mday, (t.tm_wday == 0) ? 7 : t.tm_wday)) {
            time -= 1h;
        }
    
        // set file date
        fs::last_write_time(path, time);
    }


    /*
    // determine jpeg size
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    int jpegSize = int(file.tellg());
    file.seekg(0);

    // allocate jpeg buffer
    uint8_t *jpegBuf = new uint8_t[jpegSize];

    // read jpeg into buffer
    file.read(reinterpret_cast<char *>(jpegBuf), jpegSize);
    file.close();

    // read exif
    TinyEXIF::EXIFInfo exif(jpegBuf, jpegSize);
    if (exif.Fields) {
        // get date
        if (!exif.DateTime.empty()) {
            // parse time
            auto in = std::istringstream(exif.DateTime);
            std::chrono::time_point<std::chrono::file_clock> metaTime;
            in >> std::chrono::parse("%Y:%m:%d %H:%M:%S", metaTime);
            if (metaTime.time_since_epoch().count() != 0) {
                using namespace std::chrono_literals;

                // convert date/time to UTC
                auto time = metaTime - timeShift * 1h;

                // detect daylight saving time
                auto systemTime = std::chrono::clock_cast<std::chrono::system_clock>(time);
                auto tt = std::chrono::system_clock::to_time_t(systemTime);
                tm t = *gmtime(&tt); // UTC
                //tm t = *localtime(&tt);
                if (isDst(dstType, 1900 + t.tm_year, t.tm_mon + 1, t.tm_mday, (t.tm_wday == 0) ? 7 : t.tm_wday)) {
                    time -= 1h;
                }

                // set file date
                fs::last_write_time(path, time);
            }
        }
    }*/
}

void doDirectory(const fs::path &path, int timeShift, DstType dstType) {
    if (fs::is_directory(path)) {
        fs::directory_iterator end; // default construction yields past-the-end
        for (fs::directory_iterator it(path); it != end; ++it) {
            //std::cout << fs::path(*it).string() << std::endl;
            doDirectory(*it, timeShift, dstType);
        }
    } else {
        std::string ext = path.extension().string();
        if (ext == ".jpg" || ext == ".JPG") {
            std::cout << path.string() << std::endl;
            fix(path, timeShift, dstType);
        }
    }
}


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

    doDirectory(".", timeShift, dstType);

    return 0;
}
