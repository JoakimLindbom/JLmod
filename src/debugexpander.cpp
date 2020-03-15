//***********************************************************************************************
// Debugger expander module
// Used to get relevant status and data from the module being debugged
//***********************************************************************************************

#include "JLmod.hpp"
#include "common.hpp"

#define NUM_PORTS 8

int message_BPM;
int current_seq_step;

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
    float leftMessages[2][26] = {};// messages from main module
    /*char text_1_dft[10] = "___";
    char *text_1;
    char *ptr;
*/
    //int panelTheme;
    unsigned int expanderRefreshCounter = 0;
    static const unsigned int expanderRefreshStepSkips = 64;

    DebugExpander() {
        config(0, 0, NUM_OUTPUTS, NUM_LIGHTS);

        //text_1 = text_1_dft; //TODO: Is this safe?

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
                    lights[LIGHTS+i].setBrightness(message[i]);        // 0-7 Clocks - LEDs
                    outputs[CLOCK_OUTPUTS+i].setVoltage(message[i+9]); // 9-16 Clocks - Gates
                    outputs[STEP_OUTPUTS+i].setVoltage(message[i+17]); // 17-24 Sequencer Steps
                }
                message_BPM = (int)message[8];
                current_seq_step = (int)message[25] + 1;
                //std::cout << "msg raw: " << message[8] << "message_BPM: " << message_BPM << "\n";
/*
                ptr = message[26];
                if (ptr != nullptr) {
                    text_1 = ptr;
                }
                std::cout << "text_1" << text_1 << "\n";
*/
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
		int *display_msg;;

		BpmDisplayWidget(int *msg) {
			display_msg = msg;
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
				    //snprintf(displayStr, 4, "%3u", (unsigned)message_BPM);
				    snprintf(displayStr, 4, "%3u", (unsigned)*display_msg);
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
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        static const int BaseY = 125;
        static const int RowY = 29;
        static const int BaseX = 15;

        addChild(createLight<MediumLight<GreenLight>>(Vec(mm2px(4), mm2px(7.5)), module, DebugExpander::CONNECTED_LIGHT));

        // BPM display
        BpmDisplayWidget *bpmDisplay = new BpmDisplayWidget(&message_BPM);
        bpmDisplay->box.size = Vec(55, 30);// 3 characters
        bpmDisplay->box.pos = Vec(mm2px(19), mm2px(11));
        bpmDisplay->module = module;
        addChild(bpmDisplay);

        // Second display
        BpmDisplayWidget *Display2 = new BpmDisplayWidget(&current_seq_step);
        Display2->box.size = Vec(55, 30);// 3 characters
        Display2->box.pos = Vec(mm2px(19), mm2px(25));
        Display2->module = module;
        addChild(Display2);

        for (int i=0; i<NUM_PORTS; i++) {
            addOutput(createOutput<PJ301MPort>(Vec(BaseX, BaseY + i * RowY), module, DebugExpander::CLOCK_OUTPUTS+i));
            addChild(createLight<MediumLight<RedLight>>(Vec(BaseX+32, BaseY + 7 + i * RowY), module, DebugExpander::LIGHTS+i));
            addOutput(createOutput<PJ301MPort>(Vec(BaseX+62, BaseY + i * RowY), module, DebugExpander::STEP_OUTPUTS+i));
        }

	}

	void step() override {
		Widget::step();
	}
};

Model *modelDebugExpander = createModel<DebugExpander, DebugExpanderWidget>("DebugExpander");
