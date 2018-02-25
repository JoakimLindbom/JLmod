#include "JLmod.hpp"
#include "dsp/digital.hpp"

#include "common.hpp"
#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>
//#include "util/logger.hpp"

/*
Thanks to Strum for the display widget!
*/

struct Timer {
    bool running = false;
    float seconds = 0.0f;
    float counter = 20.0f;
    int Stride = 1;
    float SampleRate = 0.000015f;
    float TimeStep = 0.0f;
    bool trigger = false;
    float output = 0.0f;

    SchmittTrigger resetTrigger;
    Timer() {
        //resetTrigger.setThresholds(0.0, 0.01);
        resetTrigger.process(rescale(0.0f, 0.0f, 0.01f, 0.f, 1.f));
    }
    void setTime(float in_seconds) {
        seconds = in_seconds;
    }
    void setStride(int steps=10000) {
        Stride = steps;
        TimeStep = Stride/SampleRate;
    }
    void setSampleRate(float in_SampleRate) {
        SampleRate = in_SampleRate;
        TimeStep = Stride/SampleRate;
    }
    void setReset(float reset) {
        if (resetTrigger.process(reset)) {
            Start();
        }
    }

    void Start() {
        //rack::debug("RESET!");
        counter = seconds;  //
        running = true;
        output = 0.0f;
    }

    void ResetOutput() {
        output = 0.0f;
    }


    //void step(float dt) {
    void step() {
        //counter -= dt;
        counter -= TimeStep;
        //rack::debug("counter %f - €f", counter, engineGetSampleRate());

        if (counter <= 0.0f) {
            trigger = true;
            running = false;
            //setReset(0.0f);
            if (false) { // TODO: Check interval/once switch
                Start();
                //counter = seconds;
                //running = true;
            }
        }

        if (trigger) {
            output = 12.0f;
            //resetLight = 1.0;
            //pulseLight = 1.0;
            trigger = false;
        }
        else {
            output = 0.0f;
        }
    }

    float light() {
        return counter<=0.0f?5.0f:0.0f;  // TODO: Check expected level
    }
};


struct Timers: Module {
    enum ParamIds {
        timer1_PARAM,
        timer1_RESET_BUTTON,
        timer2_PARAM,
        timer2_RESET_BUTTON,
        timer3_PARAM,
        timer3_RESET_BUTTON,
        timer1_FINE,
        timer2_FINE,
        timer3_FINE,
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

    float timer1_dsp_val = 0.0F;
    float timer2_dsp_val = 0.0F;
    float timer3_dsp_val = 0.0F;
    float counter1_val = 0.0F;
    float counter2_val = 0.0F;
    float counter3_val = 0.0F;

    int Stride = 2500; // How many steps to include in a giant stride?
    int Steps = 0;

    Timers() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
    void step() override;
    void onSampleRateChange() override ;
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

        Vec textPos = Vec(8.0f, 18.0f);
        NVGcolor textColor = nvgRGB(0x00, 0x00, 0x00);
        nvgFillColor(vg, textColor);
        nvgText(vg, textPos.x, textPos.y, to_display.str().substr(0, 4).c_str(), NULL);
    }
};

void Timers::onSampleRateChange() {
    int SampleRate = engineGetSampleRate();
    timer1.setSampleRate(SampleRate); timer1.setStride(Stride);
    timer2.setSampleRate(SampleRate); timer2.setStride(Stride);
    timer3.setSampleRate(SampleRate); timer3.setStride(Stride);
}

