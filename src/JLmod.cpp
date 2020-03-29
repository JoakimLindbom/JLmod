#include "JLmod.hpp"


Plugin *pluginInstance;


void init(Plugin *p) {
	pluginInstance = p;

	// Add all Models defined throughout the pluginInstance
	p->addModel(modelRatchets);
	//p->addModel(modelTimers);
	p->addModel(modelDebugExpander);
    p->addModel(modelKeySplit);

	// Any other pluginInstance initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}
