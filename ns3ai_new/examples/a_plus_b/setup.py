from setuptools import setup
from setuptools.extension import Extension
from Cython.Build import cythonize

extensions = [
    Extension(
        "ns3ai_apb",
        ["apb.pyx"],
        include_dirs=['/opt/homebrew/Cellar/boost/1.81.0_1/include'], 
        libraries=['boost_program_options'], 
        library_dirs=['/opt/homebrew/Cellar/boost/1.81.0_1/lib'], 
        extra_compile_args=['-std=c++14']
    )
]

setup(
    name="ns3ai",
    ext_modules=cythonize(extensions, language_level="3", include_path=['../../model'])
)