void Timers::step() {
    //rack::debug("CPU %f", cpuTime);


    if (Steps++ >= Stride) {
        Steps = 0;
        //rack::debug("dsp_val %f conter_val %f", timer1_dsp_val, counter1_val);

        if (inputs[CV1_INPUT].active) {
            timer1_dsp_val = inputs[CV1_INPUT].value * 99.99f;
            lights[LIGHT1].value = 1.0f;

        } else {
            lights[LIGHT1].value = 0.0f;
            timer1_dsp_val = clamp(params[timer1_PARAM].value + params[timer1_FINE].value, 0.0f, 999.9f);
        }
        timer1.setTime(timer1_dsp_val);

        if (params[timer1_RESET_BUTTON].value > 0.0f || inputs[RESET1_INPUT].value > 0.0f) {
            timer1.Start();
        }

        if (timer1.running) {
            timer1.step();
            counter1_val = timer1.counter;
        } else {
            timer1.ResetOutput();
        }
        outputs[TRIGGER1_OUTPUT].value = timer1.output;

        if (inputs[CV2_INPUT].active) {
            timer2_dsp_val = inputs[CV2_INPUT].value * 99.99f;
            lights[LIGHT2].value = 1.0f;

        } else {
            lights[LIGHT2].value = 0.0f;
            timer2_dsp_val = clamp(params[timer2_PARAM].value + params[timer2_FINE].value, 0.0f, 999.9f);
        }
        timer2.setTime(timer2_dsp_val);

        if (params[timer2_RESET_BUTTON].value > 0.0f || inputs[RESET2_INPUT].value > 0.0f) {
            timer2.Start();
        }

        if (timer2.running) {
            timer2.step();
            counter2_val = timer2.counter;
        } else {
            timer2.ResetOutput();
        }
        outputs[TRIGGER2_OUTPUT].value = timer2.output;

        if (inputs[CV3_INPUT].active) {
            timer3_dsp_val = inputs[CV3_INPUT].value * 99.99f;
            lights[LIGHT3].value = 1.0f;

        } else {
            lights[LIGHT3].value = 0.0f;
            timer3_dsp_val = clamp(params[timer3_PARAM].value + +params[timer3_FINE].value, 0.0f, 999.9f);
        }
        timer3.setTime(timer3_dsp_val);

        if (params[timer3_RESET_BUTTON].value > 0.0f || inputs[RESET3_INPUT].value > 0.0f) {
            timer3.Start();
        }

        if (timer3.running) {
            //timer3.step(1.0 / engineGetSampleRate());
            timer3.step();
            counter3_val = timer3.counter;
        } else {
            timer3.ResetOutput();
        }
        outputs[TRIGGER3_OUTPUT].value = timer3.output;
    }

}

struct TimersWidget: ModuleWidget {
    TimersWidget(Timers *module);
};

