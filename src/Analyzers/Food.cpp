#include "Analyzers/Food.h"

#include "Debug.h"
#include "EventBus.h"
#include "Saiph.h"
#include "World.h"
#include "Actions/Eat.h"
#include "Actions/EatCorpse.h"
#include "Actions/Pray.h"
#include "Data/Corpse.h"
#include "Data/Food.h"
#include "Events/ChangedInventoryItems.h"
#include "Events/EatItem.h"
#include "Events/ItemsOnGround.h"
#include "Events/ReceivedItems.h"
#include "Events/WantItems.h"

using namespace analyzer;
using namespace event;
using namespace std;

/* constructors/destructor */
Food::Food() : Analyzer("Food"), _lizard_corpses(0) {
	/* try to eat the food with lowest nutrition/weight first, and avoid food that cures bad effects */
	for (map<const string, const data::Food*>::const_iterator f = data::Food::foods().begin(); f != data::Food::foods().end(); ++f) {
		if (!(f->second->effects() & EAT_EFFECT_NEVER_ROT))
			continue; // we're not gonna carry food that rot
		if (f->second->name() == "tin" || f->second->name() == "egg")
			continue; // screw eggs & tins
		int priority = 1000;
		if (f->second->weight() <= 0)
			priority -= f->second->nutrition() / 1; // prevent divide-by-zero (or negative values)
		else
			priority -= f->second->nutrition() / f->second->weight();
		if (f->second->effects() & EAT_EFFECT_CURE_BLINDNESS)
			priority -= 200;
		if (f->second->effects() & EAT_EFFECT_CURE_SICKNESS)
			priority -= 400;
		if (f->second->effects() & EAT_EFFECT_CURE_LYCANTHROPY)
			priority -= 600;
		if (f->second->effects() & EAT_EFFECT_CURE_STONING)
			priority -= 800;
		_eat_priority[f->first] = priority;
	}

	/* register events */
	EventBus::registerEvent(ChangedInventoryItems::ID, this);
	EventBus::registerEvent(EatItem::ID, this);
	EventBus::registerEvent(ItemsOnGround::ID, this);
	EventBus::registerEvent(ReceivedItems::ID, this);
	EventBus::registerEvent(WantItems::ID, this);
}

/* methods */
void Food::analyze() {
	/* update prev_monster_loc with seen monsters (not standing on a stash) */
	_prev_monster_loc.clear();
	for (map<Point, Monster>::const_iterator m = World::level().monsters().begin(); m != World::level().monsters().end(); ++m) {
		if (m->second.visible())
			_prev_monster_loc[m->first] = m->second.symbol();
	}

	if (Saiph::encumbrance() >= OVERTAXED)
		return; // we can't eat while carrying too much

	/* are we hungry? */
	if (Saiph::hunger() <= HUNGRY) {
		/* yes, we are, eat the food item in our inventory with lowest priority */
		if (_food_items.size() > 0) {
			map<unsigned char, Item>::iterator eat = Inventory::items().end();
			for (set<unsigned char>::iterator f = _food_items.begin(); f != _food_items.end(); ++f) {
				map<unsigned char, Item>::iterator i = Inventory::items().find(*f);
				map<string, int>::iterator ep = _eat_priority.find(i->second.name());
				if (i == Inventory::items().end()) {
					/* this should not happen */
					Debug::custom(name()) << "Food item mysteriously disappeared from inventory slot '" << *f << "'" << endl;
					continue;
				} else if (ep == _eat_priority.end()) {
					/* neither should this */
					Debug::custom(name()) << "Want to eat item '" << i->second << "', but that's not in our list of edible items" << endl;
					continue;
				} else if (eat == Inventory::items().end() || _eat_priority.find(eat->second.name())->second < ep->second) {
					/* this food item got a higher eat priority than previous (if any) food item */
					eat = i;
				}
			}
			if (eat != Inventory::items().end()) {
				/* we got something to eat, hooray! */
				World::setAction(static_cast<action::Action*> (new action::Eat(this, eat->first, (Saiph::hunger() == WEAK ? PRIORITY_FOOD_EAT_WEAK : PRIORITY_FOOD_EAT_FAINTING))));
				return;
			}
		}
		/* hmm, nothing to eat, how bad is it? */
		if (Saiph::hunger() <= WEAK) {
			/* bad enough to pray for help.
			 * if this doesn't work... help! */
			if (action::Pray::isSafeToPray())
				World::setAction(static_cast<action::Action*> (new action::Pray(this, PRIORITY_FOOD_PRAY_FOR_FOOD)));
		}
	}
}

