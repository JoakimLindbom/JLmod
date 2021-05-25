#include "JLmod.hpp"
#include "common.hpp"
#include "plugin.hpp"

#define HARMONICS 6

struct Fourious : Module {
	enum ParamIds {
		PITCH,
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

        configParam(Fourious::PITCH, 0.0f, 11.0f, 10.0f, "Pitch");
        configParam(Fourious::CAP, 0.0f, 11.0f, 10.0f, "Cap level");

		for (int i=0; i<HARMONICS; i++) {
            configParam(Fourious::HARMONIC_AMP + i, -2.0f, 2.0f, 0.0f, "Amplification level");
            configParam(Fourious::HARMONIC_INPUT_AMP + i, 0.0f, 2.0f, 0.0f, "External input amplification level");
         }
	}

    void process(const ProcessArgs& args) {
        //float deltaTime = engineGetSampleTime();
        float deltaTime = args.sampleTime;

        // Compute the frequency from the pitch parameter and input
        float pitch = params[PITCH].value;
        pitch += inputs[CV_INPUT].value;
        pitch = clamp(pitch, -4.0f, 4.0f);
        // The default pitch is C4
        float freq = 261.626f * powf(2.0f, pitch);

        // Accumulate the phase
        phase += freq * deltaTime;
        if (phase >= 1.0f)
            phase -= 1.0f;

        // Compute the sine output
        // TODO: Fix 1st harmonics
        float harmonic1  = (1.0f+params[HARMONIC_AMP+0].getValue()) * sinf(2.0f * M_PI * phase);
        float harmonic1a = harmonic1;
        if (inputs[HARMONIC_INPUT].isConnected()){
              harmonic1a = harmonic1 * params[HARMONIC_INPUT_AMP+0].value * inputs[HARMONIC_INPUT].value/4.0f;
        }
        //float harmonic1b = harmonic1a * sinf(2.0f * M_PI * phase);
        harmonic1 = harmonic1a;

        //harmonic1  = ((1.0f+params[HARMONIC_AMP+0].getValue())  * inputs[HARMONIC_INPUT].isConnected()?inputs[HARMONIC_INPUT].value/4.0f:1.0f) * sinf(2.0f * M_PI * phase);
        //float harmonic1  = (1.0+params[HARMONIC_AMP+0].getValue())  * sinf(2.0f * M_PI * phase);
        //float harmonic1  = sinf(2.0f * M_PI * phase);
        //std::cout << "value: " << (1.0+params[HARMONIC_AMP+0].getValue()) << "\n";
        //float harmonic1 = (1.0+params[HARMONIC_AMP+0].value) * sinf(2.0f * M_PI * phase);
        float harmonicx = 0.0f;
        float harmonicn = 0.0f;
        for (int i=1;i<=11;i+=2) {
            harmonicn = (1.0+params[HARMONIC_AMP+(i-1)/2].getValue())*sinf(i * 2.0f * M_PI * phase)/i;
            if (inputs[HARMONIC_INPUT+(i-1)/2].isConnected()){
                  float harmonicx1 = params[HARMONIC_INPUT_AMP+((i-1)/2)].value * inputs[HARMONIC_INPUT+((i-1)/2)].value/4.0f;
                  harmonicn += harmonicx1;
            }
            harmonicx += harmonicn;
        }

        /*float harmonic3 = (1.0+params[HARMONIC_AMP+1].getValue())*sinf(3.0f * 2.0f * M_PI * phase)/3;
        float harmonic5 = (1.0+params[HARMONIC_AMP+2].getValue())*sinf(5.0f * 2.0f * M_PI * phase)/5;
        float harmonic7 = (1.0+params[HARMONIC_AMP+3].getValue())*sinf(7.0f * 2.0f * M_PI * phase)/7;
        float harmonic9 = (1.0+params[HARMONIC_AMP+4].getValue())*sinf(9.0f * 2.0f * M_PI * phase)/9;
        float harmonic11 = (1.0+params[HARMONIC_AMP+5].getValue())*sinf(11.0f * 2.0f * M_PI * phase)/11;
        */
        //outputs[OUT_OUTPUT].value = clamp(5.0f * (harmonic1+harmonic3+harmonic5+harmonic7+harmonic9+harmonic11), -params[CAP].value, params[CAP].value);
        outputs[OUT_OUTPUT].value = clamp(5.0f * harmonicx, -params[CAP].value, params[CAP].value);
        //outputs[OUT_OUTPUT].value = 5.0f * (harmonic1+harmonic3+harmonic5+harmonic7+harmonic9+harmonic11);

        if (stride_counter++ > 5000) {
            std::cout.precision(7);
            //std::cout << "1: " << harmonic1 << "\ta:" << harmonic1a << "\tb:" << harmonic1b << "\n";
            //std::cout << "1: " << harmonicx << " 2: " << harmonic1+harmonic3+harmonic5+harmonic7+harmonic9+harmonic11 << "\n";
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

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.935, 22.116)), module, Fourious::CV_INPUT));
        addParam(createParam<RoganSmallGreen>(Vec(20.0 ,20.0), module, Fourious::PITCH));
        addParam(createParam<RoganSmallGreen>(Vec(40.0 ,20.0), module, Fourious::CAP));

		static constexpr float Row_1 = 106.0;
		static constexpr float Matrix_Y_spacing = 35.0f;

		for (int i = 0; i < HARMONICS; i++) {
		    addInput(createInputCentered<PJ301MPort>(Vec(26.0 , Row_1 + 10.0 + i * Matrix_Y_spacing), module, Fourious::HARMONIC_INPUT + i));
            addParam(createParam<RoganSmallGreen>(Vec(74.0 , Row_1 + i * Matrix_Y_spacing), module, Fourious::HARMONIC_INPUT_AMP + i));
            addParam(createParam<RoganSmallGreen>(Vec(100.0 , Row_1 + i * Matrix_Y_spacing), module, Fourious::HARMONIC_AMP + i));
        }

        addOutput(createOutputCentered<PJ301MPort>(Vec(90.0, 335.0), module, Fourious::OUT_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(130.0, 335.0), module, Fourious::OUT2_OUTPUT));
	}
};


Model* modelFourious = createModel<Fourious, FouriousWidget>("Fourious");