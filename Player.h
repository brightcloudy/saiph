#ifndef PLAYER_H
/* defines */
#define PLAYER_H
/* text length for reading textual attribute/status */
#define MAX_EFFECTS 8
#define MAX_TEXT_LENGTH 16

/* forward declare */
class Player;

/* includes */
#include <iostream>
#include "Globals.h"

/* namespace */
using namespace std;

/* the player class holds various info about the player */
class Player {
	public:
		/* variables */
		/* attributes */
		int alignment; // see defined constants
		int charisma;
		int constitution;
		int dexterity;
		int intelligence;
		int strength;
		int wisdom;
		/* status */
		int armor_class;
		int encumbrance; // see defined constants
		int experience;
		int hunger; // see defined constants
		int hitpoints;
		int hitpoints_max;
		int power;
		int power_max;
		/* effects */
		bool blind;
		bool confused;
		bool foodpoisoned;
		bool hallucinating;
		bool hurt_foot;
		bool ill;
		bool slimed;
		bool stunned;
		/* position */
		char level[LEVEL_NAME_SIZE];
		int row;
		int col;
		/* zorkmids */
		int zorkmids;
		/* turn */
		int turn;
		/* for parsing text */
		char effects[MAX_EFFECTS][MAX_TEXT_LENGTH];

		/* constructors */
		Player();

		/* methods */
		bool parseAttributeRow(const char *attributerow);
		bool parseStatusRow(const char *statusrow);
};
#endif
