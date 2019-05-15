import os

from conans import ConanFile, python_requires

data = python_requires("fasttext_data/0.2.0@jgsogo/stable")

crawl_vector_languages = data.CrawlVectors.list_languages()
supervised_models = data.SupervisedModels.list_models()


def option_crawl(lang):
    return "crawl_{}".format(lang)


def option_supervised(lang):
    return "supervised_{}".format(lang)
    

class Example(ConanFile):
    name = "example"
    version = "0.2.0"

    options = dict({option_crawl(lang): [True, False] for lang in crawl_vector_languages},
                   **{option_supervised(model): [True, False] for model in supervised_models})
    default_options = dict({option_crawl(lang): False for lang in crawl_vector_languages},
                           **{option_supervised(model): False for model in supervised_models})

    def requirements(self):
        self.requires("fasttext_installer/{}@{}/{}".format(self.version, self.user, self.channel))

    def imports(self):
        self.copy("fasttext", "", "bin")
        self.copy("*.dll", "", "bin")
        self.copy("*.dylib", "", "lib")

    def package(self):
        self.copy("fasttext", src="", dst="bin")
        for it in crawl_vector_languages:
            if getattr(self.options, option_crawl(it)):
                dest_path = os.path.join(self.package_folder, 'data', 'crawl_vector', it)
                self.output.info("Install crawl_vectors ('{}') to '{}'".format(it, dest_path))
                data.CrawlVectors.download(it, "bin", dest_path, output=self.output)
            
        for it in supervised_models:
            if getattr(self.options, option_supervised(it)):
                dest_path = os.path.join(self.package_folder, 'data', 'supervised')
                self.output.info("Install supervised_models ('{}') to '{}'".format(it, dest_path))
                data.SupervisedModels.download(it, "regular", dest_path, output=self.output)
    