#include <iostream>
#include <iomanip> // remove?
#include <chrono>
#include "dsp/digital.hpp"
#include "JLmod.hpp"
#include "common.hpp"
#include "clock.hpp"
// Copyright (c) 2019 Joakim Lindbom
#include <math.h>

#define DBG

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
    ENUMS(BERNOULI_GATE, MAX_SEQUENCER_STEPS),  // Bernouli gate value - use RATCHET1 or RATCHET2 value?
    ENUMS(RATCHETS2, MAX_SEQUENCER_STEPS),      // How many sub gates to trigger
    ENUMS(OCTAVE_SEQ, MAX_SEQUENCER_STEPS),     // Output note: Octave
    ENUMS(CV_SEQ, MAX_SEQUENCER_STEPS),         // Output note: Note
    RESET_PARAM,                                // Reset button
    BPMMODE_DOWN_PARAM,
    BPMMODE_UP_PARAM,

    NUM_PARAMS
    };
    enum InputIds {
    GATE_INPUT,                                 // External clock input
    IN_RESET,                                   // External reset input
    IN_RUN,                                     // External run input
    NUM_INPUTS
    };
    enum OutputIds {
    OUT_GATE,                                   // Gate out
    OUT_CV,                                     // CV Out
    NUM_OUTPUTS
    };
    enum LightIds {
    LIGHT_RUNNING,
    ENUMS(STEP_LED, MAX_SEQUENCER_STEPS),
    BPM_LOCKED,
    NUM_LIGHTS
    };

    // Expander
	float rightMessages[2][9] = {};// messages from expander

    float phase = 0.0;
    float blinkPhase = 0.0;

    bool running = true;

    double sampleRate = 0.0;
    double sampleTime = 0.0;
    float deltaTime = 0.0f;
    double length = 0.0f;  // double period
    double stepCount = 0.0f;

	bool scheduledReset = false;
	long cantRunWarning = 0l;// 0 when no warning, positive downward step counter timer when warning
	RefreshCounter refresh;

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
    float BPM=-1.0;
    float oldBPM=0.0;
    bool BPM_locked = false;
    dsp::SchmittTrigger gateTrigger;
    dsp::SchmittTrigger clockTrigger;

	bool bpmDetectionMode;
	int restartOnStopStartRun;// 0 = nothing, 1 = restart on stop run, 2 = restart on start run
	bool sendResetOnRestart;
    int ppqn;  // Pulses Per Quarter Note
	bool momentaryRunInput;// true = trigger (original rising edge only version), false = level sensitive (emulated with rising and falling detection)

    double timeoutTime;

    float newMasterLength;
    float masterLength;
    bool syncRatios[NUM_CLOCKS];// 0 index unused
    int ratiosDoubled[NUM_CLOCKS];
    int extPulseNumber;// 0 to ppqn - 1
    double extIntervalTime;

    // Clocks related
    bool resetClockOutputsHigh;
    Clock clk[MAX_SEQUENCER_STEPS];
    const float ratioValues[NUM_CLOCKS] = {0.5, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0};
    static const int bpmMax = 300;
    static const int bpmMin = 30;
	static constexpr float masterLengthMax = 60.0f / bpmMin;// a length is a period
	static constexpr float masterLengthMin = 60.0f / bpmMax;// a length is a period

    bool emitResetOnStopRun;
    Trigger resetTrigger;
    Trigger bpmDetectTrigger;
    dsp::PulseGenerator resetPulse;
    dsp::PulseGenerator runPulse;
	Trigger runButtonTrigger;
	TriggerRiseFall runInputTrigger;


    // Sequencer
    int current_seq_step = 0; // -1;
    bool gates[MAX_SEQUENCER_STEPS] = {};
    dsp::SchmittTrigger gateTriggers[MAX_SEQUENCER_STEPS];
	const float clockValues[NUM_CLOCKS+1] = {0, 1, 2, 3, 4, 5, 6, 7, 8};  // Used to force integer selection  // TODO: Remove extra step
	int seqClockUsed1[MAX_SEQUENCER_STEPS];
	int seqClockUsed2[MAX_SEQUENCER_STEPS];
	float bernouli_value;

    Ratchets() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(Ratchets::STEPS_PARAM, 1.0, MAX_SEQUENCER_STEPS, MAX_SEQUENCER_STEPS, "");
        configParam(Ratchets::RESET_PARAM, 0.0f, 1.0f, 0.0f, "");
        configParam(Ratchets::RUN_PARAM, 0.0f, 1.0f, 0.0f, "");
        for (int i=0; i<MAX_SEQUENCER_STEPS; i++) {
            configParam(Ratchets::STEP_LED_PARAM + i, 0.0f, 1.0f, 0.0f, "");
            configParam(Ratchets::RATCHETS1+i, 1.0, NUM_CLOCKS, 1.0, "");
            configParam(Ratchets::BERNOULI_GATE+i, 0.0, 1.0, 0.0, "");
            configParam(Ratchets::RATCHETS2+i, 1.0, NUM_CLOCKS, 1.0, "");
            configParam(Ratchets::OCTAVE_SEQ+i, 1.0, 7.0, 3.0, "");
            configParam(Ratchets::CV_SEQ+i, 0.0, 1.0, 0.5, "");
        }

        for (int i=0; i<NUM_CLOCKS; i++) {
            clk[i].construct(i==0?NULL:&clk[0], &resetClockOutputsHigh);
        }

        rightExpander.producerMessage = rightMessages[0];
        rightExpander.consumerMessage = rightMessages[1];

        onReset();
    }

    void process(const ProcessArgs& args) override;

    void updateDebugExpander() {
		if (rightExpander.module && rightExpander.module->model == modelDebugExpander) {
			// Get message from right expander
			float *message = (float*) rightExpander.module->leftExpander.producerMessage;
			// Write message
			for (int i = 0; i < 8; i++) {
				message[i] = clk[i].isHigh()?1.0f:0.0f;
			}
			message[8] = (float)BPM;
			// Flip messages at the end of the timestep
			rightExpander.module->leftExpander.messageFlipRequested = true;
		}
    }

    inline bool checkBeat(float timer, int mult) {
        return ( ((timer - mult*seconds) * (timer - mult*seconds) / (seconds*seconds) < 0.2 ) && misses < 4);
    }


  // For more advanced Module features, read Rack's engine.hpp header file
  // - dataToJson, dataFromJson: serialization of internal data
  // - onSampleRateChange: event triggered by a change of sample rate
  // - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu

    void onCreate() { // override {
        onReset();
        for (int i = 0; i< MAX_SEQUENCER_STEPS; i++) {
            seqClockUsed1[i] = 0;
            seqClockUsed2[i] = 0;
        }
    }

    void onSampleRateChange() override {
        sampleRate = (double)APP->engine->getSampleRate();
        sampleTime = 1.0 / sampleRate;
        deltaTime = APP->engine->getSampleTime();  //TODO: Simplify
        deltaT = 1.0/sampleRate;
    }

    void onReset() override {
        sampleRate = (double)APP->engine->getSampleRate();
        sampleTime = 1.0 / sampleRate;
        deltaTime = APP->engine->getSampleTime();
        deltaT = 1.0/sampleRate;
        std::cout << " deltaTime: " << deltaTime << " - sampleTime: " << sampleTime << " sampleRate:" << sampleRate << "\n";

        ppqn = 1;
        running = true;

        resetClockOutputsHigh = true;

        for (int i = 0; i < MAX_SEQUENCER_STEPS; i++) {
            gates[i] = true;
        }
        resetClocks(false);

    }

    void onRandomize() override {
        for (int i = 0; i < MAX_SEQUENCER_STEPS; i++) {
            gates[i] = (random::uniform() > 0.5f);
        }
    }

    float getBPM() {
      bool rising = inputs[GATE_INPUT].getVoltage() >= 1.0f && old_gate_value <0.1f;  // TODO: Zero?
      if (rising) {
        if (waiting_for_1st_tick) {
            tick1_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            waiting_for_2nd_tick = true;
            waiting_for_1st_tick = false;
        } else if (waiting_for_2nd_tick) {   // Wait for next gate
            tick2_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            duration_between_ticks = tick2_ms - tick1_ms;
            oldBPM = BPM;
            BPM = 1000.0 * 60.0/duration_between_ticks/2;
            BPM = round(10.0*BPM)/10.0;
            BPM_locked = true;
            // std::cout << " Duration between ticks: " << duration_between_ticks;
            //std::cout << " BPM: " << BPM << "\n";
            waiting_for_2nd_tick = false;
            waiting_for_1st_tick = true;
        }
      }
      return (BPM);

    } //getBPM

	int getRatioDoubled(int Index) {
	    int ret = 1;
        if (Index < 1) // Master clock
            return 1;

        if (Index <= NUM_CLOCKS) {
            ret = (int) (ratioValues[Index] * 2.0f + 0.5f);
        } else {
            ret = 1;
        }
            std::cout << "getRatioDoubled Index= " << Index << " Return: " << ret << "\n";
            return ret;
	}


	void setIndex(int index) {
		int numSteps = MAX_SEQUENCER_STEPS; // (int) clamp(roundf(params[STEPS_PARAM].getValue() + inputs[STEPS_INPUT].getVoltage()), 1.0f, 8.0f);
		//phase = 0.f;
		this->current_seq_step = index;
		if (this->current_seq_step >= MAX_SEQUENCER_STEPS || this->current_seq_step >= params[STEPS_PARAM].getValue())
			this->current_seq_step = 0;
	}

	void resetClocks(bool hardReset) {// set hardReset to true to revert learned BPM to 120 in sync mode, or else when false, learned bmp will stay persistent
		sampleRate = (double)(APP->engine->getSampleRate());
		sampleTime = 1.0 / sampleRate;
		for (int i = 0; i < NUM_CLOCKS; i++) {
			clk[i].reset();
			syncRatios[i] = false;
			ratiosDoubled[i] = getRatioDoubled(i);
			//outputs[CLK_OUTPUTS + i].setVoltage((resetClockOutputsHigh ? 10.0f : 0.0f));
		}
		extPulseNumber = -1;
		extIntervalTime = 0.0;
		timeoutTime = 2.0 / ppqn + 0.1;// worst case. This is a double period at 30 BPM (4s), divided by the expected number of edges in the double period
									   //   which is 2*ppqn, plus epsilon. This timeoutTime is only used for timingout the 2nd clock edge
		if (inputs[GATE_INPUT].isConnected()) {
			if (bpmDetectionMode) {
				if (hardReset)
					newMasterLength = 0.5f;// 120 BPM
			}
			else
				newMasterLength = 0.5f / std::pow(2.0f, inputs[GATE_INPUT].getVoltage());// bpm = 120*2^V, T = 60/bpm = 60/(120*2^V) = 0.5/2^V
		}
		else
			newMasterLength = 60.0f;
		newMasterLength = clamp(newMasterLength, masterLengthMin, masterLengthMax);
		masterLength = newMasterLength;
	}

    void toggleRun(void) {
    if (!(bpmDetectionMode && inputs[GATE_INPUT].isConnected()) || running) {// toggle when not BPM detect, turn off only when BPM detect (allows turn off faster than timeout if don't want any trailing beats after stoppage). If allow manually start in bpmDetectionMode   the clock will not know which pulse is the 1st of a ppqn set, so only allow stop
        running = !running;
        runPulse.trigger(0.001f);
        if (!running && restartOnStopStartRun == 1) {
            resetClocks(false);
            if (sendResetOnRestart) {
                resetPulse.trigger(0.001f);
                //resetLight = 1.0f;
            }
        }
        if (running && restartOnStopStartRun == 2) {
            resetClocks(false);
            if (sendResetOnRestart) {
                resetPulse.trigger(0.001f);
                //resetLight = 1.0f;
            }
        }
    }
    else {
        cantRunWarning = (long) (0.7 * sampleRate / RefreshCounter::displayRefreshStepSkips);
    }
}


}; // Module


