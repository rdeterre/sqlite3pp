from conans import ConanFile
import os

class SQLite3ppConan(ConanFile):
    name = "sqlite3pp"
    version = "1.0.5"
    generators = "cmake"
    requires = "sqlite3/3.14.1@rdeterre/stable"
    url="http://github.com/rdeterre/sqlite3pp"
    license="MIT"
    exports="*"

    def source(self):
        self.run("git clone --branch {} https://github.com/iwongu/sqlite3pp" \
                 .format(self.version))

    def package(self):
        self.copy(pattern="*", dst="include/sqlite3pp", src="sqlite3pp/headeronly_src", keep_path=True)

    def imports(self):
        self.copy("*.dll", dst="bin", src="bin")
        self.copy("*.dylib*", dst="bin", src="lib")

    def package_info(self):
        self.cpp_info.includedirs = ['include']
