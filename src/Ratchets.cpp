#include <iostream>
#include <iomanip> // remove?
#include <chrono>
#include "dsp/digital.hpp"
#include "JLmod.hpp"
#include "common.hpp"
//#include "clock.hpp"
// Copyright (c) 2019 Joakim Lindbom
#include <math.h>

#undef DEBUG

class Clock {
  double step;  // -1.0 when stopped, [0 to 2*period[ for clock steps (*2 is because of swing, so we do groups of 2 periods)
  double length;  // double period
  double sampleTime;
  int iterations;  // run this many double periods before going into sync if sub-clock
  static constexpr double guard = 0.0005;  // in seconds, region for sync to occur right before end of length of last iteration; sub clocks must be low during this period
  bool *resetClockOutputsHigh;

  public:
  Clock();
  void reset2();
  bool isReset2();
  double getStep();
  void setup(Clock* clkGiven, bool *resetClockOutputsHighPtr);
  void start2();

  void setup(double lengthGiven, int iterationsGiven, double sampleTimeGiven);

  void stepClock();

  void applyNewLength(double lengthStretchFactor);

  int isHigh(float swing, float pulseWidth, bool debug);
};


Clock::Clock() {
  reset2();
}

inline void Clock::reset2() {
  step = -1.0;
}

inline bool Clock::isReset2() {
  return step == -1.0;
}

inline double Clock::getStep() {
  return step;
}

void Clock::setup(Clock* clkGiven, bool *resetClockOutputsHighPtr) {
  resetClockOutputsHigh = resetClockOutputsHighPtr;
}

inline void Clock::start2() {
  step = 0.0;
}

inline void Clock::setup(double lengthGiven, int iterationsGiven, double sampleTimeGiven) {
  length = lengthGiven;
  iterations = iterationsGiven;
  sampleTime = sampleTimeGiven;
}

void Clock::stepClock() {  // here the clock was output on step "step", this function is called at end of module::step()
  if (step >= 0.0) {  // if active clock
    step += sampleTime;
    //std::cout << "step:" << step << "\n";
    if (step >= length) {  // reached end iteration
      iterations--;
      step -= length;
      if (iterations <= 0)
        reset2();  // frame done
    }
  }
}

void Clock::applyNewLength(double lengthStretchFactor) {
  if (step != -1.0)
    step *= lengthStretchFactor;
  length *= lengthStretchFactor;
}

int Clock::isHigh(float swing, float pulseWidth, bool debug = false) {
  // last 0.5ms (guard time) must be low so that sync mechanism will work properly (i.e. no missed pulses)
  //   this will automatically be the case, since code below disallows any pulses or inter-pulse times less than 1ms
  int high = 0;
  if (step >= 0.0) {
    float swParam = swing;  // swing is [-1 : 1]

    // all following values are in seconds
    float onems = 0.001f;
    float period = (float)length / 2.0f;
    float swing = (period - 2.0f * onems) * swParam;
    float p2min = onems;
    float p2max = period - onems - fabsf(swing);
    if (p2max < p2min) {
      p2max = p2min;
    }

    // double p1 = 0.0;// implicit, no need
    double p2 = (double)((p2max - p2min) * pulseWidth + p2min);  // pulseWidth is [0 : 1]
    double p3 = (double)(period + swing);
    double p4 = ((double)(period + swing)) + p2;

    if (step < p2)
      high = 1;
    else if ((step >= p3) && (step < p4))
      high = 2;
    if (debug) std::cout << "p2: " << p2 << " p3: " << p3 << " p4: " << p4 << " high: " << high << "\n";
  } else if (*resetClockOutputsHigh)
    high = 1;

  return high;
}

//----------------

#define NUM_CLOCKS 8
#define MAX_SEQUENCER_STEPS 8

#define BOX_SIZE_X 240

