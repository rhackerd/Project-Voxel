#ifndef CORE_HPP
#define CORE_HPP

#include <nova/logger/logger.hpp>


class Core {
    private:
        Nova::Logger _l;
    public:
        Core();
        ~Core();

        void load();
        void unload();
};

struct ModuleContext {
    Nova::Logger logger = Nova::Logger("Module");
    
};

#endif