import os
import re

import requests
from conans import ConanFile, tools


class CrawlVectors:
    url = "https://fasttext.cc/docs/en/crawl-vectors.html"

    @staticmethod
    def list_languages():
        r = requests.get(CrawlVectors.url)

        bin_gz = re.compile(r'<a href="([^"]*?\.bin\.gz)">')
        vec_gz = re.compile(r'<a href="([^"]*?\.vec\.gz)">')
        lang_pattern = re.compile(r'^https:\/\/dl\.fbaipublicfiles\.com\/fasttext\/vectors-crawl\/cc\.([a-z]+)\.300\.bin\.gz$')
        languages = []
        for it in re.findall(bin_gz, r.text):
            lang = re.match(lang_pattern, it)
            languages.append(lang.group(1))
        return languages

    @staticmethod
    def download(lang, format, dest_folder, output, delete_if_exists=False):
        assert format in ["bin", "vec"]
        url = "https://dl.fbaipublicfiles.com/fasttext/vectors-crawl/cc.{}.300.{}.gz".format(lang, format)
        dest_filename = os.path.join(dest_folder, "cc.{}.300.{}".format(lang, format))
        if not os.path.exists(dest_filename) or delete_if_exists:
            output.info(" - output: {}".format(dest_filename))
            tools.get(url, destination=dest_filename)
            # TODO: Copy license to dest_folder


class SupervisedModels:
    @staticmethod
    def list_models():
        return ["ag_news", "amazon_review_full", "amazon_review_polarity", "dbpedia", "sogou_news", "yahoo_answers", "yelp_review_polarity", "yelp_review_full"]

    @staticmethod
    def download(model, dataset, dest_folder, output, delete_if_exists=False):
        assert dataset in ["regular", "compressed"]
        ext = "bin" if dataset == "regular" else "ftz"
        url = "https://dl.fbaipublicfiles.com/fasttext/supervised-models/{}.{}".format(model, ext)
        dest_filename = os.path.join(dest_folder, "{}.{}".format(model, ext))
        if not os.path.exists(dest_filename) or delete_if_exists:
            output.info(" - output: {}".format(dest_filename))
            tools.download(url, filename=dest_filename)


class CrawlVectorsPackage(ConanFile):
    name = "fasttext_data"
    version = "0.2.0"

    url = "https://fasttext.cc/docs/en/crawl-vectors.html"
    description = "Pre-trained word vectors for 157 languages, trained on Common Crawl and Wikipedia using fastText"
    license = "Creative Commons Attribution-Share-Alike License 3.0"
