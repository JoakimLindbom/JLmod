#!/bin/bash
set $modules=\
https://github.com/Aepelzen/AepelzensModules.git \
https://github.com/adbrant/ArableInstruments.git \
https://github.com/antoniograzioli/Autodafe-Drums.git \
https://github.com/antoniograzioli/Autodafe-Drums.git \
https://github.com/antoniograzioli/Autodafe.git \
https://github.com/av500/vcvrackplugins_av500.git \
https://github.com/sebastien-bouffier/Bidoo.git \
https://github.com/bogaudio/BogaudioModules.git \
https://github.com/cfoulc/cf.git \
https://github.com/dBiz/dBiz.git \
https://github.com/dekstop/vcvrackplugins_dekstop.git \
https://github.com/Dewb/monome-rack.git \
https://github.com/djpeterso23662/MrLumps.git \
https://github.com/eres-j/VCVRack-plugin-JE.git \
https://github.com/gratrix/vcv-gratrix.git \
https://github.com/j4s0n-c/trowaSoft-VCV.git \
https://github.com/jeremywen/JW-Modules.git \
https://github.com/jhoar/AmalgamatedHarmonics.git \
https://github.com/KarateSnoopy/vcv-karatesnoopy.git \
https://github.com/leopard86/LOGinstruments.git \
https://github.com/lindenbergresearch/LRTRack.git \
https://github.com/luckyxxl/vcv_luckyxxl.git \
https://github.com/martin-lueders/ML_modules.git \
https://github.com/mhetrick/hetrickcv.git \
https://github.com/Miserlou/RJModules.git \
https://github.com/mschack/VCV-Rack-Plugins.git \
https://github.com/naus3a/NauModular.git \
https://github.com/NonLinearInstru…/NLNRI_VCVRackPlugins.git \
https://github.com/phdsg/PvC.git \
https://github.com/raincheque/qwelk.git \
https://github.com/s-ol/vcvmods.git \
https://github.com/Strum/Strums_Mental_VCV_Modules.git \
https://github.com/VCVRack/AudibleInstruments.git \
https://github.com/VCVRack/Befaco.git \
https://github.com/VCVRack/ESeries.git \
https://github.com/VCVRack/Fundamental.git \
https://github.com/ValleyAudio/ValleyRackFree.git \
https://github.com/RODENTCAT/RODENTMODULES.git
# TOOD: clone into specified directory, or check if duplicate exists
for x in $modules
do
git clone "${x}"
done
# fix up names Don't do this right now, the problem is that if you
# re-run the script, it will check out the repos with the dumb names
# again.
#
# as a tidying step this is a good idea though
if false
then
mv NLNRI_VCVRackPlugins NLNRI
mv Strums_Mental_VCV_Modules Strums_Mental
mv trowaSoft-VCV trowaSoft
mv vcv-gratrix gratrix
mv vcv-karatesnoopy karatesnoopy
mv vcv_luckyxxl luckyxxl
mv vcvmods s-ol
mv VCVRack-plugin-JE JE
mv VCV-Rack-Plugins mscHack
mv vcvrackplugins_av500 av500
mv vcvrackplugins_dekstop dekstop
fi
buildfails=""
# build all plugins
for x in *
do
if [ -d $x ]
then
echo BUILDING IN $x
cd $x
git submodule update --init --recursive
git pull
make dep
if make -j$(nproc)
then
true; # say nothing
else
buildfails="${buildfails} ${x}"
fi
cd - > /dev/null 2>&1
fi
done
echo "BUILD FAILURES: ${buildfails}"
