#!/usr/bin/env bash

#------------------------------------------------------------------------------
# Bash script for building Solidity for emscripten.
#
# The documentation for solidity is hosted at:
#
#     https://solidity.readthedocs.org
#
# ------------------------------------------------------------------------------
# This file is part of solidity.
#
# solidity is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# solidity is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with solidity.  If not, see <http://www.gnu.org/licenses/>
#
# (c) 2016 solidity contributors.
#------------------------------------------------------------------------------

set -e

if test -z "$1"; then
    BUILD_DIR="emscripten_build"
else
    BUILD_DIR="$1"
fi

docker run -v $(pwd):/root/project -w /root/project ethereum/solidity-buildpack-deps:emsdk-1.39.15-2 \
    ./scripts/travis-emscripten/build_emscripten.sh $BUILD_DIR
