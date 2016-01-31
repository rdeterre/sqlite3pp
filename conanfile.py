from conans import ConanFile
import os

class SiliciumConan(ConanFile):
    name = "sqlite3pp"
    version = "0.1"
    generators = "cmake"
    requires = "silicium/0.3@TyRoXx/master"
    url="http://github.com/tyroxx/sqlite3pp"
    license="MIT"
    exports="sqlite3pp/*"

    def package(self):
        self.copy(pattern="*.hpp", dst="include/sqlite3pp", src="sqlite3pp", keep_path=True)
