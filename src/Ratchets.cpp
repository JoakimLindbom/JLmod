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

// TODO: Add quad pan options. Pan: Start left/right for each step. Different colour on LED
// TODO: Separate base clocks (w. option) - one drive sequencer, one drives ratchets speed
// TODO: Add BPM detect

struct Ratchets : Module {
    enum ParamIds {
    RESET_PARAM,                                // Reset button
    RUN_PARAM,                                  // Run sequencer?
    STEPS_PARAM,                                // How many step to execute
    // Sequencer matrix
    ENUMS(STEP_LED_PARAM, MAX_SEQUENCER_STEPS), // Enable/disable step
    ENUMS(RATCHETS1, MAX_SEQUENCER_STEPS),      // How many sub gates to trigger
    ENUMS(BERNOULI_GATE, MAX_SEQUENCER_STEPS),  // Bernouli gate value - use RATCHET1 or RATCHET2 value?
    ENUMS(RATCHETS2, MAX_SEQUENCER_STEPS),      // How many sub gates to trigger
    ENUMS(OCTAVE_SEQ, MAX_SEQUENCER_STEPS),     // Output note: Octave
    ENUMS(CV_SEQ, MAX_SEQUENCER_STEPS),         // Output note: Note
    ENUMS(STEP_PAN_PARAM, MAX_SEQUENCER_STEPS),        // Enable/disable panning for this step
    ENUMS(STEP_PAN_SPREAD_PARAM, MAX_SEQUENCER_STEPS), // Panning left-right spread for this step
    OCTAVE_PARAM,                                      // Base octave
    PAN_UNI_BI_SWITCH,                                 // Select Unipolar or bipolar output for the stereo panner
    NUM_PARAMS
    };
    enum InputIds {
    BPM_INPUT,                                  // External clock input
    RUN_INPUT,                                  // External run input
    RESET_INPUT,                                // External reset input
    OCT_INPUT,                                  // External base octave input
    NUM_INPUTS
    };
    enum OutputIds {
    SPAN_OUT,                                   // Stereo pan out
    OUT_GATE,                                   // Gate out
    OUT_CV,                                     // CV Out
    NUM_OUTPUTS
    };
    enum LightIds {
    RUN_LIGHT,
    RESET_LIGHT,
    ENUMS(STEP_LED, MAX_SEQUENCER_STEPS),
    ENUMS(PAN_LED, MAX_SEQUENCER_STEPS),
    BPM_LOCKED,
    NUM_LIGHTS
    };

    // Expander
    float rightMessages[2][26] = {};// messages from expander

    float phase = 0.0;
    float blinkPhase = 0.0;

    bool running = true;

    long editingBpmMode;// 0 when no edit bpmMode, downward step counter timer when edit, negative upward when show can't edit ("--")
    double sampleRate = 0.0;
    double sampleTime = 0.0;
    float deltaTime = 0.0f;
    double length = 0.0f;  // double period
    double stepCount = 0.0f;

    bool scheduledReset = false;
    long cantRunWarning = 0l;// 0 when no warning, positive downward step counter timer when warning
    RefreshCounter refresh;
    float resetLight = 0.0f;

    static constexpr float pulseWidth = 0.011f;  // 11 ms used by SEQ-3 for gate

    uint64_t duration_between_ticks = 0;
    // calculated time between clock ticks
    uint64_t tick1_ms = 0;
    uint64_t tick2_ms = 0;
    uint64_t now = 0;
    bool waiting_for_1st_tick = true;
    bool waiting_for_2nd_tick = false;

    //float old_gate_value = 0.0f;

    int misses;

    float timer = 0.0;
    float seconds = 0.0;
    float deltaT;
    float BPM=-1.0;
    float oldBPM=0.0;
    bool BPM_locked = false;
    dsp::SchmittTrigger gateTrigger;
    dsp::SchmittTrigger runTrigger;
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
    //Clock seq_clk;
    //Clock rat_clk;
    const float ratioValues[NUM_CLOCKS] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
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
    Trigger runInputTrigger;


