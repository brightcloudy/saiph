#include "Analyzers/Weapon.h"

#include <stdlib.h>
#include "Debug.h"
#include "EventBus.h"
#include "Inventory.h"
#include "Saiph.h"
#include "World.h"
#include "Actions/Wield.h"
#include "Data/Weapon.h"
#include "Events/Beatify.h"
#include "Events/ChangedInventoryItems.h"
#include "Events/Event.h"
#include "Events/ReceivedItems.h"
#include "Events/WantItems.h"

using namespace analyzer;
using namespace event;
using namespace std;

/* constructors/destructor */
Weapon::Weapon() : Analyzer("Weapon"), _wield_weapon(ILLEGAL_ITEM), _melee_weapons(), _range_weapons() {
	/* register events */
	EventBus::registerEvent(ChangedInventoryItems::ID, this);
	EventBus::registerEvent(ReceivedItems::ID, this);
	EventBus::registerEvent(WantItems::ID, this);
}

/* methods */
void Weapon::analyze() {
	if (!Saiph::polymorphed() && _wield_weapon != ILLEGAL_ITEM && _wield_weapon != Inventory::keyForSlot(SLOT_WEAPON))
		World::setAction(new action::Wield(this, _wield_weapon, WEAPON_WIELD_PRIORITY));
}

void Weapon::onEvent(event::Event* const event) {
	if (event->id() == ChangedInventoryItems::ID) {
		ChangedInventoryItems* e = static_cast<ChangedInventoryItems*> (event);
		for (set<unsigned char>::iterator k = e->keys().begin(); k != e->keys().end(); ++k) {
			const Item& item = Inventory::itemAtKey(*k);
			if (item.count() <= 0) {
				/* we lost this item */
				_range_weapons.erase(*k);
				_melee_weapons.erase(*k);
				continue;
			}
			map<const string, const data::Weapon*>::const_iterator w = data::Weapon::weapons().find(item.name());
			if (w == data::Weapon::weapons().end())
				continue; // not a weapon
			int score = calculateWeaponScore(item, w->second);
			if (isRangedWeapon(w->second))
				_range_weapons[*k] = score;
			else
				_melee_weapons[*k] = score;

		}
		setBestWeapons();
	} else if (event->id() == ReceivedItems::ID) {
		ReceivedItems* e = static_cast<ReceivedItems*> (event);
		for (map<unsigned char, Item>::iterator i = e->items().begin(); i != e->items().end(); ++i) {
			map<const string, const data::Weapon*>::const_iterator w = data::Weapon::weapons().find(i->second.name());
			if (w == data::Weapon::weapons().end())
				continue; // not a weapon
			int score = calculateWeaponScore(i->second, w->second);
			if (isRangedWeapon(w->second)) {
				_range_weapons[i->first] = score;
			} else {
				_melee_weapons[i->first] = score;
				if (i->second.beatitude() == BEATITUDE_UNKNOWN) {
					Beatify b(i->first, 175);
					EventBus::broadcast(&b);
				}
			}
		}
		setBestWeapons();
	} else if (event->id() == WantItems::ID) {
		WantItems* e = static_cast<WantItems*> (event);
//		if (e->dropping() || Saiph::encumbrance() < BURDENED) {
			/* dropping items or we're burdened (which means don't loot weapons) */
			for (map<unsigned char, Item>::iterator i = e->items().begin(); i != e->items().end(); ++i) {
				if (e->dropping()) {
					if ((_range_weapons.find(i->first) != _range_weapons.end() || _melee_weapons.find(i->first) != _melee_weapons.end())) {
						/* don't drop weapons in one of our lists */
						i->second.want(i->second.count());
					}
				} else {
					/* looting, is this better than what we got? */
					if (betterThanWhatWeGot(i->second))
						i->second.want(i->second.count());
				}
			}
//		}
	}
}

/* private methods */
bool Weapon::betterThanWhatWeGot(const Item& item) {
	if (item.beatitude() == CURSED)
		return false;
	if (Saiph::alignment() == LAWFUL && item.name().find("poisoned") != string::npos)
		return false; // ignore poisoned weapons if we're lawful
	map<const string, const data::Weapon*>::const_iterator w = data::Weapon::weapons().find(item.name());
	if (w == data::Weapon::weapons().end())
		return false; // not a weapon
	int score = calculateWeaponScore(item, w->second);
	if (score <= 0)
		return false;

	int min_score = INT_MAX;
	if ((int) _melee_weapons.size() < WEAPON_COUNT_MELEE)
		return true; // melee weapon we got room for
	if (isRangedWeapon(w->second)) {
		if ((int) _range_weapons.size() < WEAPON_COUNT_RANGE)
			return true; // ranged weapon we got room for
		/* check if this weapon is better than any of the ranged weapons we already got */
		for (map<unsigned char, int>::iterator m = _range_weapons.begin(); m != _range_weapons.end(); ++m) {
			if (m->second < min_score)
				min_score = m->second;
		}
	} else {
		/* check if this weapon is better than any of the melee weapons we already got */
		for (map<unsigned char, int>::iterator m = _melee_weapons.begin(); m != _melee_weapons.end(); ++m) {
			if (m->second < min_score)
				min_score = m->second;
		}
	}
	return score > min_score; // returns true if this weapon is better than any weapon we got
}

class Weapon::InvValue : public InventoryValuator {
	int _bestscore; // best score, halved for unknown BUC
	int _bestnscore; // best score, not halved
	int _otherscore; // score of all other weapons, no BUC penalty
	
public:
	InvValue() : _bestscore(0), _bestnscore(0), _otherscore(0) { }
	~InvValue() { }

