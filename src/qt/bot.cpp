#include "util.h"
#include "bot.h"
#include <stdio.h>
#include <string>
#include <iostream>
#include "../gamestate.h"
#include "gamemovecreator.h"
#include "botHelper.h"
#include <boost/foreach.hpp>
#include <boost/xpressive/xpressive_dynamic.hpp>
#include <boost/assign/list_of.hpp>
#include <math.h>

Bot::Bot() {

}

std::vector<Game::Coord> Bot::calculate(const Game::PlayerState &playerState,const Game::GameState &gameState) {
	std::vector<Game::Coord> result;
	if (mode == "GATHERAFK") {
		result = getGatherAfkCoords(playerState,gameState);
	}
	return result;
}

std::vector<Game::Coord> Bot::getGatherAfkCoords(const Game::PlayerState &playerState,const Game::GameState &gameState) {
	std::vector<Game::Coord> result;
	BOOST_FOREACH(const PAIRTYPE(int, Game::CharacterState) &charState, playerState.characters) {
		if (charState.first > 2) {continue;}
		if (charState.second.loot.nAmount >= maxLoot) {
			printf("MAX LOOT %s GO HOME -- ",FormatMoney(charState.second.loot.nAmount).c_str());
			result.push_back(BotHelper::getHomeCoord(color));
		} else if (dests.size() > charState.first) {
			printf("CHAR %d SEND TO DEST (%d,%d) -- ",charState.first,dests[charState.first].x,dests[charState.first].y);
			result.push_back(dests[charState.first]);
		}
	}
	return result;
}
