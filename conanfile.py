from conan import ConanFile
from conan.tools.files import copy
from conan.tools.cmake import CMake


class Project(ConanFile):
    name = "pic-sort-tool"
    description = "Tool for sorting pictures into directories"
    url = "https://github.com/Jochen0x90h/pic-sort-tool"
    license = "MIT"
    settings = "os", "compiler", "build_type", "arch"
    default_options = {
        "ffmpeg/*:with_lzma": False,
        "ffmpeg/*:with_libaom": False,
        "ffmpeg/*:with_libdav1d": False,
        "ffmpeg/*:with_freetype": False,
        "ffmpeg/*:with_libx264": False,
        "ffmpeg/*:with_libx265": False,
        "ffmpeg/*:with_libvpx": False,
        "ffmpeg/*:with_openh264": False,
        "ffmpeg/*:with_libsvtav1": False,
        "ffmpeg/*:with_libwebp": False,
        "ffmpeg/*:with_openjpeg": False,
        "ffmpeg/*:with_libmp3lame": False,
        "ffmpeg/*:with_libfdk_aac": False,
        "ffmpeg/*:with_opus": False,
        "ffmpeg/*:with_vorbis": False,
        "ffmpeg/*:with_bzip2": False,
        "ffmpeg/*:with_ssl": False,
        "ffmpeg/*:with_zeromq": False,
        "ffmpeg/*:with_sdl": False,
        "ffmpeg/*:with_fontconfig": False,
        "ffmpeg/*:avdevice": False,
        "ffmpeg/*:with_programs": False,
        "ffmpeg/*:postproc": False,
        "ffmpeg/*:avfilter": False,
        "ffmpeg/*:postproc": False,
        "ffmpeg/*:avdevice": False,
        "ffmpeg/*:with_asm": False,
    }

    generators = "CMakeDeps", "CMakeToolchain"
    exports_sources = "conanfile.py", "CMakeLists.txt", "src/*", "test/*"
    requires = [
        "glfw/3.4",
        "imgui/1.92.9b",
        "libjpeg-turbo/3.2.0",
        "ffmpeg/9.0.2",
        "exiv2/0.28.3"
    ]

    keep_imports = True
    def imports(self):
        # copy dependent libraries into the build folder
        self.copy("*", src="@bindirs", dst="bin")
        self.copy("*", src="@libdirs", dst="lib")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

        # run unit tests if CONAN_RUN_TESTS environment variable is set to 1
        #if os.getenv("CONAN_RUN_TESTS") == "1":
        #    cmake.test()

    def package(self):
        # install from build directory into package directory
        cmake = CMake(self)
        cmake.install()

        # also copy dependent libraries into the package
        #self.copy("*.dll", "bin", "bin")
        #self.copy("*.dylib*", "lib", "lib", symlinks = True)
        #self.copy("*.so*", "lib", "lib", symlinks = True)

    def package_info(self):
        pass