//TimersWidget::TimersWidget() {
//    Timers *module = new Timers();
//    setModule(module);
TimersWidget::TimersWidget(Timers *module) : ModuleWidget(module) {
    box.size = Vec(15*10, 380);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(plugin, "res/Timers.svg")));
        addChild(panel);
    }

    addChild(Widget::create<ScrewSilver>(Vec(15, 0)));
    addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(Widget::create<ScrewSilver>(Vec(15, 365)));
    addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 365)));

    int Box_pos_X = 23;
    int Box_pos_Y = 65;
    int Box_size_X = 50;
    int Box_size_Y = 22;
    // Timer 1
    SmallIntegerDisplayWidgeter *timer1_dsp = new SmallIntegerDisplayWidgeter();
    timer1_dsp->box.pos = Vec(Box_pos_X, Box_pos_Y);
    timer1_dsp->box.size = Vec(Box_size_X, Box_size_Y);
    timer1_dsp->value = &module->timer1_dsp_val;
    addChild(timer1_dsp);
    int Box_offset_X = 60;
    SmallIntegerDisplayWidgeter *counter = new SmallIntegerDisplayWidgeter();
    counter->box.pos = Vec(Box_pos_X + Box_offset_X, Box_pos_Y);
    counter->box.size = Vec(Box_size_X, Box_size_Y);
    counter->value = &module->counter1_val;
    addChild(counter);

    int Reset_X = 80;
    int CV_X = 50;
    int Button_X = 69;
    int Knob_X = 13;
    int Knob2_X = Knob_X + 21;
    int Gate_X = 113;

    int Inputs_Y = 112;
    int Knob_Y = 112;
    int Knob2_Y = Knob_Y-15;
    int Gate_Y = Inputs_Y;
    int Button_Y = 47;

    addInput(Port::create<PJ301MPort>(Vec(Reset_X, Inputs_Y), Port::INPUT, module, Timers::RESET1_INPUT));
    addInput(Port::create<PJ301MPort>(Vec(CV_X, Inputs_Y), Port::INPUT, module, Timers::CV1_INPUT));
    addParam(ParamWidget::create<LEDButton>(Vec(Button_X, Button_Y), module, Timers::timer1_RESET_BUTTON, 0.0, 1.0, 0.0));
    addParam(ParamWidget::create<BefacoTinyKnob>(Vec(Knob_X, Knob_Y), module, Timers::timer1_PARAM, 0.0, 1000.0, 0.0));
    addParam(ParamWidget::create<Trimpot>(Vec(Knob2_X, Knob2_Y), module, Timers::timer1_FINE, 0.0, 5.0, 0.0));
    addOutput(Port::create<PJ301MPort>(Vec(Gate_X, Gate_Y), Port::OUTPUT, module, Timers::TRIGGER1_OUTPUT));
    addChild(ModuleLightWidget::create<SmallLight<RedLight>>(Vec(Button_X + 20 , Button_Y), module, Timers::LIGHT1));

    // Timer 2
    int offset_Y = 100;
    SmallIntegerDisplayWidgeter *timer2_dsp = new SmallIntegerDisplayWidgeter();
    timer2_dsp->box.pos = Vec(Box_pos_X, offset_Y + Box_pos_Y);
    timer2_dsp->box.size = Vec(Box_size_X, Box_size_Y);
    timer2_dsp->value = &module->timer2_dsp_val;
    addChild(timer2_dsp);

    SmallIntegerDisplayWidgeter *counter2 = new SmallIntegerDisplayWidgeter();
    counter2->box.pos = Vec(Box_pos_X + Box_offset_X, offset_Y + Box_pos_Y);
    counter2->box.size = Vec(Box_size_X, Box_size_Y);
    counter2->value = &module->counter2_val;
    addChild(counter2);

    addInput(Port::create<PJ301MPort>(Vec(Reset_X, offset_Y + Inputs_Y), Port::INPUT, module, Timers::RESET2_INPUT));
    addInput(Port::create<PJ301MPort>(Vec(CV_X, offset_Y + Inputs_Y), Port::INPUT, module, Timers::CV2_INPUT));
    addParam(ParamWidget::create<LEDButton>(Vec(Button_X, offset_Y + Button_Y), module, Timers::timer2_RESET_BUTTON, 0.0, 1.0, 0.0));
    addParam(ParamWidget::create<BefacoTinyKnob>(Vec(Knob_X, offset_Y + Knob_Y), module, Timers::timer2_PARAM, 0.0, 1000.0, 0.0));
    addParam(ParamWidget::create<Trimpot>(Vec(Knob2_X, offset_Y + Knob2_Y), module, Timers::timer2_FINE, 0.0, 5.0, 0.0));
    addOutput(Port::create<PJ301MPort>(Vec(Gate_X, offset_Y + Gate_Y), Port::OUTPUT, module, Timers::TRIGGER2_OUTPUT));

    // Timer 3
    offset_Y = 200;
    SmallIntegerDisplayWidgeter *timer3_dsp = new SmallIntegerDisplayWidgeter();
    timer3_dsp->box.pos = Vec(Box_pos_X, offset_Y + Box_pos_Y);
    timer3_dsp->box.size = Vec(Box_size_X, Box_size_Y);
    timer3_dsp->value = &module->timer3_dsp_val;
    addChild(timer3_dsp);

    SmallIntegerDisplayWidgeter *counter3 = new SmallIntegerDisplayWidgeter();
    counter3->box.pos = Vec(Box_pos_X + Box_offset_X, offset_Y + Box_pos_Y);
    counter3->box.size = Vec(Box_size_X, Box_size_Y);
    counter3->value = &module->counter3_val;
    addChild(counter3);

    addInput(Port::create<PJ301MPort>(Vec(Reset_X, offset_Y + Inputs_Y), Port::INPUT, module, Timers::RESET3_INPUT));
    addInput(Port::create<PJ301MPort>(Vec(CV_X, offset_Y + Inputs_Y), Port::INPUT, module, Timers::CV3_INPUT));
    addParam(ParamWidget::create<LEDButton>(Vec(Button_X, offset_Y + Button_Y), module, Timers::timer3_RESET_BUTTON, 0.0, 1.0, 0.0));
    addParam(ParamWidget::create<BefacoTinyKnob>(Vec(Knob_X, offset_Y + Knob_Y), module, Timers::timer3_PARAM, 0.0, 1000.0, 0.0));
    addParam(ParamWidget::create<Trimpot>(Vec(Knob2_X, offset_Y + Knob2_Y), module, Timers::timer3_FINE, 0.0, 5.0, 0.0));
    addOutput(Port::create<PJ301MPort>(Vec(Gate_X, offset_Y + Gate_Y), Port::OUTPUT, module, Timers::TRIGGER3_OUTPUT));

    Model *modelTimers = Model::create<Timers, TimersWidget>("JLmod", "Timers", "Timers", UTILITY_TAG);

}