struct Ratchets : Module {
  enum ParamIds {
    RUN_PARAM,                                  // Run sequencer?
    STEPS_PARAM,                                // How many step to execute
    // Sequencer matrix
    ENUMS(STEP_LED_PARAM, MAX_SEQUENCER_STEPS), // Enable/disable step
    ENUMS(RATCHETS1, MAX_SEQUENCER_STEPS),      // How many sub gates to trigger
    ENUMS(BERNOULI_GATE, MAX_SEQUENCER_STEPS),  // Bernouly gate value - use RATCHET1 or RATCHET2 value?
    ENUMS(RATCHETS2, MAX_SEQUENCER_STEPS),      // How many sub gates to trigger
    ENUMS(OCTAVE_SEQ, MAX_SEQUENCER_STEPS),     // Output note: Octave
    ENUMS(CV_SEQ, MAX_SEQUENCER_STEPS),         // Output note: Note
    NUM_PARAMS
  };
  enum InputIds {
    GATE_INPUT,                                 // External clock input
    NUM_INPUTS
  };
  enum OutputIds {
    OUT_GATE,                                   // Gate out
    OUT_CV,                                     // CV Out
    #ifdef DEBUG
    ENUMS(OUT_DEBUG, MAX_SEQUENCER_STEPS),
    #endif
    NUM_OUTPUTS
  };
  enum LightIds {
    BLINK_LIGHT,
    ENUMS(STEP_LED, MAX_SEQUENCER_STEPS),
    NUM_LIGHTS
  };

  float phase = 0.0;
  float blinkPhase = 0.0;

  bool running = true;

  double sampleRate = 0.0;
  double sampleTime = 0.0;
  float deltaTime = 0.0f;
  double length = 0.0f;  // double period
  double stepCount = 0.0f;

  static constexpr float pulseWidth = 0.011f;  // 11 ms used by SEQ-3 for gate

  uint64_t duration_between_ticks = 0;
  // calculated time between clock ticks
  uint64_t tick1_ms = 0;
  uint64_t tick2_ms = 0;
  uint64_t now = 0;
  bool waiting_for_1st_tick = true;
  bool waiting_for_2nd_tick = false;

  float old_gate_value = 0.0f;

    int misses;

    float timer = 0.0;
    float seconds = 0.0;
    float deltaT;
    float BPM=120.0;
    float oldBPM=0.0;
    SchmittTrigger gateTrigger;
    SchmittTrigger clockTrigger;

    double timeoutTime;
    int ppqn;
    float newMasterLength;
	float masterLength;
	bool syncRatios[NUM_CLOCKS];// 0 index unused
	int ratiosDoubled[NUM_CLOCKS];
	int extPulseNumber;// 0 to ppqn - 1
	double extIntervalTime;

    // Clocks related
    bool resetClockOutputsHigh;
    Clock clk[MAX_SEQUENCER_STEPS];
	const float ratioValues[NUM_CLOCKS] = {1, 2, 3, 4, 5, 6, 7, 8};
	static const int bpmMax = 300;
	static const int bpmMin = 30;
	static constexpr float masterLengthMax = 120.0f / bpmMin;// a length is a double period
	static constexpr float masterLengthMin = 120.0f / bpmMax;// a length is a double period

    // Sequencer
    int current_seq_step = 0; // -1;
    bool gates[MAX_SEQUENCER_STEPS] = {};
    SchmittTrigger gateTriggers[MAX_SEQUENCER_STEPS];
	const float clockValues[NUM_CLOCKS+1] = {0, 1, 2, 3, 4, 5, 6, 7, 7};  // Used to force integer selection  // TODO: Remove extra step
	int seqClockUsed1[MAX_SEQUENCER_STEPS];
	int seqClockUsed2[MAX_SEQUENCER_STEPS];
	float bernouli_value;