void Food::parseMessages(const string& messages) {
	if ((messages.find(MESSAGE_YOU_KILL)) != string::npos || (messages.find(MESSAGE_YOU_DESTROY) != string::npos) || (messages.find(MESSAGE_IS_KILLED)) != string::npos || (messages.find(MESSAGE_IS_DESTROYED)) != string::npos) {
		/* we killed a monster or saw a monster die */
		for (map<Point, unsigned char>::iterator p = _prev_monster_loc.begin(); p != _prev_monster_loc.end(); ++p) {
			if (p->second == 'Z' || p->second == 'M' || p->second == 'V') {
				/* wherever monsters with symbol Z, M or V die, we'll mark as "tainted corpse" */
				_stashes[p->first] = 0 - FOOD_CORPSE_EAT_TIME;
			} else if (_stashes.find(p->first) == _stashes.end()) {
				/* monster probably leaves an edible corpse */
				_stashes[p->first] = World::turn();
			}
		}
		/* mark stashes that didn't appear after a monster died as dangerous */
		for (map<Point, Stash>::const_iterator s = World::level().stashes().begin(); s != World::level().stashes().end(); ++s) {
			map<Point, int>::iterator t = _stashes.find(s->first);
			if (t != _stashes.end())
				continue;
			/* this is a stash where a monster didn't recently die */
			_stashes[s->first] = 0 - FOOD_CORPSE_EAT_TIME;
		}
		/* also clear "corpse_loc" on squares where there are no items nor monsters */
		for (map<Point, int>::iterator c = _stashes.begin(); c != _stashes.end();) {
			if (World::level().monsters().find(c->first) == World::level().monsters().end() && World::level().stashes().find(c->first) == World::level().stashes().end()) {
				_stashes.erase(c++);
				continue;
			}
			++c;
		}
	}
}

void Food::onEvent(Event * const event) {
	if (event->id() == ItemsOnGround::ID) {
		// FIXME?: do we want/need to eat corpses in shops?
		if (World::level().tile().symbol() != SHOP_TILE) {
			ItemsOnGround* e = static_cast<ItemsOnGround*> (event);
			map<Point, int>::iterator cl = _stashes.find(Saiph::position());
			for (list<Item>::const_iterator i = e->items().begin(); i != e->items().end(); ++i) {
				if (cl != _stashes.end() && cl->second + FOOD_CORPSE_EAT_TIME > World::turn()) {
					/* it's safe to eat corpses here */
					map<const string, const data::Corpse*>::const_iterator c = data::Corpse::corpses().find(i->name());
					/* check that item is a corpse, it's safe to eat and that the corpse rots */
					if (c != data::Corpse::corpses().end() && safeToEat(c) && !(c->second->effects() & EAT_EFFECT_NEVER_ROT)) {
						World::setAction(static_cast<action::Action*> (new action::EatCorpse(this, i->name(), PRIORITY_FOOD_EAT_CORPSE)));
						break;
					}
				}
			}
		}
	} else if (event->id() == WantItems::ID) {
		WantItems* e = static_cast<WantItems*> (event);
		for (map<unsigned char, Item>::iterator i = e->items().begin(); i != e->items().end(); ++i) {
			if (_eat_priority.find(i->second.name()) == _eat_priority.end())
				continue; // don't want this food item
			if (i->second.name().find("lizard") && _lizard_corpses >= 1)
				continue; // how many lizard corpses do we _need_?
			else {
				i->second.want(i->second.count());
				_lizard_corpses++;
				continue;
			}
			i->second.want(i->second.count());
		}
	} else if (event->id() == ChangedInventoryItems::ID) {
		ChangedInventoryItems* e = static_cast<ChangedInventoryItems*> (event);
		for (set<unsigned char>::iterator k = e->keys().begin(); k != e->keys().end(); ++k) {
			map<unsigned char, Item>::iterator i = Inventory::items().find(*k);
			if (i == Inventory::items().end()) {
				/* lost this item */
				_food_items.erase(*k);
			} else {
				/* received item, is it food? */
				map<const string, const data::Food*>::const_iterator f = data::Food::foods().find(i->second.name());
				if (f == data::Food::foods().end() || !(f->second->effects() & EAT_EFFECT_NEVER_ROT))
					_food_items.erase(*k); // ewww
				else
					_food_items.insert(*k); // cheezeburger!
			}
		}
	} else if (event->id() == ReceivedItems::ID) {
		ReceivedItems* e = static_cast<ReceivedItems*> (event);
		for (map<unsigned char, Item>::iterator i = e->items().begin(); i != e->items().end(); ++i) {
			map<const string, const data::Food*>::const_iterator f = data::Food::foods().find(i->second.name());
			if (f == data::Food::foods().end() || !(f->second->effects() & EAT_EFFECT_NEVER_ROT))
				continue; // not food or the food rots
			_food_items.insert(i->first);
		}
	} else if (event->id() == EatItem::ID) {
		EatItem* e = static_cast<EatItem*> (event);
		World::setAction(static_cast<action::Action*> (new action::Eat(this, e->key(), e->priority())));
	}
}

