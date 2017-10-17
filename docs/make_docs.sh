#!/bin/bash

# Safety first
set -euf -o pipefail

# Determine the docs root directory
DOCS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd $DOCS_DIR

# Run doxygen to generate xml output
doxygen

# Generate html output via sphinx
make html