    // Sequencer
    int current_seq_step = 0; // -1;
    bool gates[MAX_SEQUENCER_STEPS] = {};
    bool pan[MAX_SEQUENCER_STEPS] = {};
    dsp::SchmittTrigger gateTriggers[MAX_SEQUENCER_STEPS];
    dsp::SchmittTrigger panTriggers[MAX_SEQUENCER_STEPS];
    const float clockValues[NUM_CLOCKS+1] = {0, 1, 2, 3, 4, 5, 6, 7, 8};  // Used to force integer selection  // TODO: Remove extra step
    int seqClockUsed1[MAX_SEQUENCER_STEPS];
    int seqClockUsed2[MAX_SEQUENCER_STEPS];
    float bernouli_value;
    float base_octave = 0.0f;
    float span_pos = -5.0f; // Stereo pan position, start left

    Ratchets() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(Ratchets::STEPS_PARAM, 1.0, MAX_SEQUENCER_STEPS, MAX_SEQUENCER_STEPS, "# steps");
        configParam(Ratchets::RESET_PARAM, 0.0f, 1.0f, 0.0f, "Reset");
        configParam(Ratchets::RUN_PARAM, 0.0f, 1.0f, 0.0f, "Run");
        for (int i=0; i<MAX_SEQUENCER_STEPS; i++) {
            configParam(Ratchets::STEP_LED_PARAM + i, 0.0f, 1.0f, 0.0f, "Run step?");
            configParam(Ratchets::RATCHETS1+i, 1.0, NUM_CLOCKS, 1.0, "Ratchet patter 1");
            configParam(Ratchets::BERNOULI_GATE+i, 0.0, 1.0, 0.0, "Probability for pattern 2");
            configParam(Ratchets::RATCHETS2+i, 1.0, NUM_CLOCKS, 1.0, "Ratchet patter 2");
            configParam(Ratchets::STEP_PAN_PARAM + i, 0.0f, 1.0f, 0.0f, "Pan step?");
            configParam(Ratchets::STEP_PAN_SPREAD_PARAM + i, 0.0f, 1.0f, 0.001f, "Pan spread left-right");
            configParam(Ratchets::OCTAVE_SEQ+i, 1.0, 7.0, 3.0, "Octave out knob");
            configParam(Ratchets::CV_SEQ+i, 0.0, 1.0, 0.01, "CV out knob");
        }
        configParam(Ratchets::OCTAVE_PARAM, 1.0, 10.0, 1.0, "Base octave");
        configParam(Ratchets::PAN_UNI_BI_SWITCH, 0.0, 1.0, 1.0, "Uni or bipolar");
        params[PAN_UNI_BI_SWITCH].setValue(0.0f); // Bi-polar TODO: Move to onInit??

