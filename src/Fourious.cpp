#include "JLmod.hpp"
#include "common.hpp"
#include "plugin.hpp"

#define HARMONICS 6

struct Fourious : Module {
	enum ParamIds {
		OCTAVE,
		CAP,
		AMP,
		ENUMS(HARMONIC_AMP, HARMONICS),         // Amplification level for each odd harmonics
		ENUMS(HARMONIC_INPUT_AMP, HARMONICS),  // Amplification level for each odd harmonics external input
		NUM_PARAMS
	};
	enum InputIds {
		CV_INPUT,
		ENUMS(HARMONIC_INPUT, HARMONICS),
		NUM_INPUTS
	};
	enum OutputIds {
	    OUT_OUTPUT,
	    OUT2_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
	    BLINK_LIGHT,
		NUM_LIGHTS
	};

    float damping = 0.0f;

	int stride_counter = 0;

	float phase = 0.0;
	float blinkPhase = 0.0;

	Fourious() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(Fourious::OCTAVE, -5.0f, 5.0f, 0.0f, "Octave"); // Range? -4 -- +4 ??
        configParam(Fourious::CAP, 0.0f, 11.0f, 10.0f, "Cap level");

		for (int i=0; i<HARMONICS; i++) {
            configParam(Fourious::HARMONIC_AMP + i, -2.0f, 2.0f, 0.0f, "Amplification level");
            configParam(Fourious::HARMONIC_INPUT_AMP + i, 0.0f, 2.0f, 0.0f, "External input amplification level");
         }
       configInput(CV_INPUT, "1V/oct pitch");
	}

    void process(const ProcessArgs& args) {
        //float deltaTime = engineGetSampleTime();
        float deltaTime = args.sampleTime;

        // Compute the frequency from the pitch parameter and input
        float pitch = inputs[CV_INPUT].value + params[OCTAVE].value;  // Base + Octave offset (-5.0 -- +5.0)
        pitch = clamp(pitch, -4.0f, 4.0f);
        // Default pitch (C4)
        float freq = 261.626f * powf(2.0f, pitch);

        // Accumulate the phase
        phase += freq * deltaTime;
        if (phase >= 1.0f)
            phase -= 1.0f;

        // Compute the 6 sine waves and add them
        float wave1_sum = 0.0f; // The summed amplitude of wave1
        float harmonicn = 0.0f; //
        for (int i=1;i<=11;i+=2) {
            harmonicn = (1.0+params[HARMONIC_AMP+(i-1)/2].getValue())*sinf(i * 2.0f * M_PI * phase)/i;
            if (inputs[HARMONIC_INPUT+(i-1)/2].isConnected()){
                  float onset = harmonicn * params[HARMONIC_INPUT_AMP+((i-1)/2)].value * inputs[HARMONIC_INPUT+((i-1)/2)].value/(4.0f); // *i
                  harmonicn += onset;
            }
            wave1_sum += harmonicn;
        }
        outputs[OUT_OUTPUT].value = clamp(5.0f * wave1_sum, -params[CAP].value, params[CAP].value);

        if (stride_counter++ > 5000) {
            std::cout.precision(7);
            stride_counter = 0;
        }

        // OUT2 calculations
        //damping +=
        //sinf(2.0f * M_PI * phase *

        // Blink light at 1Hz
        blinkPhase += deltaTime;
        if (blinkPhase >= 1.0f)
            blinkPhase -= 1.0f;
        lights[BLINK_LIGHT].value = (blinkPhase < 0.5f) ? 1.0f : 0.0f;
    }
};
struct FouriousWidget : ModuleWidget {
	FouriousWidget(Fourious* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Fourious.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(10.0, 15.0)), module, Fourious::BLINK_LIGHT));

		addInput(createInputCentered<PJ301MPort>(Vec(29, 50), module, Fourious::CV_INPUT));
        addParam(createParam<RoganSmallGreenSnap>(Vec(75.0 ,35.0), module, Fourious::OCTAVE));
        addParam(createParam<RoganSmallGreen>(Vec(120.0 ,35.0), module, Fourious::CAP));

		static constexpr float Row_1 = 107.0;
		static constexpr float Matrix_Y_spacing = 35.0f;

		for (int i = 0; i < HARMONICS; i++) {
		    addInput(createInputCentered<PJ301MPort>(Vec(29.0 , Row_1 + 10.0 + i * Matrix_Y_spacing), module, Fourious::HARMONIC_INPUT + i));
            addParam(createParam<RoganSmallGreen>(Vec(75.0 , Row_1 + i * Matrix_Y_spacing), module, Fourious::HARMONIC_INPUT_AMP + i));
            addParam(createParam<RoganSmallGreen>(Vec(120.0 , Row_1 + i * Matrix_Y_spacing), module, Fourious::HARMONIC_AMP + i));
        }

        addOutput(createOutputCentered<PJ301MPort>(Vec(90.0, 335.0), module, Fourious::OUT_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(130.0, 335.0), module, Fourious::OUT2_OUTPUT));
	}
};


Model* modelFourious = createModel<Fourious, FouriousWidget>("Fourious");