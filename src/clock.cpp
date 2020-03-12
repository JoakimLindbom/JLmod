#include "clock.hpp"
#include <functional>

Clock::Clock() {
    reset();
}

void Clock::reset() {
    step = -1.0;
}
bool Clock::isReset() {
    return step == -1.0;
}

double Clock::getStep() {
    return step;
}

void Clock::setSampleRate(double inSampleRate) {
    sampleRate = inSampleRate; // TODO: Cascade to sub clocks. Needed???
}

void Clock::construct(Clock* clkGiven, bool *resetClockOutputsHighPtr) {
    syncSrc = clkGiven;
    resetClockOutputsHigh = resetClockOutputsHighPtr;
}

void Clock::addSubClock(Clock* subClock) {
    //subClocks.push_back(subClock);
    if (numSubClocks < 32 ){
        numSubClocks++;
        //subClocks[numSubClocks] = subClock;
        std::cout << "size: " << numSubClocks << "\n";
    }
}

void Clock::start() {
    step = 0.0;
}

void Clock::setup(double lengthGiven, int iterationsGiven, double sampleTimeGiven) {
    length = lengthGiven;
    iterations = iterationsGiven;
    sampleTime = sampleTimeGiven;
}

void Clock::process() { // Refactored code from calling



}


void Clock::stepClock() {// here the clock was output on step "step", this function is called near end of module::process()
    _tick = false;
    if (step >= 0.0) {// if active clock
        step += sampleTime;
        if ( (syncSrc != nullptr) && (iterations == 1) && (step > (length - guard)) ) {// if in sync region
            if (syncSrc->isReset()) {
                reset();
            }// else nothing needs to be done, just wait and step stays the same
        }
        else {
            if (step >= length) {// reached end iteration
                iterations--;
                step -= length;
                _tick = true;
                if (iterations <= 0)
                    reset();// frame done
            }
        }
    }
}

void Clock::applyNewLength(double lengthStretchFactor) {
    if (step != -1.0)
        step *= lengthStretchFactor;
    length *= lengthStretchFactor;
}

int Clock::isHigh() {
    if (step >= 0.0) {
        return (step < (length * 0.5)) ? 1 : 0;
    }
    return (*resetClockOutputsHigh) ? 1 : 0;
}

bool Clock::Tick() {
    return (_tick);
}
