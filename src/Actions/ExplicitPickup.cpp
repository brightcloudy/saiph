#include "Actions/ExplicitPickup.h"

#include <sstream>
#include "Inventory.h"
#include "Saiph.h"
#include "World.h"

using namespace action;
using namespace std;

/* constructors/destructor */
ExplicitPickup::ExplicitPickup(analyzer::Analyzer* analyzer, int priority, const vector<pair<int, Item> >& pick_what) : Action(analyzer), _look(":", PRIORITY_LOOK), _pick_what(pick_what) {
	// If there's only one item on the current square, we can't open a menu - we have to issue the count before we begin.  Ick.
	map<Point, Stash>::const_iterator si = World::level().stashes().find(Saiph::position());

	if (si != World::level().stashes().end() && si->second.items().size() == 1) {
		// yep, only one item.
		const Item& obj = si->second.items().front();
		if (_pick_what.size() != 1 || _pick_what[0].second != obj) {
			// this shouldn't happen
			_sequence = N; // straight to Look
		} else {
			std::ostringstream buf;
			buf << _pick_what[0].first;
			buf << ',';
			_variable = Command(buf.str(), priority);
		}
	} else {
		// we can use the menu
		_variable = Command(',', priority);
	}
}

ExplicitPickup::~ExplicitPickup() {
}

/* methods */
int ExplicitPickup::id() {
	return ID;
}

bool ExplicitPickup::canPickup() {
	std::map<Point, int>::const_iterator s = World::level().symbols(TRAP).find(Saiph::position());
	return s == World::level().symbols(TRAP).end() || (s->second != TRAP_PIT && s->second != TRAP_SPIKED_PIT);
}

const Command& ExplicitPickup::command() {
	switch (_sequence) {
	case 0:
		return _variable;

	case 1:
		return _look;

	default:
		return Action::NOOP;
	}
}

void ExplicitPickup::update(const std::string& messages) {
	if (_sequence == 1) {
		/* looked at ground */
		_sequence = 2;
		Inventory::update();
		return;
	}

	std::string::size_type pos = std::string::npos;
	if ((pos = messages.find(MESSAGE_PICK_UP_WHAT)) == std::string::npos) {
		/* only one item, or done selecting items */
		_sequence = 1;
		return;
	}

	/* figure out which items to pick up */
	std::ostringstream pickcmd;
	while (pos != std::string::npos && messages.size() > pos + 6) {
		pos += 6;
		std::string::size_type length = messages.find("  ", pos);
		if (length == std::string::npos)
			break;
		length = length - pos;
		Item obj;
		if (messages[pos - 2] == '-') {
			obj = Item(messages.substr(pos, length), 0);
		}
		if (obj.count() != 0) {
			unsigned j;
			for (j = 0; j < _pick_what.size(); ++j) {
				if (_pick_what[j].second == obj)
					break;
			}
			if (j != _pick_what.size()) {
				if (_pick_what[j].first != obj.count())
					pickcmd << _pick_what[j].first;
				pickcmd << messages[pos - 4];
				_pick_what.erase(_pick_what.begin() + j, _pick_what.begin() + j + 1);
			}
		}
		pos += length;
	}
	pickcmd << CLOSE_PAGE;
	_variable = Command(pickcmd.str(), PRIORITY_SELECT_ITEM);
}
