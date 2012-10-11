from setuptools import setup, find_packages
from setuptools.extension import Extension
import sys, os

version = '0.1'

c_ext = Extension("drass", define_macros = [('DEBUG', '1'), ('FIFO', '1')],
                           sources = ["drass.c", "bloom.c", "good_build.c", "suggestions.c", "lookup8.c"])

setup(name='drass',
      version=version,
      description="DRASS bloom filter implementation",
      long_description="""FACS you""",
      ext_modules=[c_ext],
      classifiers=[], # Get strings from http://pypi.python.org/pypi?%3Aaction=list_classifiers
      keywords='bloom filter probabilistic',
      author='Enze Liu and Lars Arvestad, Henrik Stranneheim, Roman Valls Guimera',
      author_email='roman@scilifelab.se',
      url='http://facs.scilifelab.se/',
      license='GPLv3',
      packages=find_packages(exclude=['ez_setup', 'examples', 'tests']),
      include_package_data=True,
      zip_safe=False,
      install_requires=[
          # -*- Extra requirements: -*-
      ],
      entry_points="""
      # -*- Entry points: -*-
      """,
      )
