#pragma once

#include <string>
#include <SDL2/SDL.h>

#include "sky/image.hpp"

namespace sky {

	class screen : public image	{

		public:
			screen(int width, int height);
	};

}

