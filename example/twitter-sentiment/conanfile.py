
from conans import ConanFile


class TwitterSentiment(ConanFile):
    name = "twitter-sentiment"
    version = "0.0"

    settings = "os", "arch", "build_type"
    generators = "cmake_find_package"

    def requirements(self):
        self.requires("rxcpp/4.1.0@bincrafters/stable")
        self.requires("fasttext/0.2.0@jgsogo/stable")
        self.requires("fmt/5.2.1@bincrafters/stable")
        self.requires("oauth/1.0.3@jgsogo/stable")
        self.requires("jsonformoderncpp/3.6.1@vthiery/stable")
        self.requires("range-v3/0.5.0@ericniebler/stable")