  Ratchets() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
    for (int i = 1; i < MAX_SEQUENCER_STEPS; i++) {
        clk[i].setup(&clk[0], &resetClockOutputsHigh);
    }
    onReset();
   }
  void step() override;

	inline bool checkBeat(float timer, int mult) {
		return ( ((timer - mult*seconds) * (timer - mult*seconds) / (seconds*seconds) < 0.2 ) && misses < 4);
	}


  // For more advanced Module features, read Rack's engine.hpp header file
  // - toJson, fromJson: serialization of internal data
  // - onSampleRateChange: event triggered by a change of sample rate
  // - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu

  void onCreate() override {
    onReset();
    for (int i = 0; i< MAX_SEQUENCER_STEPS; i++) {
        seqClockUsed1[i] = 0;
        seqClockUsed2[i] = 0;
    }
  }

  void onSampleRateChange() override {
    sampleRate = (double)engineGetSampleRate();
    sampleTime = 1.0 / sampleRate;
    deltaTime = engineGetSampleTime();
    deltaT = 1.0/sampleRate;
  }

  void onReset() override {
    sampleRate = (double)engineGetSampleRate();
    sampleTime = 1.0 / sampleRate;
    deltaTime = engineGetSampleTime();
    deltaT = 1.0/sampleRate;
    std::cout << " deltaTime: " << deltaTime << " - sampleTime: " << sampleTime << " sampleRate:" << sampleRate << "\n";
    running = true;

    resetClockOutputsHigh = true;

    for (int i = 0; i < MAX_SEQUENCER_STEPS; i++) {
        gates[i] = true;
	}
	resetClocks(false);

  }

    void onRandomize() override {
        for (int i = 0; i < MAX_SEQUENCER_STEPS; i++) {
            gates[i] = (randomUniform() > 0.5f);
        }
    }

    float getBPM() {
      bool rising = inputs[GATE_INPUT].value >= 1.0f && old_gate_value <0.1f;  // TODO: Zero?
      if (rising) {
        if (waiting_for_1st_tick) {
            tick1_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            waiting_for_2nd_tick = true;
            waiting_for_1st_tick = false;
        } else if (waiting_for_2nd_tick) {   // Wait for next gate
            tick2_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            duration_between_ticks = tick2_ms - tick1_ms;
            oldBPM = BPM;
            BPM = 1000.0 * 60.0/duration_between_ticks;
            BPM = round(10.0*BPM)/10.0;
            // std::cout << " Duration between ticks: " << duration_between_ticks;
            // std::cout << " BPM: " << BPM << "\n";
            waiting_for_2nd_tick = false;
            waiting_for_1st_tick = true;
        }
      }
      return (BPM);

    } //getBPM

	int getRatioDoubled(int Index) {
		// int i = (int) round( params[RATIO_PARAMS + ratioKnobIndex].value );// [ -(numRatios-1) ; (numRatios-1) ]
		int i = Index;
		int ret = (int) (ratioValues[i] * 2.0f + 0.5f);
		return ret;
	}


	void setIndex(int index) {
		int numSteps = MAX_SEQUENCER_STEPS; // (int) clamp(roundf(params[STEPS_PARAM].value + inputs[STEPS_INPUT].value), 1.0f, 8.0f);
		//phase = 0.f;
		this->current_seq_step = index;
		if (this->current_seq_step >= MAX_SEQUENCER_STEPS || this->current_seq_step >= params[STEPS_PARAM].value)
			this->current_seq_step = 0;
	}

	void resetClocks(bool hardReset) {// set hardReset to true to revert learned BPM to 120 in sync mode, or else when false, learned bmp will stay persistent
		for (int i = 0; i < NUM_CLOCKS; i++) {
			clk[i].reset2();
//			if (i < 3) // TODO: 7?
//				delay[i].reset2(resetClockOutputsHigh);
			syncRatios[i] = false;
			ratiosDoubled[i] = getRatioDoubled(i);
			//updatePulseSwingDelay();
			//outputs[CLK_OUTPUTS + i].value = (resetClockOutputsHigh ? 10.0f : 0.0f);
		}
		extPulseNumber = -1;
		extIntervalTime = 0.0;
		timeoutTime = 2.0 / ppqn + 0.1;// worst case. This is a double period at 30 BPM (4s), divided by the expected number of edges in the double period
									   //   which is 2*ppqn, plus epsilon. This timeoutTime is only used for timingout the 2nd clock edge
		newMasterLength = 1.0f / powf(2.0f, BPM);// bpm = 120*2^V, 2T = 120/bpm = 120/(120*2^V) = 1/2^V
        //newMasterLength = 120.0f / getBpmKnob();
        newMasterLength = clamp(newMasterLength, masterLengthMin, masterLengthMax);
        masterLength = newMasterLength;
        masterLength = 1.0f;  // 120 BPM test
	}



}; // Module



