#ifndef MONSTER_H
#define MONSTER_H

#include "Globals.h"
#include "Coordinate.h"

#include <string>
#include <map>

namespace data {
	class Monster;
}

#define RANGED_MISSILES  (1 << 0)
#define RANGED_POLEARM   (1 << 1)
#define RANGED_WAND      (1 << 2)

// Number of turns to remember a monster's movement pattern for.  This is probably overkill; 6 suffices to find the movement holes for a soldier under good conditions
#define MOVE_MEMORY 20

class Monster {
public:
	Monster(const std::string& id);

	const std::string& id() const;
	unsigned char symbol() const;
	unsigned char symbol(unsigned char symbol);
	int color() const;
	int color(int color);
	bool visible() const;
	bool visible(bool visible);
	int attitude() const;
	int attitude(int attitude);
	int lastSeen() const;
	int lastMoved() const;
	Coordinate lastSeenPos() const;
	void observed(const Coordinate& in);
	int observedTurn() const;
	int maxMovesThisTurn() const;
	bool called() const;
	bool called(bool called);
	bool shopkeeper() const;
	bool shopkeeper(bool shopkeeper);
	bool priest() const;
	bool priest(bool priest);
	int ranged() const;
	const data::Monster* data() const;
	const data::Monster* data(const data::Monster* data);

	static std::map<std::string, Monster*>& byID();
	static std::multimap<int, Monster*>& byLastSeen();

	static void parseMessages(const std::string& messages);

private:
	// uniquely identifies this monster.  if G_UNIQ, will be the monster's own identity,
	// either as a typename or a shkname, otherwise will be an integer.
	std::string _id;
	unsigned char _symbol;
	int _color;
	bool _visible;
	int _attitude;
	int _last_seen;
	int _last_moved;
	int _observed_turn;
	int _observed_subturn;
	int _movehist[MOVE_MEMORY];
	Coordinate _last_seen_pos;
	int _ranged;
	// has the monster been 'C'alled with its _id?  always true for G_UNIQ
	bool _called;
	bool _shopkeeper;
	bool _priest;
	const data::Monster* _data;

	void index();
	void unindex();
	void addRanged(int what);

	static int _next_id;
	// all Monster objects in the game are owned by this map; everyone else just holds pointers
	// XXX: Level probably shouldn't be holding pointers.  Probably code which uses Level's access should use our maps instead
	static std::map<std::string, Monster*> _by_id;
	static std::multimap<int, Monster*> _by_last_seen;

	static bool removeAdj(std::string& from, const std::string& adj);
	static Monster* parseMonster(const std::string& head);
	static bool checkPolearm(const std::string& in);

};
#endif
