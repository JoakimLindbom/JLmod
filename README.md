# JLmod 
[VCV Rack](https://github.com/VCVRack/Rack) modules by Joakim Lindbom.

- [Contents](#contents)
  - [Utilities](#utilities)
    - [Timers](#timer)
    - [Ratchets](#sequencer)
    - [Ratchets debugger](#utility)
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
<img src="https://imgur.com/a/TJ4kt4q" width="22%">

8 step gate and note sequencer with 1-8 ratchets per step and with a built-in bernouli gate for each step selecting between two different ratchet settings.
Number of steps in the sequencer can be set and individual steps can be disabled.
Each step can pan (left-right) with adjustable spread. 

#### Ratchets debugger
<img src="https://imgur.com/a/ZwP3L26" width="15%">

Expander module to Ratchets that exposes the 8 internal clocks and the 8 steps in the sequencer. Used for debugging, but can be used as a utility for e.g. triggering at specific step.



## Building

First, clone and [make Rack for yourself](https://github.com/VCVRack/Rack#building).

Then, clone this repo into the `plugins` directory and run `make` from this directory.

## License

Copyright (c) Joakim Lindbom 2018-2020, BSD.

## Acknowledgements

Ratchets is using the clock function and graphics from Imprompt Modular.

