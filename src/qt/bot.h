#ifndef BOT_H
#define BOT_H

#include <stdio.h>
#include "../gamestate.h"
#include <string>
#include "util.h"
#include "bot.h"
#include <iostream>
#include "botTarget.h"
#include "botHelper.h"

struct Bot {

	int debug;
	std::string mode;
	std::string name;
	std::vector<Game::Coord> dests;
	BotConfig botConfig;
	int64 maxMaxLoot;
	int64 maxLoot;
	int maxLootDistanceAtDest;
	int maxLootDistanceInRoute;
	int born;
	int pendingCount;
	std::map<int,BotTarget> botTargets;
	int stickToDest;
	int aloneAttempt;

	IMPLEMENT_SERIALIZE (
		READWRITE(debug);
		READWRITE(name);
		READWRITE(dests);
		READWRITE(botConfig);
		READWRITE(maxLoot);
		READWRITE(maxMaxLoot);
		READWRITE(maxLootDistanceAtDest);
		READWRITE(maxLootDistanceInRoute);
		READWRITE(born);
		READWRITE(pendingCount);
		READWRITE(botTargets);
		READWRITE(stickToDest);
		READWRITE(aloneAttempt);
    )

	Bot();

	std::map<int,BotMove> calculate(const Game::PlayerState &playerState,const Game::GameState &gameState,std::map<std::string,BotTarget> &myTargets);

	std::map<int,BotMove> getGatherCoords(const Game::PlayerState &playerState,const Game::GameState &gameState,std::map<std::string,BotTarget> &myTargets);

};

#endif

