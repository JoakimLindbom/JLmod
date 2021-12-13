#!/bin/sh

set -eu

export RACK_DIR=${GITHUB_WORKSPACE}/Rack-SDK
export RACK_USER_DIR=${GITHUB_WORKSPACE}

echo Download SDK
pwd
curl -L https://vcvrack.com/downloads/Rack-SDK-2.0.3-lin.zip -o rack-sdk.zip
unzip -o rack-sdk.zip
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
