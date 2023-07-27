from setuptools import setup, Extension
with open('README.md', 'r') as f:
    long_desc = f.read()
setup(
    name='py_cycle',
    version='0.0.1',
    description='Get CPU cycle diff',
    long_description=long_desc,
    author='Pengyu Liu',
    author_email='eic_lpy@hust.edu.cn',
    ext_modules=[Extension('py_cycle', sources=['py_cycle.c'])]
)
