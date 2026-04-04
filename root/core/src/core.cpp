#include "core.h"
Core::Core(): _l("Cmake Template") {
}

Core::~Core() {}

void Core::load() {
    _l.info("Core loaded.");
}

void Core::unload() {
    _l.info("Core unloaded");
}