        //seq_clk.construct(NULL, &resetClockOutputsHigh);
        //rat_clk.construct(NULL, &resetClockOutputsHigh);

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
				message[i] = clk[i].isHigh()?1.0f:0.0f;          // 0-7 Clocks - LEDs
				message[i+9] = clk[i].isHigh()?10.0f:0.0f;       // 9-16 Clocks - Gates
				message[i+17] = current_seq_step==i?10.0f:0.0f;  // 17-24 Sequencer Steps
			}
			message[8] = (60.0f / masterLength) + 0.5f;     // BPM
			message[25] = (float)current_seq_step;
			//message[26] = "BPM"; /TODO: Check how Stoermelder sends text labels to expander

		    // Flip messages at the end of the timestep
			rightExpander.module->leftExpander.messageFlipRequested = true;
		}
    }

    inline bool checkBeat(float timer, int mult) {
        return ( ((timer - mult*seconds) * (timer - mult*seconds) / (seconds*seconds) < 0.2 ) && misses < 4);
    }

    json_t *dataToJson() override { //TODO: Add pan
		json_t *rootJ = json_object();

		//json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));

		// Sequencer steps
        //char key[10];
		json_t* gatesJ = json_array();
		for (int i=0;i<MAX_SEQUENCER_STEPS; i++) {
		    json_array_insert_new(gatesJ, i, json_integer((int) gates[i]));
		}
		json_object_set_new(rootJ, "gates", gatesJ);

		json_t* pansJ = json_array();
		for (int i=0;i<MAX_SEQUENCER_STEPS; i++) {
		    json_array_insert_new(pansJ, i, json_integer((int) pan[i]));
		}
		json_object_set_new(rootJ, "pans", pansJ);

		return rootJ;
	}


	void dataFromJson(json_t *rootJ) override {
		// panelTheme
		//json_t *panelThemeJ = json_object_get(rootJ, "panelTheme");
		//if (panelThemeJ)
		//	panelTheme = json_integer_value(panelThemeJ);

		// Sequencer steps
        //char key[10];
		//for (int i=0;i<MAX_SEQUENCER_STEPS; i++) {
		//    sprintf(key, "SEQ-1-%0d", i);
		//    json_t *on = json_object_get(rootJ, key);
        //    if (on)
        //        gates[i] = json_is_true(on);
        //}
		json_t* gatesJ = json_object_get(rootJ, "gates");
		if (gatesJ) {
			for (int i = 0; i < MAX_SEQUENCER_STEPS; i++) {
				json_t* gateJ = json_array_get(gatesJ, i);
				if (gateJ)
					gates[i] = !!json_integer_value(gateJ); //TODO: Check !!
			}
		}

		json_t* pansJ = json_object_get(rootJ, "pans");
		if (pansJ) {
			for (int i = 0; i < MAX_SEQUENCER_STEPS; i++) {
				json_t* panJ = json_array_get(pansJ, i);
				if (panJ)
					pan[i] = !!json_integer_value(panJ); //TODO: Check !!
			}
		}

		resetNonJson(true);
	}

  // For more advanced Module features, read Rack's engine.hpp header file
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
		running = true;
		bpmDetectionMode = false;
		restartOnStopStartRun = 0;
		sendResetOnRestart = false;
		ppqn = 4;
		resetClockOutputsHigh = true;
		momentaryRunInput = true;
		//displayIndex = 0;// show BPM (knob 0) by default
		resetNonJson(false);
    }

	void resetNonJson(bool delayed) {// delay thread sensitive parts (i.e. schedule them so that process() will do them)
		editingBpmMode = 0l;
		if (delayed) {
			scheduledReset = true;// will be a soft reset
		}
		else {
			resetRatchets(true);// hard reset
		}
	}

    void onRandomize() override {
        for (int i = 0; i < MAX_SEQUENCER_STEPS; i++) {
            gates[i] = (random::uniform() > 0.5f);
            pan[i] = (random::uniform() > 0.5f);
        }
    }

