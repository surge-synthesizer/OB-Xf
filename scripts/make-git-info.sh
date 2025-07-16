#!/bin/sh

mkdir -p pitmp
pushd pitmp
git clone https://github.com/surge-synthesizer/sst-plugininfra
popd
cmake -DGITHUB_ACTIONS_BUILD=TRUE -DGENERATE_BUILD_VERSION=TRUE \
  -P pitmp/sst-plugininfra/cmake/git-version-functions.cmake
rm -rf pitmp

cat BUILD_VERSION