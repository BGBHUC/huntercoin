#ifndef BOTHELPER_H
#define BOTHELPER_H

#include <stdio.h>
#include "../gamestate.h"
#include <string>
#include "util.h"
#include "bot.h"
#include "botConfig.h"
#include <iostream>


struct BotHelper {
	public:
		static bool coordIsSpawn(Game::Coord pos);
		static Game::Coord getHomeCoord(unsigned color);
		static std::string to_string(int number);
		static BotConfig getConfig();
};

#endif

