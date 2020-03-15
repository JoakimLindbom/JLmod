#ifndef CLOCK_HPP
#define CLOCK_HPP

class Clock {
  double step;  // -1.0 when stopped, [0 to 2*period[ for clock steps (*2 is because of swing, so we do groups of 2 periods)
  double length;  // double period
  double sampleTime;
  int iterations;  // run this many double periods before going into sync if sub-clock
  static constexpr double guard = 0.0005;  // in seconds, region for sync to occur right before end of length of last iteration; sub clocks must be low during this period
  bool *resetClockOutputsHigh;

  public:
  Clock();
  void reset2();
  bool isReset2();
  double getStep();
  void setup(Clock* clkGiven, bool *resetClockOutputsHighPtr);
  void start2();

  void setup(double lengthGiven, int iterationsGiven, double sampleTimeGiven);

  void stepClock();

  void applyNewLength(double lengthStretchFactor);

  int isHigh(float swing, float pulseWidth);
};

#endif
