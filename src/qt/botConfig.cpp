#include "botConfig.h"
#include <stdio.h>
#include <string>
#include "botHelper.h"


BotConfig::BotConfig() {
	enabled = 0;
	maxCreatePerBlock=1;
	createEveryNthBlock = 5;
	maxLoot = 450000000;
	color = 0;
	mode="GATHERAFK";
	botStartingIndex = 0;
	maxMoves = 5;
}

std::string BotConfig::toString() {
	std::string result = "###################################################################\n";
	result += "enabled: " + BotHelper::to_string(enabled) + "\n";
	result += "mode: " + mode + "\n";
	result += "color: " + BotHelper::to_string(color) + "\n";
	result += "maxLoot: " + BotHelper::to_string(maxLoot) + "\n";
	result += "namePrefix: " + namePrefix + "\n";
	result += "maxCreatePerBlock: " + BotHelper::to_string(maxCreatePerBlock) + "\n";
	result += "createEveryNthBlock: " + BotHelper::to_string(createEveryNthBlock) + "\n";
	result += "botStartingIndex: " + BotHelper::to_string(botStartingIndex) + "\n";
	result += "maxMoves: " + BotHelper::to_string(maxMoves) + "\n";
	result += "###################################################################\n";
	return result;
}
