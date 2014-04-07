#ifndef BOTCONFIG_H
#define BOTCONFIG_H

#include <stdio.h>
#include <string>
#include "../gamestate.h"

struct BotConfig {
	std::vector<std::vector<Game::Coord> > dests;
	std::string mode;
	int enabled;
	unsigned color;
	int maxLoot;
	std::string namePrefix;
	int createEveryNthBlock;
	int maxCreatePerBlock;
	int botStartingIndex;
	int maxMoves;

	IMPLEMENT_SERIALIZE (
		READWRITE(maxMoves);
	    READWRITE(enabled);
		READWRITE(maxCreatePerBlock);
		READWRITE(createEveryNthBlock);
		READWRITE(namePrefix);
		READWRITE(dests);
		READWRITE(mode);
		READWRITE(color);
		READWRITE(maxLoot);
		READWRITE(botStartingIndex);
	)

	BotConfig();
	std::string toString();
};

#endif