void Ratchets::step() {
  // bool gated = inputs[GATE_INPUT].value >= 1.0f;
    bool rising = inputs[GATE_INPUT].value >= 1.0f && old_gate_value <0.1f;  // TODO: Zero?
    bool falling = inputs[GATE_INPUT].value < 0.1f && old_gate_value >=1.0f;  // TODO: Zero?
    // ---
    sampleRate = (double)engineGetSampleRate();
    sampleTime = 1.0 / sampleRate;
    deltaTime = engineGetSampleTime();
    sampleTime = 1.0/sampleRate;
    // ---

    BPM = getBPM();

    if (running) {
        // See if clocks finished their prescribed number of iterations of double periods (and syncWait for sub) or
        //    if they were forced reset and if so, recalc and restart them

        // Master clock
        if (clk[0].isReset2()) {
            // See if ratio knobs changed (or unitinialized)
            for (int i = 1; i < NUM_CLOCKS; i++) {
                if (syncRatios[i]) {// unused (undetermined state) for master
                    clk[i].reset2();// force reset (thus refresh) of that sub-clock
                    ratiosDoubled[i] = getRatioDoubled(i);
                    syncRatios[i] = false;
                }
            }
            #ifdef DEBUG
            std::cout << "masterlength: " << masterLength << " sampleTime" << sampleTime << "\n";
            #endif
            clk[0].setup(masterLength, 1, sampleTime);// must call setup before start. length = double_period
            clk[0].start2();

        }

        // Sub clocks
        for (int i = 1; i < NUM_CLOCKS; i++) {
            if (clk[i].isReset2()) {
                double length;
                int iterations;
                int ratioDoubled = ratiosDoubled[i];
                length = (2.0f * masterLength) / ((double)ratioDoubled);
                iterations = ratioDoubled / (2l - (ratioDoubled % 2l));
                //std::cout << "iterations: " << iterations << " length: " << length << " sampleTime" << sampleTime << "\n";
                clk[i].setup(length, iterations, sampleTime);
                clk[i].start2();
            }
            //delay[i - 1].write(clk[i].isHigh(1, pulseWidth[i]));
            #ifdef DEBUG
            for (int i=0; i<MAX_SEQUENCER_STEPS; i++) {
                outputs[OUT_DEBUG+i].value = clk[i].isHigh(0.0, 0.5) ? 10.0f : 0.0f;
            }
            #endif
        }

        // Step clocks
        for (int i = 0; i < NUM_CLOCKS; i++) {
            clk[i].stepClock();
         }

        // Check RATCHET1 for each step in sequencer
        for (int i = 0; i < MAX_SEQUENCER_STEPS; i++) {
            //seqClockUsed[i] = clockValues[params[RATCHETS1+i].value + 0]
            //std::cout << "i=" << i << " value=" << params[RATCHETS1+i].value-1 << "\n";
            seqClockUsed1[i] = clockValues[(int)params[RATCHETS1+i].value];   // TODO: Should be -1 !!!!
            //std::cout << "2 " << i << " value=" << params[RATCHETS2+i].value-1 << "\n";
            seqClockUsed2[i] = clockValues[(int)params[RATCHETS2+i].value];

            //std::cout << "i=" << i << " clock=" << seqClockUsed[i] << "\n";
        }

        bool gateIn = false;
        if (inputs[GATE_INPUT].active) {
            // setIndex(current_seq_step + 1);  // testing
        // External clock
            if (clockTrigger.process(inputs[GATE_INPUT].value)) {
                setIndex(current_seq_step + 1);
                //if (current_seq_step == 0) {
                    bernouli_value = randomUniform();
                //}
            }
            gateIn = clockTrigger.isHigh();


            // Illuminate sequencer step LED
            for (int i=0; i<MAX_SEQUENCER_STEPS; i++) {
                if (gateTriggers[i].process(params[STEP_LED_PARAM + i].value)) {
                    gates[i] = !gates[i];
                }
            }

            int use_steps = MAX_SEQUENCER_STEPS<params[STEPS_PARAM].value?MAX_SEQUENCER_STEPS:params[STEPS_PARAM].value;
            for (int i=0; i<use_steps; i++) {
                lights[STEP_LED + i].setBrightnessSmooth((gateIn && i == current_seq_step) ? (gates[i] ? 1.f : 0.66) : (gates[i] ? 0.33 : 0.0));
            }

            if (use_steps<MAX_SEQUENCER_STEPS) {
                for (int i=use_steps; i<MAX_SEQUENCER_STEPS; i++) {
                    lights[STEP_LED + i].setBrightnessSmooth((gateIn && i == current_seq_step) ? 0.66 : (gates[i] ? 0.33 : 0.0));
                }
            }


            //std::cout << "Voltage:" << params[OCTAVE_SEQ+current_seq_step].value+params[CV_SEQ+current_seq_step].value << " Octave: " << params[OCTAVE_SEQ+current_seq_step].value << " CV: " << params[CV_SEQ+current_seq_step].value << "\n";
            if (outputs[OUT_CV].active) {
                outputs[OUT_CV].value = gates[current_seq_step]?params[OCTAVE_SEQ+current_seq_step].value+params[CV_SEQ+current_seq_step].value:0.0;
            }

            old_gate_value = inputs[GATE_INPUT].value;
            // TODO: Calculate based on sample rate instead?
            //now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();

            //std::cout << "current_seq_step=" << current_seq_step << " clock=" << seqClockUsed[current_seq_step] << " isHigh()" << clk[seqClockUsed[current_seq_step]].isHigh(0.0, 0.5) << "\n";
            //std::cout << "Bernouli= " << ((randomUniform() > params[BERNOULI_GATE+current_seq_step].value) ? "A \n" : "B \n");
            //std::cout << "seqClockUsed1[current_seq_step] " << seqClockUsed1[current_seq_step] << " step " << current_seq_step << "\n";
            //outputs[OUT_GATE].value = gates[current_seq_step]?clk[seqClockUsed1[current_seq_step]].isHigh(0.0, 0.5) ? 10.0f : 0.0f: 0.0f;
            outputs[OUT_GATE].value = gates[current_seq_step]?clk[(bernouli_value > params[BERNOULI_GATE+current_seq_step].value)?seqClockUsed1[current_seq_step]:seqClockUsed2[current_seq_step]].isHigh(0.0, 0.5) ? 10.0f : 0.0f: 0.0f;

            deltaTime = engineGetSampleTime();
        }
    }

}


