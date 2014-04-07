#include "util.h"
#include "bot.h"
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include "botConfig.h"
#include "../gamestate.h"
#include "gamemovecreator.h"
#include <boost/foreach.hpp>
#include <boost/xpressive/xpressive_dynamic.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/algorithm/string.hpp>


bool BotHelper::coordIsSpawn(Game::Coord pos) {
	return pos.x == 0 || pos.x == 501 || pos.y == 0 || pos.y == 501;
}
Game::Coord BotHelper::getHomeCoord(unsigned color) {
	if (color == 0) {
		return Game::Coord(0,0);
	} else if (color == 1) {
		return Game::Coord(501,0);
	} else if (color == 2) {
		return Game::Coord(501,501);
	} else if (color == 3) {
		return Game::Coord(0,501);
	}
}
std::string BotHelper::to_string(int number) {
    std::string number_string = "";
    char ones_char;
    int ones = 0;
    while(true){
        ones = number % 10;
        switch(ones){
            case 0: ones_char = '0'; break;
            case 1: ones_char = '1'; break;
            case 2: ones_char = '2'; break;
            case 3: ones_char = '3'; break;
            case 4: ones_char = '4'; break;
            case 5: ones_char = '5'; break;
            case 6: ones_char = '6'; break;
            case 7: ones_char = '7'; break;
            case 8: ones_char = '8'; break;
            case 9: ones_char = '9'; break;
            //default : ErrorHandling("Trouble converting number to string.");
        }
        number -= ones;
        number_string = ones_char + number_string;
        if(number == 0){
            break;
        }
        number = number/10;
    }
    return number_string;
}

BotConfig BotHelper::getConfig() {
	BotConfig config = BotConfig();
	std::ifstream infile("botConf.txt");
	if (infile.good()) {
		for( std::string line; getline( infile, line ); ) {
			if (line.substr(0,1) == "#") continue;
			if (line.find("dest=") != std::string::npos) {
				std::vector<Game::Coord> points;
				std::vector<std::string> coords;
				std::string subStr1 = line.substr(5);
				boost::split(coords, subStr1, boost::is_any_of(" "));
				for (int i = 0; i < coords.size(); i++) {
					std::vector<std::string> point;
					boost::split(point, coords[i] , boost::is_any_of(","));
					Game::Coord theCoord = Game::Coord(atoi(point[0]),atoi(point[1]));
					points.push_back(theCoord);
				}
				config.dests.push_back(points);
			} else if (line.find("namePrefix=") != std::string::npos) {
				config.namePrefix = line.substr(11);
			} else if (line.find("color=") != std::string::npos) {
				config.color = atoi(line.substr(6));
			} else if (line.find("createEveryNthBlock=") != std::string::npos) {
				config.createEveryNthBlock = atoi(line.substr(20));
			} else if (line.find("maxCreatePerBlock=") != std::string::npos) {
				config.maxCreatePerBlock = atoi(line.substr(18));
			} else if (line.find("botMode=") != std::string::npos) {
				config.mode = line.substr(8);
			} else if (line.find("maxLoot=") != std::string::npos) {
				config.maxLoot = atoi(line.substr(8));
			} else if (line.find("enabled=")  != std::string::npos) {
				config.enabled = atoi(line.substr(8));
			} else if (line.find("botStartingIndex=") != std::string::npos) {
				config.botStartingIndex = atoi(line.substr(17));
			} else if (line.find("maxMoves=") != std::string::npos) {
				config.maxMoves = atoi(line.substr(9));
			}
		}
		printf("LOADED WITH CONF: %s",config.toString().c_str());
		infile.close();
	}
	return config;
}
