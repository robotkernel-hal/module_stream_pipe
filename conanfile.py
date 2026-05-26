from conan import ConanFile

class MainProject(ConanFile):
    python_requires = "conan_template/[~6]@robotkernel/stable"
    python_requires_extend = "conan_template.RobotkernelConanFile"

    name = "module_stream_pipe"
    description = ""
    exports_sources = ["*", "!.gitignore"] 
    requires = "robotkernel/[~6]@robotkernel/stable"

