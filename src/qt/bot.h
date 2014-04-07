#ifndef BOT_H
#define BOT_H

#include <stdio.h>
#include "../gamestate.h"
#include <string>
#include "util.h"
#include <iostream>
#include "botHelper.h"

struct Bot {

	int maxLoot;
	std::string mode;
	std::string name;
	unsigned color;
	std::vector<Game::Coord> dests;

	IMPLEMENT_SERIALIZE (
		READWRITE(maxLoot);
		READWRITE(mode);
        READWRITE(name);
		READWRITE(color);
		READWRITE(dests);
    )

	Bot();

	std::vector<Game::Coord> calculate(const Game::PlayerState &playerState,const Game::GameState &gameState);

	std::vector<Game::Coord> getGatherAfkCoords(const Game::PlayerState &playerState,const Game::GameState &gameState);

};


#endif

