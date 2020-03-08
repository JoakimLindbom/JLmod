#include <iostream>
#include <sstream>
#include "rack.hpp"

#pragma once

using namespace rack;

extern Plugin *pluginInstance;


std::stringstream format4display(float value);

struct SmallGreyKnob : RoundKnob {
        SmallGreyKnob();
};

struct SmallGreyKnobSnap : SmallGreyKnob {
        SmallGreyKnobSnap();
};


// Widgets from ValleyRack

struct Rogan1PSBrightRed : Rogan {
    Rogan1PSBrightRed() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Rogan1PSBrightRed.svg")));
    }
};

struct RoganSmallBrightRed : Rogan {
    RoganSmallBrightRed() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Rogan1PSBrightRedSmall.svg")));
    }
};

struct Rogan1PSYellow : Rogan {
    Rogan1PSYellow() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Rogan1PSYellow.svg")));
    }
};

struct RoganSmallYellow : Rogan {
    RoganSmallYellow() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Rogan1PSYellowSmall.svg")));
    }
};

struct RoganMedWhite : Rogan {
    RoganMedWhite() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Rogan1PSWhiteMed.svg")));
    }
};

struct RoganMedSmallWhite : Rogan {
    RoganMedSmallWhite() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Rogan1PSWhiteMedSmall.svg")));
    }
};

struct RoganSmallWhite : Rogan {
    RoganSmallWhite() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Rogan1PSWhiteSmall.svg")));
    }
};

struct RoganMedGreen : Rogan {
    RoganMedGreen() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Rogan1PSGreenMed.svg")));
    }
};

struct RoganSmallGreen : Rogan {
    RoganSmallGreen() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Rogan1PSGreenSmall.svg")));
    }
};

struct RoganSmallGreenSnap : Rogan {
    RoganSmallGreenSnap() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Rogan1PSGreenSmall.svg")));
        snap = true;
    }
};


struct RoganMedBlue : Rogan {
    RoganMedBlue() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Rogan1PSBlueMed.svg")));
    }
};

struct RoganMedBlueSnap : Rogan {
    RoganMedBlueSnap() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Rogan1PSBlueMed.svg")));
        snap = true;
    }
};

struct RoganSmallBlue : Rogan {
    RoganSmallBlue() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Rogan1PSBlueSmall.svg")));
    }
};

struct RoganSmallBlueSnap : Rogan {
    RoganSmallBlueSnap() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Rogan1PSBlueSmall.svg")));
        snap = true;
    }
};


struct RoganMedRed : Rogan {
    RoganMedRed() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Rogan1PSRedMed.svg")));
    }
};

struct RoganSmallRed : Rogan {
    RoganSmallRed() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Rogan1PSRedSmall.svg")));
    }
};

struct Rogan1PSPurple : Rogan {
    Rogan1PSPurple() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Rogan1PSPurple.svg")));
    }
};

struct RoganMedPurple : Rogan {
    RoganMedPurple() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Rogan1PSPurpleMed.svg")));
    }
};

struct RoganSmallPurple : Rogan {
    RoganSmallPurple() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Rogan1PSPurpleSmall.svg")));
    }
};

struct Rogan1PSMustard : Rogan {
    Rogan1PSMustard() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Rogan1PSMustard.svg")));
    }
};

struct RoganSmallMustard : Rogan {
    RoganSmallMustard() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Rogan1PSMustardSmall.svg")));
    }
};

struct Rogan1PSOrange : Rogan {
    Rogan1PSOrange() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Rogan1PSOrange.svg")));
    }
};

struct RoganMedOrange : Rogan {
    RoganMedOrange() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Rogan1PSOrangeMed.svg")));
    }
};

struct RoganSmallOrange : Rogan {
    RoganSmallOrange() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Rogan1PSOrangeSmall.svg")));
    }
};

template <typename BASE>
struct MuteLight : BASE {
	MuteLight() {
		this->box.size = mm2px(Vec(6.0f, 6.0f));
	}
};



// General routines

struct RefreshCounter {
	// Note: because of stagger, and asyncronous dataFromJson, should not assume this processInputs() will return true on first run
	// of module::process()
	static const unsigned int displayRefreshStepSkips = 256;
	static const unsigned int userInputsStepSkipMask = 0xF;// sub interval of displayRefreshStepSkips, since inputs should be more responsive than lights
	// above value should make it such that inputs are sampled > 1kHz so as to not miss 1ms triggers

	unsigned int refreshCounter = (random::u32() % displayRefreshStepSkips);// stagger start values to avoid processing peaks when many Geo and Impromptu modules in the patch

	bool processInputs() {
		return ((refreshCounter & userInputsStepSkipMask) == 0);
	}
	bool processLights() {// this must be called even if module has no lights, since counter is decremented here
		refreshCounter++;
		bool process = refreshCounter >= displayRefreshStepSkips;
		if (process) {
			refreshCounter = 0;
		}
		return process;
	}
};


struct Trigger : dsp::SchmittTrigger {
	// implements a 0.1V - 1.0V dsp::SchmittTrigger (see include/dsp/digital.hpp) instead of
	//   calling SchmittTriggerInstance.process(math::rescale(in, 0.1f, 1.f, 0.f, 1.f))
	bool process(float in) {
		if (state) {
			// HIGH to LOW
			if (in <= 0.1f) {
				state = false;
			}
		}
		else {
			// LOW to HIGH
			if (in >= 1.0f) {
				state = true;
				return true;
			}
		}
		return false;
	}
};

struct TriggerRiseFall {
	bool state = false;

	void reset() {
		state = false;
	}

	int process(float in) {
		if (state) {
			// HIGH to LOW
			if (in <= 0.1f) {
				state = false;
				return -1;
			}
		}
		else {
			// LOW to HIGH
			if (in >= 1.0f) {
				state = true;
				return 1;
			}
		}
		return 0;
	}
};

NVGcolor prepareDisplay(NVGcontext *vg, Rect *box, int fontSize);
static const int displayAlpha = 23;
