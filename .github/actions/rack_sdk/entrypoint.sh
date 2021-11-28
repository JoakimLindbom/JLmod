#!/bin/bash

# Expects RACK_SDK_VERSION environment variable to be set in workflow

set -eu
RACK_SDK_VERSION='2.beta.1'
ARCH='lin'

echo Testing act
#echo $RACK_SDK_VERSION

#curl -o RackSDK.zip https://vcvrack.com/downloads/Rack-SDK-${{ RACK_SDK_VERSION }}-${{ ARCH }}.zip
#unzip Rack-SDK-${{ RACK_SDK_VERSION }}.zip
#rm Rack-SDK-${{ RACK_SDK_VERSION }}.zip

curl -L https://vcvrack.com/downloads/Rack-SDK-2.beta.1-lin.zip -o Rack-SDK-2-lin.zip
unzip -o Rack-SDK-2-lin.zip
rm Rack-SDK-2-lin.zip

mkdir -p plugins
