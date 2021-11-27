#!/bin/sh

# Expects RACK_SDK_VERSION environment variable to be set in workflow

set -eu

curl -o RackSDK.zip https://vcvrack.com/downloads/Rack-SDK-${{ RACK_SDK_VERSION }}-${{ ARCH }}.zip
unzip Rack-SDK-${{ RACK_SDK_VERSION }}.zip
rm Rack-SDK-${{ RACK_SDK_VERSION }}.zip

#curl -L https://vcvrack.com/downloads/Rack-SDK-${RACK_SDK_VERSION}.zip -o rack-sdk.zip
#unzip -o rack-sdk.zip
#rm rack-sdk.zip

mkdir -p plugins