	int addItem(const Item& itm, int, bool save) {
		int score = 0;
		int bestscore = _bestscore;
		int bestnscore = _bestnscore;
		int otherscore = _otherscore;
		if (data::Weapon::weapons().find(itm.name()) != data::Weapon::weapons().end()) {
			const data::Weapon* wdat = data::Weapon::weapons().find(itm.name())->second;
			for (vector<data::Attack>::const_iterator a = wdat->attackSmall().begin(); a != wdat->attackSmall().end(); ++a)
				score += a->avgDamage() * 750;
			for (vector<data::Attack>::const_iterator a = wdat->attackLarge().begin(); a != wdat->attackLarge().end(); ++a)
				score += a->avgDamage() * 750;
			score += itm.enchantment() * 800;
			score -= itm.damage() * 750;
			int skill = data::Skill::roleMax(Saiph::role(), abs(wdat->type()));
			switch (skill) {
			case P_BASIC:
				break;

			case P_SKILLED:
				score += 400 + 1000;
				break;

			case P_EXPERT:
				score += 600 + 2000;
				break;

			default:
				score += -800 + -2000;
				break;
			}
			if (score > bestscore) {
				otherscore += bestnscore;
				bestnscore = score;
				if (itm.beatitude() == BEATITUDE_UNKNOWN)
					score /= 2;
				bestscore = score;
			} else
				otherscore += score;
		}
		if (save) {
			_bestscore = bestscore;
			_bestnscore = bestnscore;
			_otherscore = otherscore;
		}
		return bestscore + otherscore;
	}

};

void Weapon::createValuators(vector<InventoryValuator*>& into) {
	into.push_back(new InvValue);
}

int Weapon::calculateWeaponScore(const Item& item, const data::Weapon* weapon) {
	if (!weapon->oneHanded() && Inventory::keyForSlot(SLOT_SHIELD) != ILLEGAL_ITEM)
		return 0; // for now, don't try to wield two-hander when we got a shield
	int score = 0;
	for (vector<data::Attack>::const_iterator a = weapon->attackSmall().begin(); a != weapon->attackSmall().end(); ++a)
		score += a->avgDamage();
	for (vector<data::Attack>::const_iterator a = weapon->attackLarge().begin(); a != weapon->attackLarge().end(); ++a)
		score += a->avgDamage();
	if (!weapon->oneHanded())
		score /= 2;
	score += item.enchantment() * 2;
	score -= item.damage() * 2;

	/* set role/skill specific score */
	int skill = data::Skill::roleMax(Saiph::role(), abs(weapon->type()));

	/* score += tohit + 2 * damage */
	switch (skill) {
	case P_BASIC:
		/* +hit = 0, +dam = 0 */
		break;

	case P_SKILLED:
		/* +hit = 2, +dam = 1 */
		score += 2 + 1 * 2;
		break;

	case P_EXPERT:
		/* +hit = 3, +dam = 2 */
		score += 3 + 2 * 2;
		break;

	default:
		/* unskilled */
		/* +hit = -4, +dam = -2 */
		score += -4 + -2 * 2;
		break;
	}

	return score;
}

bool Weapon::isRangedWeapon(const data::Weapon* weapon) {
	/* TODO: roles may have different preferences of which weapons to throw */
	return (weapon->type() == P_DAGGER || weapon->type() == -P_DART || weapon->type() == P_JAVELIN || weapon->type() == P_KNIFE || weapon->type() == -P_SHURIKEN || weapon->type() == P_SPEAR);
}

void Weapon::setBestWeapons() {
	/* set the best weapon we got for wielding.
	 * also clean up _melee_weapons and _range_weapons */
	while ((int) _range_weapons.size() > WEAPON_COUNT_RANGE) {
		map<unsigned char, int>::iterator remove = _range_weapons.begin();
		for (map<unsigned char, int>::iterator m = ++(_range_weapons.begin()); m != _range_weapons.end(); ++m) {
			if (m->second < remove->second)
				remove = m;
		}
		_range_weapons.erase(remove);
	}
	while ((int) _melee_weapons.size() > WEAPON_COUNT_MELEE) {
		map<unsigned char, int>::iterator remove = _melee_weapons.begin();
		for (map<unsigned char, int>::iterator m = ++(_melee_weapons.begin()); m != _melee_weapons.end(); ++m) {
			if (m->second < remove->second)
				remove = m;
		}
		_melee_weapons.erase(remove);
	}

	/* check if the weapon we're wielding is cursed */
	unsigned char cur_key = Inventory::keyForSlot(SLOT_WEAPON);
	if (cur_key != ILLEGAL_ITEM) {
		const Item& item = Inventory::itemAtKey(cur_key);
		if (item.beatitude() == CURSED) {
			_wield_weapon = cur_key;
			return;
		}
	}

	/* find best weapon, weapons we don't know beatitude of are penalized */
	int best_score = INT_MIN;
	int best_key = ILLEGAL_ITEM;
	for (map<unsigned char, int>::iterator m = _melee_weapons.begin(); m != _melee_weapons.end(); ++m) {
		int score = Inventory::itemAtKey(m->first).beatitude() == BEATITUDE_UNKNOWN ? m->second / 2 : m->second;
		if (score > best_score) {
			best_score = score;
			best_key = m->first;
		}
	}
	_wield_weapon = best_key;
}
