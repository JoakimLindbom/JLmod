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

curl -L https://vcvrack.com/downloads/Rack-SDK-2.beta.1.zip -o rack-sdk.zip
unzip -o rack-sdk.zip
rm rack-sdk.zip

mkdir -p plugins
