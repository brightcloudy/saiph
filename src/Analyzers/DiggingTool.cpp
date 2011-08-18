#include "Analyzers/DiggingTool.h"

#include <stdlib.h>
#include "Debug.h"
#include "EventBus.h"
#include "Inventory.h"
#include "Item.h"
#include "Saiph.h"
#include "World.h"
#include "Data/Pickaxe.h"
#include "Events/Event.h"
#include "Events/Beatify.h"
#include "Events/ChangedInventoryItems.h"
#include "Events/ReceivedItems.h"
#include "Events/GotDiggingTool.h"

using namespace analyzer;
using namespace event;
using namespace std;

#define VALUE_DIGGER 7000

/* constructors/destructor */
DiggingTool::DiggingTool() : Analyzer("DiggingTool"), _digging_tool_key(0) {
	/* register events */
	EventBus::registerEvent(ChangedInventoryItems::ID, this);
	EventBus::registerEvent(ReceivedItems::ID, this);
}

/* methods */
void DiggingTool::onEvent(Event* const event) {
	if (event->id() == ChangedInventoryItems::ID || event->id() == ReceivedItems::ID) {
		findDigger();
	}
}

class DiggingTool::InvValue : public InventoryValuator {
	int _best;

public:
	InvValue() : _best(0) { }
	~InvValue() { }

	int addItem(const Item& itm, int, bool save) {
		int newbest = max(_best, scoreDigger(itm));
		if (save)
			_best = newbest;
		return newbest * VALUE_DIGGER;
	}
};

void DiggingTool::createValuators(vector<InventoryValuator*>& into) {
	into.push_back(new InvValue);
}

/* private methods */
int DiggingTool::scoreDigger(const Item& item) {
	if (data::Pickaxe::pickaxes().find(item.name()) == data::Pickaxe::pickaxes().end() || item.beatitude() == CURSED)
		return -1;
	int score = 1;
	if (item.beatitude() != BEATITUDE_UNKNOWN)
		score += 2;
	// prefer pick-axes - they're lighter
	if (item.name() == "pick-axe")
		score++;
	else if (Inventory::keyForSlot(SLOT_SHIELD)) // XXX all keyForSlot checks in valuators are wrong
		score = -1;
	return score;
}

void DiggingTool::findDigger() {
	int key = ILLEGAL_ITEM;
	int score = -1;

	for (map<unsigned char, Item>::const_iterator i = Inventory::items().begin(); i != Inventory::items().end(); ++i) {
		int nscore = scoreDigger(i->second);
		if (nscore > score) {
			score = nscore;
			key = i->first;
		}
	}

	if (key != ILLEGAL_ITEM && Inventory::itemAtKey(key).beatitude() == BEATITUDE_UNKNOWN) {
		Beatify b((unsigned char)key, 175);
		EventBus::broadcast(&b);
		key = ILLEGAL_ITEM; // not usable yet
	}

	if (_digging_tool_key != key) {
		_digging_tool_key = key;
		GotDiggingTool g(key >= 0 ? (unsigned char)key : ILLEGAL_ITEM);
		EventBus::broadcast(&g);
	}
}