// Main DSP loop
void Ratchets::process(const ProcessArgs& args) {

    // Scheduled reset
    if (scheduledReset) {
        resetClocks(false);
        scheduledReset = false;
    }
    // Run button
    if (runButtonTrigger.process(params[RUN_PARAM].getValue())) {
        toggleRun();
    }
    // Run input
    if (inputs[IN_RUN].isConnected()) {
        int state = runInputTrigger.process(inputs[IN_RUN].getVoltage());
        if (state != 0) {
            if (momentaryRunInput) {
                if (state == 1) {
                    toggleRun();
                }
            }
            else {
                if ( (running && state == -1) || (!running && state == 1) ) {
                    toggleRun();
                }
            }
        }
    }

    updateDebugExpander();

    // Slowed down polling of inputs
    if (refresh.processInputs()) {
        bool rising = inputs[GATE_INPUT].getVoltage() >= 1.0f && old_gate_value <0.1f;  // TODO: replace with bpmDetectTrigger
        bool falling = inputs[GATE_INPUT].getVoltage() < 0.1f && old_gate_value >=1.0f;  // TODO: Zero?
        // ---
        sampleRate = (double)args.sampleRate;
        sampleTime = 1.0 / sampleRate;
        deltaTime = args.sampleTime;
    }

    if (refresh.processLights()){
        if (BPM_locked) {
            lights[BPM_LOCKED].setSmoothBrightness(1.f, 0.3f);
        } else {
            lights[BPM_LOCKED].setSmoothBrightness(0.f, 0.3f);
        }
    }


}


