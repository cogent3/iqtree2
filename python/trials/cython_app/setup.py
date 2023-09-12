from setuptools import setup
from Cython.Build import cythonize
from setuptools.extension import Extension
import os
if os.name == 'nt':  # Windows
    compile_args = ['/std:c++11']
else:
    compile_args = ['-std=c++11']
    
setup(
    name='cython_wrapper',
    ext_modules=[
        Extension(
            "cython_wrapper",
            sources=["cython_wrapper.pyx", "../cpp_lib/mock_iqtree.cpp"],
            language="c++",
            language_level=3,
            extra_compile_args=compile_args,
        )
    ]
)
