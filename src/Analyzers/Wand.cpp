#include "Analyzers/Wand.h"

#include <utility>
#include "Saiph.h"
#include "Globals.h"
#include "World.h"
#include "Debug.h"

using namespace analyzer;
using namespace std;

/* constructors/destructor */
Wand::Wand() : Tracker<data::Wand>("Wand") {
}

/* methods */
void Wand::analyze() {

}

void Wand::parseMessages(const string& messages) {
	
}

/*
 * If a wand has possibilities in two different engrave-message groups, then
 * engraving with it will allow us to eliminate some possibilities.
 */
bool Wand::engraveUseful(const std::string& appearance) {
	const std::set<const data::Wand*>& possibilities = possibilitiesFor(appearance);
	int setsAppearsIn = 0;

	for (std::map<const std::string, std::set<const data::Wand*> >::const_iterator i = _engrave_groups.begin(); i != _engrave_groups.end(); i++)
		for (std::set<const data::Wand*>::const_iterator j = possibilities.begin(); j != possibilities.end(); j++)
			if (i->second.find(*j) != i->second.end()) {
				setsAppearsIn++;
				break; //move to next engrave group
			}

	return setsAppearsIn > 1;
}
