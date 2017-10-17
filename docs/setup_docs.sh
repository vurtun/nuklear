#!/bin/bash

# Dependencies required to generate documentation

if [[ "$(which pip)" == "" ]]; then
  echo "Could not find pip, please python pip from your repos"
  exit 1
fi

if [[ "$(which doxygen)" == "" ]]; then
  echo "Could not find doxygen, please doxygen from your repos"
  exit 1
fi

pip install sphinx
pip install breathe
pip install -e git+git://github.com/nuklear-ui/sphinx-to-github.git#egg=sphinx-to-github
