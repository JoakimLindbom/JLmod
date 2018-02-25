<img src="https://i.imgur.com/97vmDVl.png" width="15%">

# JLmod 
[VCV Rack](https://github.com/VCVRack/Rack) modules by Joakim Lindbom.

Version 0.5

- [Contents](#contents)
  - [Utilities](#utilities)
    - [Timer](#timer)
- [Building](#building)
- [License](#license)

## Contents

### Utilities

#### Timers
Send a trigger after n seconds.
Range: 0.1 - 999.9 seconds
A timer can re-trigger itself or another timer.
The timer starts when the Trigger button is pressed, or when there's an external trigger received.


## Building

First, clone and [make Rack for yourself](https://github.com/VCVRack/Rack#building).

Then, clone this repo into the `plugins` directory and run `make` from this directory.

## License

(c) Joakim Lindbom 2018, BSD.
