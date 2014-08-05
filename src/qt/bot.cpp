#include "util.h"
#include "bot.h"
#include <stdio.h>
#include <string>
#include <iostream>
#include "../gamestate.h"
#include "gamemovecreator.h"
#include <boost/foreach.hpp>
#include <boost/xpressive/xpressive_dynamic.hpp>
#include <boost/assign/list_of.hpp>
#include <math.h>

Bot::Bot() {
	born = 0;
	pendingCount = 0;
	aloneAttempt = 0;
}

std::map<int,BotMove> Bot::calculate(const Game::PlayerState &playerState,const Game::GameState &gameState,std::map<std::string,BotTarget> &myTargets) {
	 //if (mode == "GATHER") {
		 return getGatherCoords(playerState,gameState,myTargets);
	 //}
}

std::map<int,BotMove> Bot::getGatherCoords(const Game::PlayerState &playerState,const Game::GameState &gameState,std::map<std::string,BotTarget> &myTargets) {
	std::map<int,BotMove> botMoves;
	BOOST_FOREACH(const PAIRTYPE(int, Game::CharacterState) &charState, playerState.characters) {
		botMoves[charState.first] = BotMove();
	}
	BOOST_FOREACH(const PAIRTYPE(int, Game::CharacterState) &charState, playerState.characters) {
		if (debug == 1) printf("HANDLING CHAR %d -- ",charState.first);
		BotMove &botMove = botMoves[charState.first];
		std::vector<Game::Coord> coords;
		int destIdx = charState.first;
		if (destIdx+1 > dests.size()) {
			destIdx = 0;
		}
		if (charState.second.loot.nAmount > 1000000 && !charState.second.waypoints.empty() && BotHelper::coordIsSpawn(charState.second.waypoints.front())) {
			if (debug == 1) printf("HAVE LOOT ALREADY ON WAY HOME -- ");
			continue;
		}



		if (botTargets[charState.first].targetName.find("!LOOT ") != std::string::npos) {
			if (debug == 1) printf("CHECKING TARGETED LOOT CHAR %d -- ",charState.first);
			if (BotHelper::isLootAt(gameState,botTargets[charState.first].coord)) {
				if (debug == 1) printf("LOOT STILL THERE -- ");
				if (charState.second.waypoints.empty() || charState.second.waypoints.front() != botTargets[charState.first].coord) {
					if (debug == 1) printf("MOVE TO TARGET -- ");
					botMove.coords.push_back(botTargets[charState.first].coord);
					continue;
				} else {
					if (debug == 1) printf("CONTINUE TO TARGET -- ");
					continue;
				}
			} else {
				if (debug == 1) printf("LOOT GONE ERASE TARGET DATA -- ");
				myTargets.erase(botTargets[charState.first].targetName);
				botTargets[charState.first].targetName = "";
			}
		}
		if (charState.second.loot.nAmount >= maxMaxLoot) {
			if (debug == 1) printf("ACHIEVED MAX-MAX LOOT GO HOME -- ");
			aloneAttempt = 0;
			botMove.coords.push_back(BotHelper::getClosestHome(playerState.color,charState.second.coord));
			continue;
		}
		BotState botState;
		botState.name = name;
		botState.state = charState.second;
		botState.color = playerState.color;
		botState.mode = mode;
		botState.index = charState.first;
		BotTarget lootTarget;



		if (BotHelper::getDistance(charState.second.coord,dests[destIdx]) > BotHelper::getLinear(maxLootDistanceAtDest)) {
			if (maxLootDistanceInRoute > 0) {
				if (debug == 1) printf("OUTSIDE DESTINATION SEARCH LOOT %d -- ",maxLootDistanceInRoute);
				lootTarget = BotHelper::getClosestLoot(gameState,botState,myTargets,BotHelper::getLinear(maxLootDistanceInRoute));
				if (BotHelper::getTime(lootTarget.coord,charState.second.coord) > maxLootDistanceInRoute) {
					lootTarget.targetName = "";
				}
			}
		} else {
			if (debug == 1) printf("INSIDE DESTINATION SEARCH LOOT %d -- ",maxLootDistanceAtDest);
			lootTarget = BotHelper::getClosestLoot(gameState,botState,myTargets,BotHelper::getLinear(maxLootDistanceAtDest));
			if (BotHelper::getTime(lootTarget.coord,charState.second.coord) > maxLootDistanceAtDest) {
				lootTarget.targetName = "";
			}
		}
		if (lootTarget.targetName != "") {
			if (debug == 1) printf("LOOT TARGET DETECTED -- ");
			myTargets[lootTarget.targetName] = lootTarget;
			botTargets[charState.first] = lootTarget;
			botMove.coords.push_back(lootTarget.coord);
			aloneAttempt = 0;
			continue;
		}
		if (charState.second.loot.nAmount >= maxLoot) {
			if (debug == 1) printf("ACHIEVED MAX LOOT GO HOME -- ");
			aloneAttempt = 0;
			botMove.coords.push_back(BotHelper::getClosestHome(playerState.color,charState.second.coord));
			continue;
		} else {
			if (charState.second.waypoints.empty()) {
				if (stickToDest == 1) {
					if (debug == 1) printf("STICK TO DEST -- ");
					if (charState.second.coord != dests[destIdx]) {
						botMove.coords.push_back(dests[destIdx]);
						continue;
					}
				} else {
					if (BotHelper::getDistance(charState.second.coord,dests[destIdx]) > BotHelper::getLinear(maxLootDistanceAtDest)) {
						if (debug == 1) printf("WPS EMPTY GO TO DEST (%d,%d) -- ",dests[destIdx].x,dests[destIdx].y);
						botMove.coords.push_back(dests[destIdx]);
						aloneAttempt = 0;
						continue;
					} else {
						if (!BotHelper::isAlone(botState,gameState) && aloneAttempt == 0) {
							aloneAttempt++;
							if (debug == 1) printf("NOT ALONE -- ");
							Game::Coord coord = BotHelper::getEmptyCoinCoord(gameState,dests[destIdx],maxLootDistanceAtDest);
							if (coord.x != -1) {
								if (debug == 1) printf("MOVE TO EMPTY COORD -- ");
								botMove.coords.push_back(coord);
								continue;
							}
						}
					}
				}
			} else {
				if (BotHelper::getDistance(charState.second.waypoints.front(),dests[destIdx]) > BotHelper::getLinear(maxLootDistanceAtDest)) {
					if (debug == 1) printf("WPS ADJUST FROM (%d,%d) TO DEST (%d,%d) -- ",charState.second.waypoints.front().x,charState.second.waypoints.front().y,dests[destIdx].x,dests[destIdx].y);
					botMove.coords.push_back(dests[destIdx]);
					aloneAttempt = 0;
					continue;
				}
			}
		}

	}
	return botMoves;
}
