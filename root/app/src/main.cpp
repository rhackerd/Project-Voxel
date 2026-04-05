#include "render/graphics.hpp"
#include <SDL3/SDL.h>

int main() {
	N::Graphics::Graphics gfx;
	gfx.Init();

	gfx.SetVsync(true);
	while (gfx.Tick()) {};

	gfx.Shutdown();

    return 0;
}