/*
    float getBPM() {
      bool rising = inputs[BPM_INPUT].getVoltage() >= 1.0f && old_gate_value <0.1f;  // TODO: Zero?
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
*/

	int getRatioDoubled(int Index) {
	    int ret = 1;
        if (Index < 1) // Master clock
            return 1;

        if (Index <= NUM_CLOCKS) {
            ret = (int) (ratioValues[Index] * 2.0f + 0.5f);
        } else {
            ret = 1;
        }
            return ret;
	}

	void setIndex(int index) {
		int numSteps = MAX_SEQUENCER_STEPS; // (int) clamp(roundf(params[STEPS_PARAM].getValue() + inputs[STEPS_INPUT].getVoltage()), 1.0f, 8.0f);
		//phase = 0.f;
		this->current_seq_step = index;
		if (this->current_seq_step >= MAX_SEQUENCER_STEPS || this->current_seq_step >= params[STEPS_PARAM].getValue())
			this->current_seq_step = 0;
	}

	void resetRatchets(bool hardReset) {// set hardReset to true to revert learned BPM to 120 in sync mode, or else when false, learned bmp will stay persistent
		sampleRate = (double)(APP->engine->getSampleRate());
		//seq_clk.setSampleRate(sampleRate);
		//rat_clk.setSampleRate(sampleRate);
		sampleTime = 1.0 / sampleRate;
		//seq_clk.reset();
		//rat_clk.reset();
		for (int i = 0; i < NUM_CLOCKS; i++) {
			clk[i].reset();
			syncRatios[i] = false;
			ratiosDoubled[i] = getRatioDoubled(i);
		}
		extPulseNumber = -1;
		extIntervalTime = 0.0;
		timeoutTime = 2.0 / ppqn + 0.1;// worst case. This is a double period at 30 BPM (4s), divided by the expected number of edges in the double period
									   //   which is 2*ppqn, plus epsilon. This timeoutTime is only used for timingout the 2nd clock edge
		if (inputs[BPM_INPUT].isConnected()) {
			if (bpmDetectionMode) {
				if (hardReset)
					newMasterLength = 0.5f;// 120 BPM
			}
			else
				newMasterLength = 0.5f / std::pow(2.0f, inputs[BPM_INPUT].getVoltage());// bpm = 120*2^V, T = 60/bpm = 60/(120*2^V) = 0.5/2^V
		}
		else
			newMasterLength = 60.0f;
		newMasterLength = clamp(newMasterLength, masterLengthMin, masterLengthMax);
		masterLength = newMasterLength;
	}

    void toggleRun(void) {
    if (!(bpmDetectionMode && inputs[BPM_INPUT].isConnected()) || running) {// toggle when not BPM detect, turn off only when BPM detect (allows turn off faster than timeout if don't want any trailing beats after stoppage). If allow manually start in bpmDetectionMode   the clock will not know which pulse is the 1st of a ppqn set, so only allow stop
        running = !running;
        runPulse.trigger(0.001f);
        if (!running && restartOnStopStartRun == 1) {
            resetRatchets(false); //TODO: Check
            if (sendResetOnRestart) {
                resetPulse.trigger(0.001f);
                resetLight = 1.0f;
            }
        }
        if (running && restartOnStopStartRun == 2) {
            resetRatchets(false); //TODO: Check
            if (sendResetOnRestart) {
                resetPulse.trigger(0.001f);
                resetLight = 1.0f;
            }
        }
    }
    else {
        cantRunWarning = (long) (0.7 * sampleRate / RefreshCounter::displayRefreshStepSkips); //TODO: Remove
    }
}


}; // Module