/*
void Ratchets::xx(){
    // Scheduled reset
    if (scheduledReset) {
        resetClocks(false);
        scheduledReset = false;
    }

        BPM = getBPM();

        // Reset (has to be near top because it sets steps to 0, and 0 not a real step (clock section will move to 1 before reaching outputs)
        if (resetTrigger.process(inputs[IN_RESET].getVoltage() + params[RESET_PARAM].getValue())) {
            //resetLight = 1.0f;
            std::cout << "RESET pressed\n";
            resetPulse.trigger(0.001f);
            resetClocks(false);
            current_seq_step = 0;
        }

        // Recalculate MasterLength if needed
        newMasterLength = masterLength;
        if (inputs[GATE_INPUT].isConnected()) {
            bool trigBpmInValue = bpmDetectTrigger.process(inputs[GATE_INPUT].getVoltage());

            // rising edge detect
            if (trigBpmInValue) {
                if (!running) {
                    // this must be the only way to start runnning when in bpmDetectionMode or else
                    //   when manually starting, the clock will not know which pulse is the 1st of a ppqn set
                    running = true;
                    runPulse.trigger(0.001f);
                    resetClocks(false);
                }
                if (running) {
                    extPulseNumber++;
                    if (extPulseNumber >= ppqn * 2)// *2 because working with double_periods
                        extPulseNumber = 0;
                    if (extPulseNumber == 0)// if first pulse, start interval timer
                        extIntervalTime = 0.0;
                    else {
                        // all other ppqn pulses except the first one. now we have an interval upon which to plan a strecth
                        double timeLeft = extIntervalTime * (double)(ppqn * 2 - extPulseNumber) / ((double)extPulseNumber);
                        std::cout << "clk[0].getStep= " << clk[0].getStep() << " timeLeft= " << timeLeft << "isHigh()" << clk[0].isHigh() << "\n";
                        newMasterLength = clamp(clk[0].getStep() + timeLeft, masterLengthMin / 1.5f, masterLengthMax * 1.5f);// extended range for better sync ability (20-450 BPM)
                        timeoutTime = extIntervalTime * ((double)(1 + extPulseNumber) / ((double)extPulseNumber)) + 0.1; // when a second or higher clock edge is received,
                        //  the timeout is the predicted next edge (which is extIntervalTime + extIntervalTime / extPulseNumber) plus epsilon
                    }
                    if (newMasterLength != masterLength) {
                        double lengthStretchFactor = ((double)newMasterLength) / ((double)masterLength);
                        for (int i = 0; i < NUM_CLOCKS; i++) {
                            clk[i].applyNewLength(lengthStretchFactor);
                        }
                        masterLength = newMasterLength;
                    }
                }
            }
            // TODO: Check IN_RUN and PARAM_RUN!
            if (running) {
                extIntervalTime += sampleTime;
                if (extIntervalTime > timeoutTime) {
                    running = false;
                    runPulse.trigger(0.001f);
                    if (emitResetOnStopRun) {
                        resetClocks(false);
                        resetPulse.trigger(0.001f);
                        //resetLight = 1.0f;
                    }
                }
            }

        }
        // Check RATCHET1 for each step in sequencer
        for (int i = 0; i < MAX_SEQUENCER_STEPS; i++) {
            seqClockUsed1[i] = clockValues[(int)params[RATCHETS1+i].getValue()];   // TODO: Should be -1 !!!!
            seqClockUsed2[i] = clockValues[(int)params[RATCHETS2+i].getValue()];
        }

    if (running && BPM_LOCKED) {
        // See if clocks finished their prescribed number of iterations of double periods (and syncWait for sub) or
        //    if they were forced reset and if so, recalc and restart them

        // Master clock
        if (clk[0].isReset()) {
            // See if ratio knobs changed (or unitinialized)
            for (int i = 1; i < NUM_CLOCKS; i++) {
                if (syncRatios[i]) {// unused (undetermined state) for master
                    clk[i].reset();// force reset (thus refresh) of that sub-clock
                    ratiosDoubled[i] = getRatioDoubled(i);
                    syncRatios[i] = false;
                }
            }
            #ifdef DBG
            std::cout << "masterlength: " << masterLength << " BPM: " << BPM << "\n";
            #endif
            clk[0].setup(masterLength, 1, sampleTime);// must call setup before start. length = double_period
            clk[0].start();

        }

        // Sub clocks
        for (int i = 1; i < NUM_CLOCKS; i++) {
            if (clk[i].isReset()) {
                double length;
                int iterations;
                int ratioDoubled = ratiosDoubled[i];
                length = (2.0f * masterLength) / ((double)ratioDoubled);
                iterations = ratioDoubled / (2l - (ratioDoubled % 2l));
                //std::cout << "iterations: " << iterations << " length: " << length << " sampleTime" << sampleTime << "\n";
                clk[i].setup(length, iterations, sampleTime);
                clk[i].start();
            }
        }

        // Step clocks
        for (int i = 0; i < NUM_CLOCKS; i++) {
            clk[i].stepClock();
         }

        bool gateIn = false;
        if (inputs[GATE_INPUT].isConnected()) {
            // External clock
            if (clockTrigger.process(inputs[GATE_INPUT].getVoltage())) {
                setIndex(current_seq_step + 1);
                bernouli_value = random::uniform();
            }

            gateIn = clockTrigger.isHigh();

            // Illuminate sequencer step LED
            for (int i=0; i<MAX_SEQUENCER_STEPS; i++) {
                if (gateTriggers[i].process(params[STEP_LED_PARAM + i].getValue())) {
                    gates[i] = !gates[i];
                }
            }

            int use_steps = MAX_SEQUENCER_STEPS<params[STEPS_PARAM].getValue()?MAX_SEQUENCER_STEPS:params[STEPS_PARAM].getValue();
            for (int i=0; i<use_steps; i++) {
                lights[STEP_LED + i].setSmoothBrightness((gateIn && i == current_seq_step) ? (gates[i] ? 1.f : 0.66) : (gates[i] ? 0.33 : 0.0), 0.3f);
            }
            if (use_steps<MAX_SEQUENCER_STEPS) {
                for (int i=use_steps; i<MAX_SEQUENCER_STEPS; i++) {
                    lights[STEP_LED + i].setSmoothBrightness((gateIn && i == current_seq_step) ? 0.66 : (gates[i] ? 0.33 : 0.0), 0.3f);
                }
            }

            //std::cout << "Voltage:" << params[OCTAVE_SEQ+current_seq_step].getValue()+params[CV_SEQ+current_seq_step].getValue() << " Octave: " << params[OCTAVE_SEQ+current_seq_step].getValue() << " CV: " << params[CV_SEQ+current_seq_step].getValue() << "\n";
            if (outputs[OUT_CV].isConnected()) {
                outputs[OUT_CV].setVoltage(gates[current_seq_step]?params[OCTAVE_SEQ+current_seq_step].getValue()+params[CV_SEQ+current_seq_step].getValue():0.0);
            }

            old_gate_value = inputs[GATE_INPUT].getVoltage();
            // TODO: Calculate based on sample rate instead?
            //now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();

            //std::cout << "current_seq_step=" << current_seq_step << " clock=" << seqClockUsed[current_seq_step] << " isHigh()" << clk[seqClockUsed[current_seq_step]].isHigh(0.0, 0.5) << "\n";
            //std::cout << "Bernouli= " << ((random::uniform() > params[BERNOULI_GATE+current_seq_step].getValue()) ? "A \n" : "B \n");
            //std::cout << "seqClockUsed1[current_seq_step] " << seqClockUsed1[current_seq_step] << " step " << current_seq_step << "\n";
            //outputs[OUT_GATE].setVoltage(gates[current_seq_step]?clk[seqClockUsed1[current_seq_step]].isHigh() ? 10.0f : 0.0f: 0.0f);
            if (gateIn) {
                outputs[OUT_GATE].setVoltage(gates[current_seq_step]?clk[(bernouli_value > params[BERNOULI_GATE+current_seq_step].getValue())?seqClockUsed1[current_seq_step]:seqClockUsed2[current_seq_step]].isHigh() ? 10.0f : 0.0f: 0.0f);
            } else {
                outputs[OUT_GATE].setVoltage(0.0f);
            }

            lights[LIGHT_RUNNING].setSmoothBrightness(1.f, 0.3f);

            // Step clocks
            for (int i = 0; i < NUM_CLOCKS; i++) {
                clk[i].stepClock();
             }

            deltaTime = args.sampleTime;
        } else { // Not running
            lights[LIGHT_RUNNING].setSmoothBrightness(0.f, 0.3f);
        }
    }



}
*/

