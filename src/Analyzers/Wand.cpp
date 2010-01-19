#include "Analyzers/Wand.h"

#include <utility>
#include "Saiph.h"
#include "Globals.h"
#include "World.h"
#include "Debug.h"
#include "Inventory.h"
#include "Actions/Look.h"
#include "Actions/Engrave.h"
#include "Events/ChangedInventoryItems.h"
#include "Events/ElberethQuery.h"
#include "Events/ReceivedItems.h"
#include "Events/WantItems.h"

using namespace analyzer;
using namespace event;
using namespace std;

/* constructors/destructor */
Wand::Wand() : Tracker<data::Wand>("Wand"), _state(WAND_STATE_PREPARE), _wand_key(0) {
	EventBus::registerEvent(ChangedInventoryItems::ID, this);
	EventBus::registerEvent(ReceivedItems::ID, this);
	EventBus::registerEvent(WantItems::ID, this);

	for (map<const string, const data::Wand*>::const_iterator i = data::Wand::items().begin(); i != data::Wand::items().end(); i++) {
		_engrave_groups[i->second->engraveMessage()].insert(i->second);
//		_zap_type_groups[i->second->zapType()].insert(i->second);
	}
}
	
/* methods */
void Wand::analyze() {
	Tracker<data::Wand>::analyze();

	if (_state == WAND_STATE_PREPARE) {
		if (_unidentified_wands.empty() || Saiph::blind() || (Saiph::intrinsics() | Saiph::extrinsics()) & PROPERTY_LEVITATION)
			return;
		ElberethQuery eq;
		EventBus::broadcast(&eq);
		if (eq.type() == ELBERETH_MUST_CHECK) {
			World::setAction(new action::Look(this));
			return;
		} else if (eq.type() == ELBERETH_INEFFECTIVE)
			return;

		_wand_key = *(_unidentified_wands.begin());
		_state = WAND_STATE_DUST_E;
		Debug::notice() << WAND_DEBUG_NAME << "Preparing to identify " << Inventory::items()[_wand_key].name() << std::endl;
	}

	if (_state == WAND_STATE_DUST_E) {
		Debug::notice() << WAND_DEBUG_NAME << "Trying to dust E" << std::endl;
		World::setAction(new action::Engrave(this, ELBERETH "\n", HANDS, PRIORITY_WAND_ENGRAVE_IDENTIFY));
	} else if (_state == WAND_STATE_ENGRAVE_WITH_WAND) {
		Debug::notice() << WAND_DEBUG_NAME << "Trying to engrave with the wand" << std::endl;
		World::setAction(new action::Engrave(this, ELBERETH "\n", _wand_key, PRIORITY_WAND_ENGRAVE_IDENTIFY));
	}
}

void Wand::parseMessages(const string& messages) {
	Tracker<data::Wand>::parseMessages(messages);
}

void Wand::actionCompleted(const string& messages) {
	if (_state == WAND_STATE_DUST_E) {
		_state = WAND_STATE_ENGRAVE_WITH_WAND;
		Debug::notice() << WAND_DEBUG_NAME << "Dusted E, now trying to engrave with wand" << std::endl;
	} else if (_state == WAND_STATE_ENGRAVE_WITH_WAND) {
		Debug::notice() << WAND_DEBUG_NAME << "Engraved with wand, will now parse messages" << std::endl;

		const string& appearance = Inventory::items()[_wand_key].name();
		Debug::notice() << WAND_DEBUG_NAME << "Trying to identify " << appearance << ", messages are: " << messages << std::endl;

		bool called_constrain = false;
		for (map<const string, std::set<const data::Wand*> >::const_iterator i = _engrave_groups.begin(); i != _engrave_groups.end(); i++) {
			if (i->first == "")
				continue;
			if (messages.find(i->first) != string::npos) {
				if (i->second.size() == 1)
					set(appearance, *(i->second.begin()));
				else
					constrainWithin(appearance, i->second);
				called_constrain = true;
				break;
			}
		}
		if (!called_constrain) //no engrave message found
			constrainWithin(appearance, _engrave_groups[""]);

		Debug::notice() << WAND_DEBUG_NAME << "Finished identifying " << appearance << std::endl;

		_unidentified_wands.erase(_wand_key);
		_wand_key = 0;
		_state = WAND_STATE_PREPARE;
	}
}

void Wand::actionFailed() {
	//we failed to engrave at some point in our sequence
	//just start over
	Debug::notice() << WAND_DEBUG_NAME << "Action failed; starting over" << std::endl;
	_state = WAND_STATE_PREPARE;
	_wand_key = 0;
}

void Wand::onEvent(event::Event* const event) {
	Tracker<data::Wand>::onEvent(event);

	//TODO: rather than check all of inventory, only examine items that changed
	if (event->id() == ChangedInventoryItems::ID) {
		refreshUnidentifiedWands();
	} else if (event->id() == ReceivedItems::ID) {
		refreshUnidentifiedWands();
	} else if (event->id() == WantItems::ID) {
		//we want to pick up unidentified wands if engraving would help us identify them
		WantItems* wi = static_cast<WantItems*>(event);
		for (std::map<unsigned char, Item>::iterator i = wi->items().begin(); i != wi->items().end(); i++) {
			if (std::find(data::Wand::appearances().begin(), data::Wand::appearances().end(), i->second.name()) != data::Wand::appearances().end()
					&& engraveUseful(i->second.name()))
				i->second.want(1);
		}
	}
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

///*
// * If a wand has possibilities in two different zap-type groups, then zapping it
// * will allow us to elim possibilities.
// *
// * TODO: Even if a wand's zap type is known for sure, it might still be worth
// * zapping at something for purposes of identification (e.g., zapping undead
// * turning at . will result in "You shudder in dread.", and zapping a directional
// * no-engrave-message wand at . is safe).
// */
//bool Wand::zapUseful(const std::string& appearance) {
//	//TODO: implement
//	return false;
//}

void Wand::refreshUnidentifiedWands() {
	_unidentified_wands.clear();
	for (std::map<unsigned char, Item>::const_iterator i = Inventory::items().begin(); i != Inventory::items().end(); i++) {
		if (std::find(data::Wand::appearances().begin(), data::Wand::appearances().end(), i->second.name()) != data::Wand::appearances().end()
					&& engraveUseful(i->second.name()))
				_unidentified_wands.insert(i->first);
	}
}