/* private methods */
bool Food::safeToEat(const map<const string, const data::Corpse*>::const_iterator& c) {
	/* this method returns true if it's safe to eat given corpse */
	if (Saiph::hunger() >= SATIATED && !(c->second->effects() & EAT_EFFECT_GAIN_LEVEL) && !(c->second->effects() & EAT_EFFECT_ESP))
		return false; // satiated and eating it won't give us benefits that's worth the risk of choking
		/* acidic ain't so bad
		else if ((c->second->eat_effects & EAT_EFFECT_ACIDIC) != 0)
			return false;
		 */
	else if ((c->second->effects() & EAT_EFFECT_AGGRAVATE) != 0)
		return false;
	else if ((c->second->effects() & EAT_EFFECT_DIE) != 0)
		return false;
		/* eat dwarves for now
		else if ((c->second->effects() & EAT_EFFECT_DWARF) != 0)
			return false;
		 */
		/* eat elves for now
		else if ((c->second->effects() & EAT_EFFECT_ELF) != 0)
			return false;
		 */
		/* eat gnomes for now
		else if ((c->second->effects() & EAT_EFFECT_GNOME) != 0)
			return false;
		 */
	else if ((c->second->effects() & EAT_EFFECT_HALLUCINOGENIC) != 0)
		return false;
	else if ((c->second->effects() & EAT_EFFECT_HUMAN) != 0)
		return false;
	else if ((c->second->effects() & EAT_EFFECT_LYCANTHROPY) != 0)
		return false;
		/* mimic for some turns isn't that bad, is it?
		else if ((c->second->eat_effects & EAT_EFFECT_MIMIC) != 0)
			return false;
		 */
	else if ((c->second->effects() & EAT_EFFECT_PETRIFY) != 0)
		return false;
	else if ((c->second->effects() & EAT_EFFECT_POISONOUS) != 0 &&
		!(Saiph::intrinsics() & PROPERTY_POISON ||
		Saiph::extrinsics() & PROPERTY_POISON))
		return false;
	else if ((c->second->effects() & EAT_EFFECT_POLYMORPH) != 0)
		return false;
	else if ((c->second->effects() & EAT_EFFECT_SLIME) != 0)
		return false;
	else if ((c->second->effects() & EAT_EFFECT_STUN) != 0)
		return false;
		/* teleportitis might be fun for a bot
		else if ((c->second->eat_effects & EAT_EFFECT_TELEPORTITIS) != 0)
			return false;
		 */
		/* we're not vegan
		else if ((c->second->eat_effects & EAT_EFFECT_VEGAN) != 0)
			return false;
		 */
		/* nor vegetarian
		else if ((c->second->eat_effects & EAT_EFFECT_VEGETARIAN) != 0)
			return false;
		 */
		/* let's eat stuff that makes us strong
		else if ((c->second->eat_effects & EAT_EFFECT_STRENGTH) != 0)
			return false;
		 */
		/* and stuff that gives us a level
		else if ((c->second->eat_effects & EAT_EFFECT_GAIN_LEVEL) != 0)
			return false;
		 */
		/* and stuff that heals us
		else if ((c->second->eat_effects & EAT_EFFECT_HEAL) != 0)
			return false;
		 */
	else if ((c->second->effects() & EAT_EFFECT_SPEED_TOGGLE) != 0)
		return false;
	/* since we took out lizards from data::Corpse, no monster got this effect
	else if ((c->second->eat_effects & EAT_EFFECT_CURE_STONING) != 0)
		return false;
	 */
	/* since we took out lichens from data::Corpse, no monster got this effect
	else if ((c->second->eat_effects & EAT_EFFECT_REDUCE_STUNNING) != 0)
		return false;
	 */
	/* since we took out lichens from data::Corpse, no monster got this effect
	else if ((c->second->eat_effects & EAT_EFFECT_REDUCE_CONFUSION) != 0)
		return false;
	 */
	/* sure, let's go invisible if we can
	else if ((c->second->eat_effects & EAT_EFFECT_INVISIBILITY) != 0)
		return false;
	 */
	return true;
}
