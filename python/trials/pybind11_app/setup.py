from setuptools import setup, Extension
import pybind11
setup(
    name='pybind_wrapper',
    ext_modules=[
        Extension(
            'pybind_wrapper',
            sources=['pybind_wrapper.cpp', '../Cpp_lib/mock_iqtree.cpp'],
            include_dirs=['.', '../Cpp_lib',pybind11.get_include()],
            library_dirs=['../Cpp_lib/build/Release'], 
            libraries=['mock_iqtree'], 
            language='c++'
        ),
    ],
)
