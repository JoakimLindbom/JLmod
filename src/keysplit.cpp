#include "JLmod.hpp"
#include "common.hpp"

struct KeySplit : Module {
	enum ParamIds {
		KNOB_SP,
		NUM_PARAMS
	};
	enum InputIds {
		SP_INPUT,
		CV_INPUT,
		GATE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		CV_RIGHT_OUTPUT,
		GATE_RIGHT_OUTPUT,
		CV_LEFT_OUTPUT,
		GATE_LEFT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

    dsp::SchmittTrigger gateTrigger;
    float gateLeftTime = 0.f;
    float gateRightTime = 0.f;
    bool gateLeft = false;
    bool gateRight = false;

	KeySplit() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(KNOB_SP, 0.f, 10.f, 0.001f, ""); // Full 10 octaves TODO: Add 32, 48, 88 keys etc. templates to reduce scope
	}

	void process(const ProcessArgs& args) override;
};

void KeySplit::process(const ProcessArgs& args) {
    float _inCV;
    if (inputs[CV_INPUT].isConnected() && inputs[GATE_INPUT].isConnected()) {
        if (gateTrigger.process(inputs[GATE_INPUT].getVoltage())) {
            _inCV = inputs[CV_INPUT].getVoltage();

            if (_inCV<params[KNOB_SP].getValue()) {
                gateLeftTime = 1e-3f; // 1 ms
                gateLeft = true;
                outputs[CV_LEFT_OUTPUT].setVoltage(_inCV);
            } else {
                gateRightTime = 1e-3f; // 1 ms
                gateRight = true;
                outputs[CV_RIGHT_OUTPUT].setVoltage(_inCV);
            }
        }
    }

    if (gateLeftTime > 0.f) {
        outputs[GATE_LEFT_OUTPUT].setVoltage(10.0f);
        gateLeftTime -= args.sampleTime;
    } else {
        outputs[GATE_LEFT_OUTPUT].setVoltage(0.f);
    }

    if (gateRightTime > 0.f) {
        outputs[GATE_RIGHT_OUTPUT].setVoltage(10.0f);
        gateRightTime -= args.sampleTime;
    } else {
        outputs[GATE_RIGHT_OUTPUT].setVoltage(0.0f);
    }

}

struct KeySplitWidget : ModuleWidget {
	KeySplitWidget(KeySplit* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/KeySplit.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoganSmallBlue>(mm2px(Vec(29.244, 35.077)), module, KeySplit::KNOB_SP));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.935, 22.116)), module, KeySplit::CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(29.244, 22.116)), module, KeySplit::SP_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.935, 37.194)), module, KeySplit::GATE_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(9.406, 97.909)), module, KeySplit::CV_LEFT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30.7, 97.909)), module, KeySplit::CV_RIGHT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30.7, 113.31)), module, KeySplit::GATE_RIGHT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(9.406, 113.311)), module, KeySplit::GATE_LEFT_OUTPUT));
	}
};


Model* modelKeySplit = createModel<KeySplit, KeySplitWidget>("KeySplit");