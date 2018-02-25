#include "JLmod.hpp"
//#include "VAStateVariableFilter.h"

Plugin *plugin;

void init(rack::Plugin *p) {
	plugin = p;
	p->slug = "JLmod";
#ifdef VERSION
	p->version = TOSTRING(VERSION);
#endif
	p->website = "https://github.com/JoakimLindbom/vcvrack";

    // Utililites
    //p->addModel(createModel<TimersWidget>("JLmod", "Timers", "Timers", UTILITY_TAG));
    p->addModel(modelTimers);
}