// Main DSP loop
void Ratchets::process(const ProcessArgs& args) {

    // Scheduled reset
    if (scheduledReset) {
        resetRatchets(false);
        scheduledReset = false;
    }

    // Run button
    if (runButtonTrigger.process(params[RUN_PARAM].getValue())) {
        toggleRun();
    }

    // Run input
    if (inputs[RUN_INPUT].isConnected() && runInputTrigger.process(inputs[RUN_INPUT].getVoltage())) {
        toggleRun();
    }

    // Reset (has to be near top because it sets steps to 0, and 0 not a real step (clock section will move to 1 before reaching outputs)
    if (resetTrigger.process(inputs[RESET_INPUT].getVoltage() + params[RESET_PARAM].getValue())) {
        resetLight = 1.0f;
        resetPulse.trigger(0.001f);
        resetRatchets(false);
        current_seq_step = 0;
    }

    if (refresh.processInputs()) {
        // Check pattern selectors for each step in sequencer
        for (int i = 0; i < MAX_SEQUENCER_STEPS; i++) {
            seqClockUsed1[i] = clockValues[(int)params[RATCHETS1+i].getValue()];   // TODO: Should be -1 !!!!
            seqClockUsed2[i] = clockValues[(int)params[RATCHETS2+i].getValue()];
        }

        if (inputs[OCT_INPUT].isConnected()) {
            base_octave = inputs[OCT_INPUT].getVoltage();
        } else {
            base_octave = params[OCTAVE_PARAM].getValue();
        }
    }// userInputs refresh

    // BPM input and knob
    newMasterLength = masterLength;
    if (inputs[BPM_INPUT].isConnected()) {
        //bool trigBpmInValue = bpmDetectTrigger.process(inputs[BPM_INPUT].getVoltage());

        //if (clockTrigger.process(clk[0].)) {
        if (clk[0].Tick()) {
            setIndex(current_seq_step + 1);
            bernouli_value = random::uniform();
        }

        // BPM Detection method
/*        if (bpmDetectionMode) {
            // rising edge detect
            if (trigBpmInValue) {
                if (!running) {
                    // this must be the only way to start runnning when in bpmDetectionMode or else
                    //   when manually starting, the clock will not know which pulse is the 1st of a ppqn set
                    running = true;
                    runPulse.trigger(0.001f);
                    resetRatchets(false);
                    if (restartOnStopStartRun == 2) {
                        // resetRatchets(false); implicit above
                        if (sendResetOnRestart) {
                            resetPulse.trigger(0.001f);
                            //resetLight = 1.0f;
                        }
                    }
                }
                if (running) {
                    extPulseNumber++;
                    if (extPulseNumber >= ppqn)
                        extPulseNumber = 0;
                    if (extPulseNumber == 0)// if first pulse, start interval timer
                        extIntervalTime = 0.0;
                    else {
                        // all other ppqn pulses except the first one. now we have an interval upon which to plan a strecth
                        double timeLeft = extIntervalTime * (double)(ppqn - extPulseNumber) / ((double)extPulseNumber);
                        newMasterLength = clamp(clk[0].getStep() + timeLeft, masterLengthMin / 1.5f, masterLengthMax * 1.5f);// extended range for better sync ability (20-450 BPM)
                        timeoutTime = extIntervalTime * ((double)(1 + extPulseNumber) / ((double)extPulseNumber)) + 0.1; // when a second or higher clock edge is received,
                        //  the timeout is the predicted next edge (whici is extIntervalTime + extIntervalTime / extPulseNumber) plus epsilon
                    }
                }
            }
            if (running) {
                extIntervalTime += sampleTime;
                if (extIntervalTime > timeoutTime) {
                    running = false;
                    runPulse.trigger(0.001f);
                    if (restartOnStopStartRun == 1) {
                        resetRatchets(false);
                        if (sendResetOnRestart) {
                            resetPulse.trigger(0.001f);
                            //resetLight = 1.0f;
                        }
                    }
                }
            }
        }
        // BPM CV method
        else {// bpmDetectionMode not active*/
            newMasterLength = clamp(0.5f / std::pow(2.0f, inputs[BPM_INPUT].getVoltage()), masterLengthMin, masterLengthMax);// bpm = 120*2^V, T = 60/bpm = 60/(120*2^V) = 0.5/2^V
            // no need to round since this clocked's master's BPM knob is a snap knob thus already rounded, and with passthru approach, no cumul error
       // }
    }
    else {// BPM_INPUT not active
        // TDOD: Check 0f if it's right!!! newMasterLength = clamp(60.0f / bufferedKnobs[3], masterLengthMin, masterLengthMax);
        newMasterLength = 60.0f; // clamp(60.0f / 1.0f, masterLengthMin, masterLengthMax);
    }
    if (newMasterLength != masterLength) {
        double lengthStretchFactor = ((double)newMasterLength) / ((double)masterLength);
        //seq_clk.applyNewLength(lengthStretchFactor);
        //rat_clk.applyNewLength(lengthStretchFactor);
        for (int i = 0; i < 4; i++) { // Why 4??? MAX_SEQSTEPS
            clk[i].applyNewLength(lengthStretchFactor);
        }
        masterLength = newMasterLength;
    }


    // main clock engine
    if (running) {
        // See if clocks finished their prescribed number of iteratios of double periods (and syncWait for sub) or
        //    if they were forced reset and if so, recalc and restart them

        // Master clock
        if (clk[0].isReset()) {
            // See if ratio knobs changed (or unitinialized)
            for (int i = 0; i < MAX_SEQUENCER_STEPS-1; i++) { // -1 to safe guard
                if (syncRatios[i]) {// unused (undetermined state) for master
                    clk[i + 1].reset();// force reset (thus refresh) of that sub-clock
                    ratiosDoubled[i] = getRatioDoubled(i);
                    syncRatios[i] = false;
                }
            }
            //seq_clk.setup(masterLength, 1, sampleTime);// must call setup before start. length = double_period
            //seq_clk.start();
            //rat_clk.setup(masterLength, 1, sampleTime);
            //rat_clk.start();
            clk[0].setup(masterLength, 1, sampleTime);// must call setup before start. length = double_period
            clk[0].start();

        }

        // Sub clocks
        for (int i = 1; i < MAX_SEQUENCER_STEPS; i++) {
            if (clk[i].isReset()) {
                double length;
                int iterations;
                int ratioDoubled = ratiosDoubled[i - 1];
                if (ratioDoubled < 0) { // if div
                    ratioDoubled *= -1;
                    length = masterLength * ((double)ratioDoubled) / 2.0;
                    iterations = 1l + (ratioDoubled % 2);
                }
                else {// mult
                    length = (2.0f * masterLength) / ((double)ratioDoubled);
                    iterations = ratioDoubled / (2l - (ratioDoubled % 2l));
                }
                clk[i].setup(length, iterations, sampleTime);
                clk[i].start();
            }
        }

        // Step clocks
        //seq_clk.stepClock();
        //rat_clk.stepClock();
        for (int i = 0; i < MAX_SEQUENCER_STEPS; i++)
            clk[i].stepClock();
    }

    bool gateIn = true; // TODO: Remove

    for (int i=0; i<MAX_SEQUENCER_STEPS; i++) {
        if (gateTriggers[i].process(params[STEP_LED_PARAM + i].getValue())) {
            gates[i] = !gates[i];
        }
        if (panTriggers[i].process(params[STEP_PAN_PARAM + i].getValue())) {
            pan[i] = !pan[i];
        }
    }
    // --> seq lights

    //std::cout << "Voltage:" << params[OCTAVE_SEQ+current_seq_step].getValue()+params[CV_SEQ+current_seq_step].getValue() << " Octave: " << params[OCTAVE_SEQ+current_seq_step].getValue() << " CV: " << params[CV_SEQ+current_seq_step].getValue() << "\n";
    if (outputs[OUT_CV].isConnected()) {
        outputs[OUT_CV].setVoltage(gates[current_seq_step]?params[OCTAVE_SEQ+current_seq_step].getValue()+params[CV_SEQ+current_seq_step].getValue() + base_octave:0.0);
    }

    if (outputs[SPAN_OUT].isConnected() && gates[current_seq_step] && pan[current_seq_step]) {
        if (span_pos == 0.0f) {
            span_pos = 5.0f; // TODO: Check softer - 7.5?
        }
        //span_pos = (random::uniform()-0.5f)*10.0f; // Random spread
        //span_pos = random::uniform()>0.5f?-10.0f:10.0f; // Random left/right
        if (clk[(bernouli_value > params[BERNOULI_GATE+current_seq_step].getValue())?seqClockUsed1[current_seq_step]:seqClockUsed2[current_seq_step]].Tick()) {
            span_pos *=-1.0f;
            //std::cout << "pos: " << span_pos*params[STEP_PAN_SPREAD_PARAM+current_seq_step].getValue() << "\n";
        }
        outputs[SPAN_OUT].setVoltage(span_pos*params[STEP_PAN_SPREAD_PARAM+current_seq_step].getValue()+(int(params[PAN_UNI_BI_SWITCH].getValue())==1?5.0f:0.0f));
        //std::cout << "spread: " << params[STEP_PAN_SPREAD_PARAM+current_seq_step].getValue() << "\n";
        //std::cout << "pos: " << span_pos*params[STEP_PAN_SPREAD_PARAM+current_seq_step].getValue() << "uni/bi: " << (int(params[PAN_UNI_BI_SWITCH].getValue())==0?5:0) << "\n";
        //std::cout << "pos: " << span_pos*params[STEP_PAN_SPREAD_PARAM+current_seq_step].getValue() << "\n";
    } else {
        span_pos = 0.0f;
        outputs[SPAN_OUT].setVoltage(0.0f);
    }


    if (gateIn) {
        outputs[OUT_GATE].setVoltage(gates[current_seq_step]?clk[(bernouli_value > params[BERNOULI_GATE+current_seq_step].getValue())?seqClockUsed1[current_seq_step]:seqClockUsed2[current_seq_step]].isHigh() ? 10.0f : 0.0f: 0.0f);
    } else {
        outputs[OUT_GATE].setVoltage(0.0f);
    }

    // lights
    if (refresh.processLights()) {
        // Reset light
        lights[RESET_LIGHT].setSmoothBrightness(resetLight, (float)sampleTime * (RefreshCounter::displayRefreshStepSkips >> 2));
        resetLight = 0.0f;

        // Run light
        lights[RUN_LIGHT].setBrightness(running ? 1.0f : 0.0f);

        for (int i = 0; i< MAX_SEQUENCER_STEPS;i++){
            //lights[PAN_LED + i].setSmoothBrightness(pan[i] ? 1.0f : 0.0f, 0.0f);
        }

        int use_steps = MAX_SEQUENCER_STEPS<params[STEPS_PARAM].getValue()?MAX_SEQUENCER_STEPS:params[STEPS_PARAM].getValue();
        for (int i=0; i<use_steps; i++) {
            lights[STEP_LED + i].setSmoothBrightness((gateIn && i == current_seq_step) ? (gates[i] ? 1.f : 0.77) : (gates[i] ? 0.44 : 0.0), 0.44f);
            lights[PAN_LED + i].setSmoothBrightness( (pan[i] ? 0.44 : 0.0), 0.44f);
        }
        if (use_steps<MAX_SEQUENCER_STEPS) {
            for (int i=use_steps; i<MAX_SEQUENCER_STEPS; i++) {
                lights[STEP_LED + i].setSmoothBrightness((gateIn && i == current_seq_step) ? 0.77 : (gates[i] ? 0.44 : 0.0), 0.44f);
                lights[PAN_LED + i].setSmoothBrightness( (pan[i] ? 0.44 : 0.0), 0.44f);
            }
        }

        // BPM light
        bool warningFlashState = true;

        if (cantRunWarning > 0l)
            cantRunWarning--;

        editingBpmMode--;
        if (editingBpmMode < 0l)
            editingBpmMode = 0l;

        updateDebugExpander();

        if (BPM_locked) {
            lights[BPM_LOCKED].setSmoothBrightness(1.f, 0.3f);
        } else {
            lights[BPM_LOCKED].setSmoothBrightness(0.f, 0.3f);
        }
    }// lightRefreshCounter
} // process() Main DSP loop

