#include "Actions/ExplicitDrop.h"

#include <sstream>
#include "Inventory.h"
#include "Saiph.h"
#include "World.h"

using namespace action;
using namespace std;

/* constructors/destructor */
ExplicitDrop::ExplicitDrop(analyzer::Analyzer* analyzer, int priority, const vector<pair<int, Item> >& drop_what) : Action(analyzer), _drop("D", priority), _look(":", PRIORITY_LOOK), _drop_what(drop_what) {
}

ExplicitDrop::~ExplicitDrop() {
}

/* methods */
int ExplicitDrop::id() {
	return ID;
}

const Command& ExplicitDrop::command() {
	switch (_sequence) {
	case 0:
		return _drop;

	case 1:
		return _page_select;

	case 2:
		return _look;

	default:
		return Action::NOOP;
	}
}

void ExplicitDrop::update(const std::string& messages) {
	std::string::size_type pos = std::string::npos;
	if (_sequence == 0) {
		if ((pos = messages.find(MESSAGE_DROP_WHICH_ITEMS)) != std::string::npos) {
			/* should always happen, which items do we want to drop? */
			_sequence = 1;
		} else {
			/* shouldn't really happen, maybe we don't have anything to drop? */
			Inventory::update();
			_sequence = 3;
		}
	} else if (_sequence == 1) {
		if (World::menu()) {
			/* more than 0 pages */
			_sequence = 1;
			pos = 0;
		} else {
			/* all pages shown */
			Inventory::update();
			_sequence = 2;
		}
	} else if (_sequence == 2) {
		/* looked at ground */
		_sequence = 3;
	}
	if (_sequence == 1) {
		/* figure out which items we would like to drop */
		std::ostringstream dropcmd;
		while (pos != std::string::npos && messages.size() > pos + 6) {
			pos += 6;
			std::string::size_type length = messages.find("  ", pos);
			if (length == std::string::npos)
				break;
			length = length - pos;
			Item obj;
			if (messages[pos - 2] == '-') {
				std::map<unsigned char, Item>::iterator i = Inventory::items().find(messages[pos - 4]);
				if (i != Inventory::items().end()) {
					obj = i->second;
				} else {
					/* should only happen for gold */
					if (messages.substr(pos, length).find("gold piece") == std::string::npos) {
						obj = Item("1 gold piece");
						obj.count(Saiph::zorkmids());
					}
				}
			}
			if (obj.count() != 0) {
				unsigned j;
				for (j = 0; j < _drop_what.size(); ++j) {
					if (_drop_what[j].second == obj)
						break;
				}
				if (j != _drop_what.size()) {
					if (_drop_what[j].first != obj.count())
						dropcmd << _drop_what[j].first;
					dropcmd << messages[pos - 4];
					_drop_what.erase(_drop_what.begin() + j, _drop_what.begin() + j + 1);
				}
			}
			pos += length;
		}
		dropcmd << CLOSE_PAGE;
		_page_select = Command(dropcmd.str(), PRIORITY_SELECT_ITEM);
	}
}
