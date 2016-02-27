from conans import ConanFile
import os

class SiliciumConan(ConanFile):
    name = "sqlite3pp"
    version = "0.5"
    generators = "cmake"
    requires = "silicium/0.8@TyRoXx/master", "sqlite3/3.10.2@TyRoXx/stable"
    url="http://github.com/tyroxx/sqlite3pp"
    license="MIT"
    exports="sqlite3pp/*"

    def package(self):
        self.copy(pattern="*.hpp", dst="include/sqlite3pp", src="sqlite3pp", keep_path=True)

    def imports(self):
        self.copy("*.dll", dst="bin", src="bin")
        self.copy("*.dylib*", dst="bin", src="lib")