struct RatchetsWidget : ModuleWidget {
  RatchetsWidget(Ratchets *module) : ModuleWidget(module) {
    setPanel(SVG::load(assetPlugin(plugin, "res/Ratchets.svg")));

    // Screws
    addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
    addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
    addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

    static constexpr float RowPosY = 34.0f;
    static constexpr float BaseY = 50.0;
    for (int i = 0; i < MAX_SEQUENCER_STEPS; i++) {
        addParam(createParam<LEDButton>(Vec(15 , BaseY + i * RowPosY+3), module, Ratchets::STEP_LED_PARAM + i, 0.0f, 1.0f, 0.0f));
        addChild(ModuleLightWidget::create<MediumLight<GreenLight>>(Vec(15 + 4.4f, BaseY + i * RowPosY + 4.4f+3), module, Ratchets::STEP_LED + i));

    	addParam(ParamWidget::create<SmallGreyKnobSnap>(Vec(33, BaseY + i * RowPosY), module, Ratchets::RATCHETS1+i, 1.0, NUM_CLOCKS, 1.0));
    	addParam(ParamWidget::create<SmallGreyKnob>(Vec(63, BaseY + i * RowPosY), module, Ratchets::BERNOULI_GATE+i, 0.0, 1.0, 0.0));
    	addParam(ParamWidget::create<SmallGreyKnobSnap>(Vec(93, BaseY + i * RowPosY), module, Ratchets::RATCHETS2+i, 1.0, NUM_CLOCKS, 1.0));
    	addParam(ParamWidget::create<SmallGreyKnobSnap>(Vec(150, BaseY + i * RowPosY), module, Ratchets::OCTAVE_SEQ+i, 1.0, 7.0, 3.0));
    	addParam(ParamWidget::create<SmallGreyKnob>(Vec(180, BaseY + i * RowPosY), module, Ratchets::CV_SEQ+i, 0.0, 1.0, 0.5));

        #ifdef DEBUG
        addOutput(Port::create<PJ301MPort>(Vec(185, BaseY + i * RowPosY), Port::OUTPUT, module, Ratchets::OUT_DEBUG+i));
        #endif
    }
    // IMSmallKnobNotify
    addInput(Port::create<PJ301MPort>(Vec(16, BaseY + 8 * RowPosY + 12), Port::INPUT, module, Ratchets::GATE_INPUT));
    addParam(ParamWidget::create<SmallGreyKnobSnap>(Vec(50, BaseY + 8 * RowPosY + 13), module, Ratchets::STEPS_PARAM, 1.0, MAX_SEQUENCER_STEPS, MAX_SEQUENCER_STEPS));
    addOutput(Port::create<PJ301MPort>(Vec(157, BaseY + 8 * RowPosY + 10), Port::OUTPUT, module, Ratchets::OUT_GATE));
    addOutput(Port::create<PJ301MPort>(Vec(189, BaseY + 8 * RowPosY + 10), Port::OUTPUT, module, Ratchets::OUT_CV));
    //addParam(ParamWidget::create<LEDButton>(Vec(60, BaseY + 8 * RowPosY), module, Ratchets::RUN_PARAM, 0.0f, 1.0f, 0.0f));

//    addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(41, BaseY + 8 * RowPosY), module, Ratchets::BLINK_LIGHT));
  }
};


// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelRatchets = Model::create<Ratchets, RatchetsWidget>("JL modules", "Ratchets", "My Module", SEQUENCER_TAG); // TODO: Change tag
