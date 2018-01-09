#include "JLmod.hpp"
#include "dsp/digital.hpp"

#include "common.hpp"
#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>
#include "util.hpp"

/*
Thanks to Strum for the display widget!
*/

struct Timer {
    float seconds = 0.0f;
    float counter = 0.0f;
    SchmittTrigger resetTrigger;
    Timer() {
        resetTrigger.setThresholds(0.0, 0.01);
    }
    void setTime(float in_seconds) {
        seconds = in_seconds;
    }
    void setReset(float reset) {
        if (resetTrigger.process(reset)) {
            counter = seconds;  //
        }
    }
    void step(float dt) {
        counter -= dt;
    }

    float light() {
        return counter<=0?5:0;  // TODO: Check expected level
    }
};


struct Timers: Module {
    enum ParamIds {
        timer1_dsp_PARAM,
        timer2_dsp_PARAM,
        timer3_dsp_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        RESET1_INPUT,
        CV1_INPUT,
        RESET2_INPUT,
        CV2_INPUT,
        RESET3_INPUT,
        CV3_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        TRIGGER1_OUTPUT,
        TRIGGER2_OUTPUT,
        TRIGGER3_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        LIGHT1,
        LIGHT2,
        LIGHT3,
        NUM_LIGHTS
    };

    Timer timer1;
    Timer timer2;
    Timer timer3;

    float timer1_dsp_val;
    float timer2_dsp_val;
    float timer3_dsp_val;
    float counter1_val;
    float counter2_val;
    float counter3_val;

    Timers() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
    void step() override;
};

struct SmallIntegerDisplayWidgeter : TransparentWidget {

    float *value;
    std::shared_ptr<Font> font;

    SmallIntegerDisplayWidgeter() {
        font = Font::load(assetPlugin(plugin, "res/Segment7Standard.ttf"));
    };

    void draw(NVGcontext *vg) override
    {
        // Background
        NVGcolor backgroundColor = nvgRGB(0xC0, 0xC0, 0xC0);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, 0.0, 0.0, box.size.x, box.size.y, 4.0);
        nvgFillColor(vg, backgroundColor);
        nvgFill(vg);

        // text
        nvgFontSize(vg, 16);
        nvgFontFaceId(vg, font->handle);
        nvgTextLetterSpacing(vg, 0.5);

        std::stringstream to_display;
        to_display = format4display(*value);

        Vec textPos = Vec(8.0f, 23.0f);
        NVGcolor textColor = nvgRGB(0x00, 0x00, 0x00);
        nvgFillColor(vg, textColor);
        nvgText(vg, textPos.x, textPos.y, to_display.str().substr(0, 4).c_str(), NULL);
    }
};

void Timers::step() {

    timer1_dsp_val = params[timer1_dsp_PARAM].value;
    outputs[TRIGGER1_OUTPUT].value = 0.0f;

    timer1.step(1.0 / engineGetSampleRate());
    counter1_val = timer1.counter;

    timer2_dsp_val = params[timer2_dsp_PARAM].value;
    outputs[CV2_INPUT].value = 0.0f;

    counter2_val = 4.0f;  // TODO: implement counter

    timer3_dsp_val = params[timer3_dsp_PARAM].value;
    outputs[CV3_INPUT].value = 0.0f;

    counter3_val = 6.0f;  // TODO: implement counter

}

TimersWidget::TimersWidget() {
    Timers *module = new Timers();
    setModule(module);
    box.size = Vec(15*10, 380);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(plugin, "res/Timers.svg")));
        addChild(panel);
    }

    addChild(createScrew<ScrewSilver>(Vec(15, 0)));
    addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(createScrew<ScrewSilver>(Vec(15, 365)));
    addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

    // Timer 1
    SmallIntegerDisplayWidgeter *timer1_dsp = new SmallIntegerDisplayWidgeter();
    timer1_dsp->box.pos = Vec(23, 60);
    timer1_dsp->box.size = Vec(50, 30);
    timer1_dsp->value = &module->timer1_dsp_val;
    addChild(timer1_dsp);

    SmallIntegerDisplayWidgeter *counter = new SmallIntegerDisplayWidgeter();
    counter->box.pos = Vec(83, 60);
    counter->box.size = Vec(50, 30);
    counter->value = &module->counter1_val;
    addChild(counter);

    addInput(createInput<PJ301MPort>(Vec(8, 123), module, Timers::RESET1_INPUT));
    addInput(createInput<PJ301MPort>(Vec(33, 123), module, Timers::CV1_INPUT));
    addParam(createParam<RoundBlackKnob>(Vec(58, 95), module, Timers::timer1_dsp_PARAM, 0.0, 1000.0, 0.0));
    addOutput(createOutput<PJ301MPort>(Vec(113, 123), module, Timers::TRIGGER1_OUTPUT));

    // Timer 2
    SmallIntegerDisplayWidgeter *timer2_dsp = new SmallIntegerDisplayWidgeter();
    timer2_dsp->box.pos = Vec(23, 160);
    timer2_dsp->box.size = Vec(50, 30);
    timer2_dsp->value = &module->timer2_dsp_val;
    addChild(timer2_dsp);

    SmallIntegerDisplayWidgeter *counter2 = new SmallIntegerDisplayWidgeter();
    counter2->box.pos = Vec(83, 160);
    counter2->box.size = Vec(50, 30);
    counter2->value = &module->counter2_val;
    addChild(counter2);

    addInput(createInput<PJ301MPort>(Vec(35, 223), module, Timers::RESET2_INPUT));
    addInput(createInput<PJ301MPort>(Vec(33, 223), module, Timers::CV2_INPUT));
    addParam(createParam<RoundBlackKnob>(Vec(58, 195), module, Timers::timer2_dsp_PARAM, 0.0, 1000.0, 0.0));
    addOutput(createOutput<PJ301MPort>(Vec(95, 223), module, Timers::TRIGGER2_OUTPUT));

    // Timer 3
    SmallIntegerDisplayWidgeter *timer3_dsp = new SmallIntegerDisplayWidgeter();
    timer3_dsp->box.pos = Vec(23, 260);
    timer3_dsp->box.size = Vec(50, 30);
    timer3_dsp->value = &module->timer3_dsp_val;
    addChild(timer3_dsp);

    SmallIntegerDisplayWidgeter *counter3 = new SmallIntegerDisplayWidgeter();
    counter3->box.pos = Vec(83, 260);
    counter3->box.size = Vec(50, 30);
    counter3->value = &module->counter3_val;
    addChild(counter3);

    addInput(createInput<PJ301MPort>(Vec(35, 323), module, Timers::RESET3_INPUT));
    addInput(createInput<PJ301MPort>(Vec(33, 323), module, Timers::CV3_INPUT));
    addParam(createParam<RoundBlackKnob>(Vec(58, 295), module, Timers::timer3_dsp_PARAM, 0.0, 1000.0, 0.0));
    addOutput(createOutput<PJ301MPort>(Vec(95, 323), module, Timers::TRIGGER3_OUTPUT));

}
