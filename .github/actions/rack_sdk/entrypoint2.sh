#!/bin/bash

# Expects RACK_SDK_VERSION environment variable to be set in workflow

set -eu
RACK_SDK_VERSION='2.3.0'
ARCH='lin'

# export RACK_DIR=${GITHUB_WORKSPACE}/Rack-SDK

echo Testing act 1234
#echo $RACK_SDK_VERSION

#curl -o RackSDK.zip https://vcvrack.com/downloads/Rack-SDK-${{ RACK_SDK_VERSION }}-${{ ARCH }}.zip
#unzip Rack-SDK-${{ RACK_SDK_VERSION }}.zip
#rm Rack-SDK-${{ RACK_SDK_VERSION }}.zip

curl -L https://vcvrack.com/downloads/Rack-SDK-2.0.3-lin.zip -o Rack-SDK-2-lin.zip
unzip -o -d Rack-SDK-lin Rack-SDK-2-lin.zip
rm Rack-SDK-2-lin.zip

curl -L https://vcvrack.com/downloads/Rack-SDK-2.0.3-win.zip -o Rack-SDK-2-win.zip
unzip -o -d Rack-SDK-win Rack-SDK-2-win.zip
rm Rack-SDK-2-win.zip

mkdir -p plugins
