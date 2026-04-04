#include <fmt/format.h>
#include <stdio.h>
#include <dlfcn.h>
#include <filesystem>
#include <chrono>
#include <thread>
#include <core.h>

void* handle = nullptr;
Nova::Logger logger("Main");

void load_module() {
    const char* path = "../lib/libmodule.so";


    while (!std::filesystem::exists(path)) {
        logger.info("Waiting for module to be built...");
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    while (true) {

        void* new_handle = dlopen(path, RTLD_NOW);

        if (new_handle) {
            handle = new_handle;
            break;
        }

        logger.warn(std::string("dlopen failed: ") + dlerror());
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

ModuleContext * core_ptr = nullptr;

int main() {
    logger.info("Starting application...");
    const char* module_path = "../lib/libmodule.so";

    Core core;
    core.load();

    core_ptr = new ModuleContext{};

    load_module();
    logger.info("Module loaded.");
    auto last_write = std::filesystem::last_write_time(module_path);

    using update_fn = void(*)();
    update_fn plugin_update = (update_fn)dlsym(handle, "plugin_update");
    using init_fn = void(*)(ModuleContext*);
    init_fn plugin_init = (init_fn)dlsym(handle, "plugin_init");

    while (true) {


        plugin_update();

        if (std::filesystem::exists(module_path)) {
            auto cur = std::filesystem::last_write_time(module_path);
            if (cur != last_write) {

                last_write = cur;

                logger.info("Reloading module...");

                dlclose(handle);
                load_module();
                plugin_update = (update_fn)dlsym(handle, "plugin_update");
                plugin_init = (init_fn)dlsym(handle, "plugin_init");
                plugin_init(core_ptr);
                logger.info("Module reloaded.");
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    core.unload();
    return 0;
}