/*
void Ratchets::xx(){

        bool rising = inputs[BPM_INPUT].getVoltage() >= 1.0f && old_gate_value <0.1f;  // TODO: replace with bpmDetectTrigger
        bool falling = inputs[BPM_INPUT].getVoltage() < 0.1f && old_gate_value >=1.0f;  // TODO: Zero?
        // ---
        sampleRate = (double)args.sampleRate;
        sampleTime = 1.0 / sampleRate;
        deltaTime = args.sampleTime;

        // Recalculate MasterLength if needed
        newMasterLength = masterLength;
        if (inputs[BPM_INPUT].isConnected()) {
            bool trigBpmInValue = bpmDetectTrigger.process(inputs[BPM_INPUT].getVoltage());

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
        }
    }
} */

struct RatchetsWidget: ModuleWidget {
    RatchetsWidget(Ratchets *module){
        setModule(module);

        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Ratchets.svg")));
        static constexpr int smallRoganOffset = 2;
        static constexpr float LedButtonOffset = 12;

        // Screws
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // Inputs block
        static float Row_0 = mm2px(13);
        addInput(createInput<PJ301MPort>(Vec(mm2px(4.5), Row_0), module, Ratchets::BPM_INPUT));

        addInput(createInput<PJ301MPort>(Vec(mm2px(17), Row_0), module, Ratchets::RUN_INPUT));
        addParam(createParamCentered<LEDBezel>(Vec(mm2px(31), Row_0 + LedButtonOffset), module, Ratchets::RUN_PARAM));
        addChild(createLightCentered<MuteLight<GreenLight>>(Vec(mm2px(31), Row_0 + LedButtonOffset), module, Ratchets::RUN_LIGHT));

        addInput(createInput<PJ301MPort>(Vec(mm2px(40), Row_0), module, Ratchets::RESET_INPUT));
        addParam(createParamCentered<LEDBezel>(Vec(mm2px(54), Row_0 + LedButtonOffset), module, Ratchets::RESET_PARAM));
        addChild(createLightCentered<MuteLight<GreenLight>>(Vec(mm2px(54), Row_0 + LedButtonOffset), module, Ratchets::RESET_LIGHT));

        addParam(createParam<RoganSmallBlueSnap>(Vec(mm2px(64.5), Row_0 + smallRoganOffset), module, Ratchets::STEPS_PARAM));

        //Lights
        //addChild(createLight<MediumLight<RedLight>>(Vec(33, Row_0+22), module, Ratchets::BPM_LOCKED));

        // Sequencer matrix block
        static constexpr float Matrix_Y_spacing = 28.0f;
        static constexpr float Row_1 = 91.0;
        for (int i = 0; i < MAX_SEQUENCER_STEPS; i++) {
            addParam(createParam<LEDButton>(Vec(mm2px(3) , Row_1 + i * Matrix_Y_spacing+3), module, Ratchets::STEP_LED_PARAM + i));
            addChild(createLight<MediumLight<GreenLight>>(Vec(mm2px(3) + 4, Row_1 + i * Matrix_Y_spacing + 7), module, Ratchets::STEP_LED + i));

            addParam(createParam<RoganSmallBlueSnap>(Vec(mm2px(12), Row_1 + i * Matrix_Y_spacing + smallRoganOffset), module, Ratchets::RATCHETS1+i));
            addParam(createParam<RoganSmallRed>(Vec(mm2px(19), Row_1 + i * Matrix_Y_spacing + smallRoganOffset), module, Ratchets::BERNOULI_GATE+i));
            addParam(createParam<RoganSmallBlueSnap >(Vec(mm2px(26), Row_1 + i * Matrix_Y_spacing + smallRoganOffset), module, Ratchets::RATCHETS2+i));

            addParam(createParam<LEDButton>(Vec(mm2px(39), Row_1 + i * Matrix_Y_spacing+3), module, Ratchets::STEP_PAN_PARAM + i));
            addChild(createLight<MediumLight<GreenLight>>(Vec(mm2px(39) + 4, Row_1 + i * Matrix_Y_spacing + 7), module, Ratchets::PAN_LED + i));
            addParam(createParam<RoganSmallBlue>(Vec(mm2px(47), Row_1 + i * Matrix_Y_spacing + smallRoganOffset), module, Ratchets::STEP_PAN_SPREAD_PARAM+i));

            addParam(createParam<RoganSmallGreenSnap>(Vec(mm2px(60), Row_1 + i * Matrix_Y_spacing + smallRoganOffset), module, Ratchets::OCTAVE_SEQ+i));
            addParam(createParam<RoganSmallGreen>(Vec(mm2px(69), Row_1 + i * Matrix_Y_spacing + smallRoganOffset), module, Ratchets::CV_SEQ+i));
        }

        // Last row - incl. outputs block
        static constexpr float Row_Last = 333.0;
        addInput(createInput<PJ301MPort>(Vec(mm2px(4.5),Row_Last), module, Ratchets::OCT_INPUT));
        addParam(createParam<RoganSmallBlueSnap>(Vec(mm2px(15),Row_Last+2), module, Ratchets::OCTAVE_PARAM));

        addParam(createParam<CKSS>(Vec(mm2px(29), Row_Last-1), module, Ratchets::PAN_UNI_BI_SWITCH));
        addOutput(createOutput<PJ301MPort>(Vec(mm2px(38.75), Row_Last), module, Ratchets::SPAN_OUT));

        addOutput(createOutput<PJ301MPort>(Vec(mm2px(57), Row_Last), module, Ratchets::OUT_GATE));
        addOutput(createOutput<PJ301MPort>(Vec(mm2px(68.75), Row_Last), module, Ratchets::OUT_CV));
  }
};

Model *modelRatchets = createModel<Ratchets, RatchetsWidget>("Ratchets");
