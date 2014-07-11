#include "util.h"
#include "botTarget.h"
#include <stdio.h>
#include <string>
#include "../gamestate.h"
#include "botHelper.h"

BotTarget::BotTarget() {
	targetIndex = 0;
	targetName = "";
	hunterIndex = 0;
	hunterName = "";
	coord = Game::Coord(-1,-1);
	removeNextBlock = false;
}

BotMove::BotMove() {
	destruct = false;
}

BotConfig::BotConfig() {
	debug = 0;
	enabled = 0;
	mode = "";
	maxLoot = 300000000;
	startBlock = 0;
	startDelay = 120;
	color = 0;
	maxLootDistanceAtDest = 3;
	maxLootDistanceInRoute = 3;
	maxBorn = 0;
	stickToDest = 0;
}
