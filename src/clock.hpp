#ifndef CLOCK_HPP
#define CLOCK_HPP

class Clock {
	// The -1.0 step is used as a reset state every period so that
	//   lengths can be re-computed; it will stay at -1.0 when a clock is inactive.
	// a clock frame is defined as "length * iterations + syncWait", and
	//   for master, syncWait does not apply and iterations = 1

	double step;// -1.0 when stopped, [0 to period[ for clock steps
	double length;// period
	double sampleTime;
	int iterations;// run this many periods before going into sync if sub-clock
	Clock* syncSrc = nullptr; // only subclocks will have this set to master clock
	static constexpr double guard = 0.0005;// in seconds, region for sync to occur right before end of length of last iteration; sub clocks must be low during this period
	bool *resetClockOutputsHigh;
	public:

	Clock();
	void reset();
	bool isReset();

	double getStep();

	void construct(Clock* clkGiven, bool *resetClockOutputsHighPtr);

	void start();

	void setup(double lengthGiven, int iterationsGiven, double sampleTimeGiven);

	void stepClock();

	void applyNewLength(double lengthStretchFactor);

	int isHigh();

};

#endif
