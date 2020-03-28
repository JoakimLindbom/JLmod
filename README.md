# JLmod 
![Build Master](https://github.com/JoakimLindbom/JLmod/workflows/Build%20Master/badge.svg)

[VCV Rack](https://github.com/VCVRack/Rack) modules by Joakim Lindbom.

- [Contents](#contents)
  - [Utilities](#utilities)
    - [Timers](#Timers)
    - [Ratchet](#Ratchets)
    - [Ratchets debugger](#Ratchets_debugger)
- [Building](#building)
- [License](#license)

## Contents

### Utilities

#### Timers
<< Discontinued for the moment >>

Send a trigger after n seconds.
Range: 0.1 - 999.9 seconds
A timer can re-trigger itself or another timer.
The timer starts when the Trigger button is pressed, or when there's an external trigger received.

#### Ratchets
<img src="https://user-images.githubusercontent.com/3755877/77819450-35bdc780-70db-11ea-9758-4c60c0cb8fb8.png" width="25%">


8 step gate and note sequencer with 1-8 ratchets per step and with a built-in bernouli gate for each step selecting between two different ratchet settings.
Number of steps in the sequencer can be set and individual steps can be disabled.
Each step can pan (left-right) with adjustable spread. 

#### Ratchets_debugger
<img src="https://user-images.githubusercontent.com/3755877/77819454-3b1b1200-70db-11ea-9c04-82abc0274c90.png" width="25%">

Expander module to Ratchets that exposes the 8 internal clocks and the 8 steps in the sequencer. Used by me for debugging, but can be used as a utility for e.g. triggering at specific step.



## Building

First, clone and [make Rack for yourself](https://github.com/VCVRack/Rack#building).

Then, clone this repo into the `plugins` directory and run `make` from this directory.

## License

Copyright (c) Joakim Lindbom 2018-2020, BSD.

## Acknowledgements

Ratchets is using the clock function and graphics from Imprompt Modular.

