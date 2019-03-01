#include <iostream>
#include <sstream>
#include "rack.hpp"

#pragma once

using namespace rack;

extern Plugin *plugin;


std::stringstream format4display(float value);

struct SmallGreyKnob : RoundKnob {
        SmallGreyKnob();
};

struct SmallGreyKnobSnap : SmallGreyKnob {
        SmallGreyKnobSnap();
};
