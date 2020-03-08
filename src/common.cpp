#include <iomanip>
#include <cmath>
#include "common.hpp"

std::stringstream format4display(float value){
    std::stringstream to_display;
    std::stringstream to_display2;

    float value_rounded = round(abs(10.0f * value))/10.0f;

    if (value_rounded < 1) {
        to_display << " " << std::setprecision(1) << std::setw(1) << std::fixed << value_rounded;
    }
    else if (value_rounded < 10) {
        to_display << " " << std::setprecision(2) << std::setw(3) << std::fixed << value_rounded;

    } else {
        to_display << std::setprecision(3) << std::setw(3) << std::fixed << value_rounded;
    }
    to_display2 << to_display.str().substr(0, 4);
    return to_display2;
}


SmallGreyKnobSnap::SmallGreyKnobSnap() {
    //setSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/Knob_28.svg")));
    setSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/RoundSmallBlackKnob.svg")));
	snap = true;
};

SmallGreyKnob::SmallGreyKnob() {
    //setSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/Knob_28.svg")));
    setSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/RoundSmallBlackKnob.svg")));
};

NVGcolor prepareDisplay(NVGcontext *vg, Rect *box, int fontSize) {
	NVGcolor backgroundColor = nvgRGB(0x38, 0x38, 0x38);
	NVGcolor borderColor = nvgRGB(0x10, 0x10, 0x10);
	nvgBeginPath(vg);
	nvgRoundedRect(vg, 0.0, 0.0, box->size.x, box->size.y, 5.0);
	nvgFillColor(vg, backgroundColor);
	nvgFill(vg);
	nvgStrokeWidth(vg, 1.0);
	nvgStrokeColor(vg, borderColor);
	nvgStroke(vg);
	nvgFontSize(vg, fontSize);
	NVGcolor textColor = nvgRGB(0xaf, 0xd2, 0x2c);
	return textColor;
}
