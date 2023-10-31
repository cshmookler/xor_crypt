"""xor_crypt root Conan file"""

from os.path import join as join_path
from typing import List
from dataclasses import dataclass

from conan import ConanFile
from conan.tools.files import copy as copy_file
from conan.tools.gnu import PkgConfigDeps
from conan.tools.meson import Meson, MesonToolchain
from conan.tools.scm import Git
from conan.errors import ConanException

required_conan_version = ">=2.0.6"


@dataclass
class MesonDependency:
    """Meson dependency information"""
    package_name: str
    component_name: str
    version: str


class xor_crypt(ConanFile):
    """xor_crypt"""

    # Required
    name = "xor_crypt"

    # Metadata
    license = "Zlib"
    author = "Caden Shmookler (cshmookler@gmail.com)"
    url = "https://github.com/cshmookler/xor_crypt.git"
    description = "Uses a single-use pad to encrypt and decrypt data."
    topics = []

    # Configuration
    package_type = "application" 
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    build_policy = "missing"

    # Essential files
    exports_sources = ".git/*", "include/*", "src/*", "meson.build", "LICENSE"

    # Paths
    _build_folder = "build"
    _generator_folder = join_path(_build_folder, "generators")

    # Other
    _dependencies: List[str] = []

    def set_version(self):
        """Get project version from Git"""
        git = Git(self, folder=self.recipe_folder)
        try:
            self.version = (
                git.run("describe --tags").replace("-", ".").rpartition(".")[0]
            )
        except ConanException:
            self.version = "0.0.0"

    def config_options(self):
        """Change available options"""
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        """Change behavior based on set options"""
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def build_requirements(self):
        """Declare dependencies of the build system"""
        self.tool_requires("meson/1.2.1")
        self.tool_requires("pkgconf/2.0.3")

    def requirements(self):
        """Declare library dependencies"""
        for dep in self._dependencies:
            self.requires(dep)

    def layout(self):
        """Set the layout of the build files"""
        self.folders.build = self._build_folder
        self.folders.generators = self._generator_folder

    def generate(self):
        """Generate the build system"""
        deps = PkgConfigDeps(self)
        deps.generate()

        meson_dependencies: List[MesonDependency] = []
        for pc, content in deps.content.items():
            pc_no_extension = pc.rstrip(".pc")
            split = pc_no_extension.split("-")
            # Assumes that len(split) is always <= 2
            meson_dependencies.append(
                MesonDependency(
                    package_name=split[0],
                    component_name=split[0] + "::" + split[1]
                    if len(split) == 2
                    else "",
                    version=content.split("Version: ")[1]
                    .split("\n")[0]
                    .strip(),
                )
            )

            last: MesonDependency = meson_dependencies[
                len(meson_dependencies) - 1
            ]
            print(
                "Name: "
                + last.package_name
                + " -> Component: "
                + last.component_name
                + " -> Version: "
                + last.version
            )

        meson_dependencies_classless = [
            [i.package_name, i.component_name, i.version]
            for i in meson_dependencies
        ]

        toolchain = MesonToolchain(self)
        toolchain.properties = {
            "name": self.name,
            "version": self.version,
            "deps": meson_dependencies_classless,
        }
        toolchain.generate()

    def build(self):
        """Build the test project"""
        meson = Meson(self)
        meson.configure()
        copy_file(
            self,
            "version.hpp",
            self.build_folder,
            self.source_folder + "/src/",
        )
        meson.build()
        copy_file(
            self,
            "compile_commands.json",
            self.build_folder,
            self.source_folder,
        )

    def package(self):
        """Install project headers and compiled libraries"""
        meson = Meson(self)
        meson.install()

    def package_info(self):
        """Package information"""
        self.cpp_info.libs = [self.name]
        self.cpp_info.includedirs = ["include"]