//void Ratchets::updateDebugExpander() {
//		if (rightExpander.module && rightExpander.module->model == modelDebugExpander) {
//			// Get message from right expander
//			float *message = (float*) rightExpander.module->leftExpander.producerMessage;
//			// Write message
//			for (int i = 0; i < 8; i++) {
//				message[i] = clk[i].isHigh()?1.0f:0.0f;
//			}
//			// Flip messages at the end of the timestep
//			rightExpander.module->leftExpander.messageFlipRequested = true;
//		}
//}


struct RatchetsWidget: ModuleWidget {
    RatchetsWidget(Ratchets *module){
//        if (module){
            setModule(module);

            setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Ratchets.svg")));
            static constexpr int smallRoganOffset = 2;
            static constexpr float LedButtonOffset = 3;

            // Screws
            addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
            addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
            addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
            addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

            // Inputs block
            static constexpr float InputsY = 40.0;
            addInput(createInput<PJ301MPort>(Vec(19, InputsY), module, Ratchets::GATE_INPUT));
            addInput(createInput<PJ301MPort>(Vec(49, InputsY), module, Ratchets::IN_RESET));
            addParam(createParam<RoganSmallBlueSnap>(Vec(83, InputsY + smallRoganOffset), module, Ratchets::STEPS_PARAM));
            addParam(createParam<LEDButton>(Vec(115, InputsY + LedButtonOffset), module, Ratchets::RESET_PARAM));
            addParam(createParam<LEDButton>(Vec(153, InputsY + LedButtonOffset), module, Ratchets::RUN_PARAM));

            //Lights
            addChild(createLight<MediumLight<RedLight>>(Vec(33, InputsY+22), module, Ratchets::BPM_LOCKED));
            addChild(createLight<MediumLight<RedLight>>(Vec(188, InputsY+5), module, Ratchets::LIGHT_RUNNING));

            // Sequencer matrix block
            static constexpr float RowPosY = 29.0f;
            static constexpr float BaseY = 77.0;
            for (int i = 0; i < MAX_SEQUENCER_STEPS; i++) {
                addParam(createParam<LEDButton>(Vec(15 , BaseY + i * RowPosY+3), module, Ratchets::STEP_LED_PARAM + i));
                addChild(createLight<MediumLight<GreenLight>>(Vec(15 + 4.4f, BaseY + i * RowPosY + 4.4f+3), module, Ratchets::STEP_LED + i));

                addParam(createParam<RoganSmallBlueSnap>(Vec(35, BaseY + i * RowPosY + smallRoganOffset), module, Ratchets::RATCHETS1+i));
                addParam(createParam<RoganSmallRed>(Vec(63, BaseY + i * RowPosY + smallRoganOffset), module, Ratchets::BERNOULI_GATE+i));
                addParam(createParam<RoganSmallBlueSnap >(Vec(93, BaseY + i * RowPosY + smallRoganOffset), module, Ratchets::RATCHETS2+i));
                addParam(createParam<RoganSmallGreenSnap>(Vec(150, BaseY + i * RowPosY + smallRoganOffset), module, Ratchets::OCTAVE_SEQ+i));
                addParam(createParam<RoganSmallGreen>(Vec(180, BaseY + i * RowPosY + smallRoganOffset), module, Ratchets::CV_SEQ+i));
            }

            // Outputs block
            static constexpr float OutputsY = 323.0;
            addOutput(createOutput<PJ301MPort>(Vec(149, OutputsY), module, Ratchets::OUT_GATE));
            addOutput(createOutput<PJ301MPort>(Vec(182, OutputsY), module, Ratchets::OUT_CV));
//      }
  }
};


Model *modelRatchets = createModel<Ratchets, RatchetsWidget>("Ratchets");
