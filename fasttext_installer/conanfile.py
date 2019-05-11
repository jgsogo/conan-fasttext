import os
import shutil
from conans import ConanFile, tools, CMake

class FastText(ConanFile):
    name = "fasttext_installer"
    version = "0.2.0"

    url = "https://github.com/facebookresearch/fastText"
    description = "Library for fast text representation and classification"
    license = "MIT"

    settings = "os", "arch", "build_type"
    generators = "cmake"

    exports_sources = "CMakeLists.txt"

    def requirements(self):
        self.requires("fasttext/{}@{}/{}".format(self.version, self.user, self.channel))

    def source(self):
        # main.cpp
        url = "https://raw.githubusercontent.com/facebookresearch/fastText/v{}/src/main.cc".format(self.version)
        tools.download(url, filename="main.cc")

        # LICENSE
        url = "https://raw.githubusercontent.com/facebookresearch/fastText/v{}/LICENSE".format(self.version)
        tools.download(url, filename="LICENSE")

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.configure()
        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        self.copy("LICENSE", src=self.name, dst="licenses")
        cmake = self._configure_cmake()
        cmake.install()

    def package_info(self):
        self.env_info.path.append(os.path.join(self.package_folder, "bin"))
