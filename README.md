<img src="https://i.imgur.com/97vmDVl.png" width="15%">
<img src="https://imgur.com/VUkUCzJ.png" width="22%">

# JLmod 
[VCV Rack](https://github.com/VCVRack/Rack) modules by Joakim Lindbom.

- [Contents](#contents)
  - [Utilities](#utilities)
    - [Timers](#timer)
    - [Ratchets](#sequencer)
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
Gate and note sequencer with 1-8 ratchets per step and with a built-in bernouli gate for each step selecting between two different ratchet settings.
Number of steps in the sequencer can be set and individual steps can be disabled.


## Building

First, clone and [make Rack for yourself](https://github.com/VCVRack/Rack#building).

Then, clone this repo into the `plugins` directory and run `make` from this directory.

## License

Copyright (c) Joakim Lindbom 2018-2019, BSD.

## Acknowledgements

Ratchets is using the clock function and graphics from Imprompt Modular.

