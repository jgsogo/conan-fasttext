
import os
from conans import ConanFile


class TestPackage(ConanFile):
    settings = "os", "arch"

    def test(self):
        input_data = os.path.join(os.path.dirname(__file__), "data.txt")
        self.run('fasttext  skipgram -input "{}" -output model -minCount 1 -thread 1'.format(input_data))
