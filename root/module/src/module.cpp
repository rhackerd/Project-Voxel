#include "module.hpp"
#include <core.h>

ModuleContext * core_ptr = nullptr;

extern "C" void plugin_update() {
    //core_ptr->logger.info("Plugin update called.");
}

extern "C" void plugin_init(ModuleContext * core_ptr_) {
    core_ptr = core_ptr_;
};