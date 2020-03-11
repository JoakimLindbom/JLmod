//***********************************************************************************************
// Debugger expander module
// Used to get relevant status and data from the module being debugged
//***********************************************************************************************

#include "JLmod.hpp"
#include "common.hpp"

#define NUM_PORTS 8

int message_BPM;

struct DebugExpander : Module {
	enum OutputIds {
		ENUMS(CLOCK_OUTPUTS, NUM_PORTS),
		ENUMS(STEP_OUTPUTS, NUM_PORTS),
		NUM_OUTPUTS
	};
	enum LightIds {
		CONNECTED_LIGHT,
		ENUMS(LIGHTS, NUM_PORTS),
		NUM_LIGHTS
	};

	// Message interchange
	float leftMessages[2][25] = {};// messages from main module

	//int panelTheme;
	unsigned int expanderRefreshCounter = 0;
    static const unsigned int expanderRefreshStepSkips = 64;

	DebugExpander() {
		config(0, 0, NUM_OUTPUTS, NUM_LIGHTS);

		leftExpander.producerMessage = leftMessages[0];
		leftExpander.consumerMessage = leftMessages[1];
	}

	void process(const ProcessArgs &args) override {

		expanderRefreshCounter++;
		if (expanderRefreshCounter >= expanderRefreshStepSkips) {
			expanderRefreshCounter = 0;

			bool motherPresent = (leftExpander.module && leftExpander.module->model == modelRatchets); // TODO: Add more models
			if (motherPresent) {
                lights[CONNECTED_LIGHT].setSmoothBrightness(1.f,0.3f);
                // Get consumer message
                float *message = (float*) leftExpander.consumerMessage;
                for (int i = 0; i < 8; i++) {
                    lights[LIGHTS+i].setBrightness(message[i]);        // Clocks - LEDs
                    outputs[CLOCK_OUTPUTS+i].setVoltage(message[i+9]); // Clocks - Gates
                    outputs[STEP_OUTPUTS+i].setVoltage(message[i+17]); // Sequencer Steps
                }
                message_BPM = (int)message[8];
            }
            else {
                // No mother module is connected.
                lights[CONNECTED_LIGHT].setSmoothBrightness(0.f,0.3f);
                message_BPM = -1;
            }
		}// expanderRefreshCounter
	}// process()
};


struct DebugExpanderWidget : ModuleWidget {
	SvgPanel* darkPanel;

	struct BpmDisplayWidget : TransparentWidget {
		DebugExpander *module;
		std::shared_ptr<Font> font;
		char displayStr[4] = "---";

		BpmDisplayWidget() {
			font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/Segment14.ttf"));
		}

		void draw(const DrawArgs &args) override {
			NVGcolor textColor = prepareDisplay(args.vg, &box, 18);
			nvgFontFaceId(args.vg, font->handle);
			//nvgTextLetterSpacing(args.vg, 2.5);

			Vec textPos = Vec(6, 24);
			nvgFillColor(args.vg, nvgTransRGBA(textColor, displayAlpha));
			nvgText(args.vg, textPos.x, textPos.y, "~~~", NULL);
			nvgFillColor(args.vg, textColor);
			if (module != NULL && message_BPM != -1) {
				    snprintf(displayStr, 4, "%3u", (unsigned)message_BPM);
			} else {
			  //    snprintf(displayStr, 4, "---");
			}
			displayStr[3] = 0;// more safety
			nvgText(args.vg, textPos.x, textPos.y, displayStr, NULL);
		}
	};

	DebugExpanderWidget(DebugExpander *module) {
		setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Debugger.svg")));

		// Screws
		//addChild(createDynamicWidget<IMScrew>(Vec(box.size.x-30, 0), module ? &module->panelTheme : NULL));
		//addChild(createDynamicWidget<IMScrew>(Vec(box.size.x-30, 365), module ? &module->panelTheme : NULL));

		// Expansion module
		static const int BaseY = 100;
		static const int RowPosY = 30;
		static const int BaseX = 13;

        addChild(createLight<MediumLight<GreenLight>>(Vec(BaseX+3, 12), module, DebugExpander::CONNECTED_LIGHT));

        for (int i=0; i<NUM_PORTS; i++) {
            addOutput(createOutput<PJ301MPort>(Vec(BaseX, BaseY + i * RowPosY), module, DebugExpander::CLOCK_OUTPUTS+i));
            addChild(createLight<MediumLight<RedLight>>(Vec(BaseX+35, BaseY + 7 + i * RowPosY), module, DebugExpander::LIGHTS+i));
            addOutput(createOutput<PJ301MPort>(Vec(BaseX+73, BaseY + i * RowPosY), module, DebugExpander::STEP_OUTPUTS+i));
        }

        // BPM display
		BpmDisplayWidget *bpmDisplay = new BpmDisplayWidget();
		bpmDisplay->box.size = Vec(55, 30);// 3 characters
		bpmDisplay->box.pos = Vec((175) / 2 - bpmDisplay->box.size.x / 2 - 8, 48 - bpmDisplay->box.size.y / 2);
		bpmDisplay->module = module;
		addChild(bpmDisplay);

	}

	void step() override {
		Widget::step();
	}
};

Model *modelDebugExpander = createModel<DebugExpander, DebugExpanderWidget>("DebugExpander");
