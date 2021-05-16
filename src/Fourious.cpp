#include "JLmod.hpp"
#include "common.hpp"
#include "plugin.hpp"

#define HARMONICS 6

struct Fourious : Module {
	enum ParamIds {
		PARAM1,
		PARAM2,
		ENUMS(HARMONIC_AMP, HARMONICS), // Amplification level for each odd harmonics
		NUM_PARAMS
	};
	enum InputIds {
		CV_INPUT,
		ENUMS(HARMONIC_AMP_INPUT, HARMONICS),
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

        configParam(Fourious::PARAM1, 0.0f, 11.0f, 10.0f, "Pitch");
        configParam(Fourious::PARAM2, 0.0f, 11.0f, 10.0f, "Cap level");

		for (int i=0; i<HARMONICS; i++) {
            configParam(Fourious::HARMONIC_AMP + i, -2.0f, 2.0f, 0.0f, "Amplification level");
         }

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
        float harmonic1 = (1.0+params[HARMONIC_AMP+0].getValue()+inputs[HARMONIC_AMP_INPUT].isConnected()?inputs[HARMONIC_AMP_INPUT].value:0.0)*sinf(2.0f * M_PI * phase);
        //float harmonic1 = (1.0+params[HARMONIC_AMP+0].getValue())*sinf(2.0f * M_PI * phase);
        float harmonic3 = (1.0+params[HARMONIC_AMP+1].getValue())*sinf(3.0f * 2.0f * M_PI * phase)/3;
        float harmonic5 = (1.0+params[HARMONIC_AMP+2].getValue())*sinf(5.0f * 2.0f * M_PI * phase)/5;
        float harmonic7 = (1.0+params[HARMONIC_AMP+3].getValue())*sinf(7.0f * 2.0f * M_PI * phase)/7;
        float harmonic9 = (1.0+params[HARMONIC_AMP+4].getValue())*sinf(9.0f * 2.0f * M_PI * phase)/9;
        float harmonic11 = (1.0+params[HARMONIC_AMP+5].getValue())*sinf(11.0f * 2.0f * M_PI * phase)/11;
        outputs[OUT_OUTPUT].value = clamp(5.0f * (harmonic1+harmonic3+harmonic5+harmonic7+harmonic9+harmonic11), -params[PARAM2].value, params[PARAM2].value);
        //outputs[OUT_OUTPUT].value = 5.0f * (harmonic1+harmonic3+harmonic5+harmonic7+harmonic9+harmonic11);

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
        addParam(createParam<RoganSmallGreen>(Vec(20.0 ,20.0), module, Fourious::PARAM1));
        addParam(createParam<RoganSmallGreen>(Vec(40.0 ,20.0), module, Fourious::PARAM2));

		static constexpr float Matrix_Y_spacing = 28.0f;
        static constexpr float Row_1 = 91.0;

		for (int i = 0; i < HARMONICS; i++) {
		    addInput(createInputCentered<PJ301MPort>(Vec(20.0 , Row_1 + i * Matrix_Y_spacing+3.0), module, Fourious::HARMONIC_AMP_INPUT + i));
            addParam(createParam<RoganSmallGreen>(Vec(40.0 , Row_1 + i * Matrix_Y_spacing+3.0), module, Fourious::HARMONIC_AMP + i));
        }

        addOutput(createOutputCentered<PJ301MPort>(Vec(20.0, 283.0), module, Fourious::OUT_OUTPUT));



	}
};


Model* modelFourious = createModel<Fourious, FouriousWidget>("Fourious");