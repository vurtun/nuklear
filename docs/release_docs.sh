#!/bin/bash

# Safety first
set -euf -o pipefail

# Determine the docs root directory
DOCS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd $DOCS_DIR/build/html
git commit -am '[auto] updating static documentation'
git push origin master




