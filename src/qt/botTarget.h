#ifndef BOTTARGET_H
#define BOTTARGET_H

#include <stdio.h>
#include <string>
#include "util.h"
#include <iostream>
#include "../gamestate.h"

struct BotMove {
	std::vector<Game::Coord> coords;
	bool destruct;

	IMPLEMENT_SERIALIZE (
		READWRITE(coords);
		READWRITE(destruct);
	)

	BotMove();
};

struct BotState {
	Game::CharacterState state;
	int index;
	std::string name;
	unsigned color;
	std::string mode;

	IMPLEMENT_SERIALIZE (
		READWRITE(mode);
		READWRITE(color);
		READWRITE(state);
		READWRITE(index);
		READWRITE(name);
	)
};

struct BotConfig {

	int debug;
	int enabled;
	std::string mode;
	int64 maxLoot;
	int64 maxMaxLoot;
	int startBlock;
	int startDelay;
	std::vector<std::string> names;
	std::vector<std::vector<Game::Coord> > dests;
	int color;
	std::vector<int> colors;
	int maxLootDistanceAtDest;
	int maxLootDistanceInRoute;
	std::string transfer;
	int maxMoves;
	int maxBorn;
	int maxCreate;
	int createOnBlock;
	int64 minCreateBalance;
	int stickToDest;

	IMPLEMENT_SERIALIZE (
		READWRITE(debug);
		READWRITE(enabled);
		READWRITE(mode);
		READWRITE(maxLoot);
		READWRITE(maxMaxLoot);
		READWRITE(startBlock);
		READWRITE(startDelay);
		READWRITE(maxLootDistanceAtDest);
		READWRITE(maxLootDistanceInRoute);
		READWRITE(transfer);
		READWRITE(maxMoves);
		READWRITE(maxBorn);
		READWRITE(maxCreate);
		READWRITE(createOnBlock);
		READWRITE(minCreateBalance);
		READWRITE(stickToDest);

		READWRITE(color);
		READWRITE(names);
		READWRITE(dests);
		READWRITE(colors);

	)

	BotConfig();
};

struct BotTarget {

	int color;
	Game::Coord coord;
	int64 value;
	std::string targetName;
	int targetIndex;
	std::string hunterName;
	int hunterIndex;
	int block;
	bool removeNextBlock;

	IMPLEMENT_SERIALIZE (
		READWRITE(removeNextBlock);
		READWRITE(color);
		READWRITE(block);
		READWRITE(coord);
		READWRITE(value);
		READWRITE(targetName);
		READWRITE(hunterName);
		READWRITE(targetIndex);
		READWRITE(hunterIndex);
	)

	BotTarget();
};

#endif

