JLmod 
=====
![Build Master](https://github.com/JoakimLindbom/JLmod/workflows/Build%20Master/badge.svg)

[VCV Rack](https://github.com/VCVRack/Rack) synthesizer modules by Joakim Lindbom.

<p float="left">
    <img src="https://user-images.githubusercontent.com/3755877/77819450-35bdc780-70db-11ea-9758-4c60c0cb8fb8.png" height="382"> 
    <img src="https://user-images.githubusercontent.com/3755877/77819454-3b1b1200-70db-11ea-9c04-82abc0274c90.png" height=382">
    <img src="https://user-images.githubusercontent.com/3755877/77861183-b67fdf00-7213-11ea-9f5a-6d02522cb15e.png" height=382">

</p>

The main aim for my module collection is sequencing and utility modules of various kinds. Currently there's no plans for any advanced modules working in the sound domain simply because I lack knowledge in that field. But by digging deeper into DSP programming I also expect to learn more...

Contents
========
- [Sequencers](#Sequencers)
    - [Ratchets](#Ratchets)
    - [Ratchets debugger](#Ratchets_debugger)
- [Utilities](#Utilities)
    - [KeySplit](#KeySPlit)
    - [Timers](#Timers)
- [Building - local and cloud](#Building)
- [VCV Rack Library](#library)
- [License](#license)


# Sequencers

## Ratchets
<img src="https://user-images.githubusercontent.com/3755877/77819450-35bdc780-70db-11ea-9758-4c60c0cb8fb8.png" height="191">

8 step gate and note sequencer with 1-8 ratchet "clicks" per step and with a built-in Bernouli gate for each step selecting between two different ratchet settings.

The module needs an external clock. Currently the BPM input is exactly that and not a clock input. I'm working on adding Clock input functionaly to the BPM input.

The Run input and button are used to start and stop the sequencer. 

The Reset input and button resets the step sequencer and makes it restart at the first step.

The Steps knob is used to control the number of steps used by the sequencer. Individual steps can be enabled/ disabled.

Each step can pan (left-right) with adjustable width; each step panner can be enabled/disabled. The panner output can be set to bi-polar (±5V  - default) and then it works as a normal panner. In unipolar mode (0-10V) it can e.g. be used to drive a parameter on a filter.

Each step has an Octave and CV setting. There's also a global octave input and knob, if you want to transpose all CV output.

The Gate output emits gates for each ratchet "click".

The CV output emits CV, based on Ocv+CV + octave transposed if selected.

## Ratchets debugger
<img src="https://user-images.githubusercontent.com/3755877/77819454-3b1b1200-70db-11ea-9c04-82abc0274c90.png" height="191">

Expander module to Ratchets that exposes the 8 internal clocks and the 8 steps in the sequencer. Used by me for debugging, but can be used as a utility for e.g. triggering at specific step.

The Conn light is green when the module has connected to the Ratchets module, that needs to be on the left of the expander.
The Clocks outputs are arranged from top to bottom. The first is the clock generating single "clicks" and the following are the ones generating the multiplied "clicks"
The Steps output emits a gate when the current step is activate; also organised from top to bottom. 

# Utilities
## KeySplit
Experimental.

Keyboard splitter. Input from single keyboard (or sequencer, any CV source) is split up into two CV and gate outputs. Split point can be set by input or knob.
Monophonic at the moment. Next to add polyphony.

<p float="left">
    <img src="https://user-images.githubusercontent.com/3755877/77861134-7d476f00-7213-11ea-90a6-9b74b2a8ddb2.png" height=191">
</p>p>


## Timers
<< Discontinued for the moment >>

Send a trigger after n seconds.

Range: 0.1 - 999.9 seconds

A timer can re-trigger itself or another timer.
The timer starts when the Trigger button is pressed, or when there's an external trigger received.

# Building
## Local computer
First, install VCV Rack SDK asdescribed in the [Plugin Development Tutorial]("https://vcvrack.com/manual/PluginDevelopmentTutorial") or if you're more adventurous [clone and setup a full development version of Rack for yourself](https://github.com/VCVRack/Rack#building).

Then, clone this repo into the `plugins` directory and run `make` from this directory. `make dist` will copy the binaries to your live Rack instance.

## Cloud build
I've set up a cloud build environment using Github Actions. Currently it builds Linux, Windows and OSX versions, all X.86 architecture. All binaries are packaged in a format ready to for direct installation.
If you clone this repo, you'll get the Cloud Build automatically. To enable the Cloud Build, edit `.github/workflows/build.yml` to trigger on commits on the master branch (row 4).

# VCV Rack Library
[This plugin](https://github.com/VCVRack/library/issues/638) is published and handled in Library issue tracker.

# License
Copyright (c) Joakim Lindbom 2018-2020, GPLv3.

# Acknowledgements
The internal clocks of Ratchets is based around code from Imprompt Modular and also some graphics elements are derived from the same.


