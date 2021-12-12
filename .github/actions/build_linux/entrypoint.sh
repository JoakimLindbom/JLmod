#!/bin/sh

set -eu

#export RACK_DIR=${GITHUB_WORKSPACE}/Rack-SDK-lin
export RACK_DIR=${GITHUB_WORKSPACE}/Rack-SDK
#export RACK_DIR=${GITHUB_WORKSPACE}/Rack-SDK-lin/Rack-SDK
export RACK_USER_DIR=${GITHUB_WORKSPACE}

echo Download SDK
pwd
#cd $RACK_DIR
#curl -L https://vcvrack.com/downloads/Rack-SDK-"${RACK_SDK_VERSION}"-lin.zip -o rack-sdk.zip
curl -L https://vcvrack.com/downloads/Rack-SDK-2.0.3-lin.zip -o rack-sdk.zip
unzip -o rack-sdk.zip
#unzip -o -d Rack-SDK-lin rack-sdk.zip
rm rack-sdk.zip

#cd $GITHUB_WORKSPACE
echo Starting Linux make
pwd
ls
echo Clean up
make clean
echo make dist
make dist
echo Finished!
