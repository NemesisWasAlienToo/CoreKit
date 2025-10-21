from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMake

class CoreKitConan(ConanFile):
    name = "CoreKit"
    version = "0.1"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"
    layout = cmake_layout

    def requirements(self):
        self.requires("ctre/3.10.0")
        self.requires("openssl/3.6.0")

    def build_requirements(self):
        self.tool_requires("cmake/3.27.9")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    