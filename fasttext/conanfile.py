import shutil
from conans import ConanFile, tools, CMake

class FastText(ConanFile):
    name = "fasttext"
    version = "0.2.0"

    url = "https://github.com/facebookresearch/fastText"
    description = "Library for fast text representation and classification"
    license = "MIT"

    settings = "os", "arch", "compiler", "build_type"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    generators = "cmake"

    exports_sources = "CMakeLists.txt"

    def configure(self):
        if self.settings.compiler == 'Visual Studio':
            del self.options.fPIC

    def source(self):
        url = "https://github.com/facebookresearch/fastText/archive/v{}.tar.gz".format(self.version)
        tools.get(url, sha256="71d24ffec9fcc4364554ecac2b3308d834178c903d16d090aa6be9ea6b8e480c")
        shutil.move("fasttext-{}".format(self.version), self.name)

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
        self.cpp_info.libs = ['fasttext']
