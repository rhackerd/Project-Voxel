#include "log/log.hpp"
#include "render/graphics.hpp"
#include <SDL3/SDL.h>

int main() {
	N::Graphics::Graphics gfx;

	gfx.SetVsync(false);

	auto* obj = gfx.AddObject();

	obj->LoadModel("DamagedHelmet.glb");
	obj->setScale({1,-1,1});


	while (gfx.Tick()) {
		// Here changes like moving the object or camera
		// obj->move(0.00001, 0, 0);
		obj->rotate(0.001, 0.0f, 0.0f);
	};

    return 0;
}