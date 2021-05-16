#include "JLmod.hpp"
#include "plugin.hpp"


struct Fourious : Module {
	enum ParamIds {
		PARAM1,
		NUM_PARAMS
	};
	enum InputIds {
		CV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
	    OUT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	float phase = 0.0;
	float blinkPhase = 0.0;

	Fourious() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	}

    void process(const ProcessArgs& args) {
        //float deltaTime = engineGetSampleTime();
        float deltaTime = args.sampleTime;

        // Compute the frequency from the pitch parameter and input
        float pitch = params[PARAM1].value;
        pitch += inputs[CV_INPUT].value;
        pitch = clamp(pitch, -4.0f, 4.0f);
        // The default pitch is C4
        float freq = 261.626f * powf(2.0f, pitch);

        // Accumulate the phase
        phase += freq * deltaTime;
        if (phase >= 1.0f)
            phase -= 1.0f;

        // Compute the sine output
        float harmonic1 = sinf(2.0f * M_PI * phase);
        float harmonic3 = sinf(3.0f * 2.0f * M_PI * phase)/3;
        float harmonic5 = sinf(5.0f * 2.0f * M_PI * phase)/5;
        float harmonic7 = sinf(7.0f * 2.0f * M_PI * phase)/7;
        float harmonic9 = sinf(9.0f * 2.0f * M_PI * phase)/9;
        float harmonic11 = sinf(11.0f * 2.0f * M_PI * phase)/11;
        outputs[OUT_OUTPUT].value = 5.0f * (harmonic1+harmonic3+harmonic5+harmonic7+harmonic9+harmonic11);

        // Blink light at 1Hz
        blinkPhase += deltaTime;
        if (blinkPhase >= 1.0f)
            blinkPhase -= 1.0f;
        //lights[BLINK_LIGHT].value = (blinkPhase < 0.5f) ? 1.0f : 0.0f;

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

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.935, 22.116)), module, Fourious::CV_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(Vec(20.0, 283), module, Fourious::OUT_OUTPUT));
	}
};


Model* modelFourious = createModel<Fourious, FouriousWidget>("Fourious");