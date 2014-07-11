#ifndef BOTHELPER_H
#define BOTHELPER_H

#include <stdio.h>
#include "../gamestate.h"
#include <string>
#include "util.h"
#include "bot.h"
#include <iostream>
#include "botTarget.h"

struct BotHelper {
	public:
		static BotConfig getConfig();
		static bool coordIsSpawn(Game::Coord pos);
		static Game::Coord getClosestHome(unsigned color,Game::Coord from);
		static double getDistance(Game::Coord to,Game::Coord from);
		static double getLinear(int i);
		static bool isLootAt(const Game::GameState &gameState,const Game::Coord &coord);
		static BotTarget getClosestLoot(const Game::GameState &gameState,const BotState &botState,std::map<std::string,BotTarget> &myTargets,double within);
		static std::string coordToString(Game::Coord coord);
		static std::string to_string(int number);
		static unsigned getTime(Game::Coord to,Game::Coord from);
		static bool isAlone(const BotState &botState,const Game::GameState &gameState);
		static bool isEmpty(const Game::Coord &coord,const Game::GameState &gameState);
		static Game::Coord getEmptyCoinCoord(const Game::GameState &gameState,const Game::Coord &coord,int within);
};

#endif

