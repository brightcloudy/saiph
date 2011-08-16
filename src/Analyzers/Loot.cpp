#include "Analyzers/Loot.h"

#include <sstream>
#include <stdlib.h>
#include "Debug.h"
#include "EventBus.h"
#include "Inventory.h"
#include "Saiph.h"
#include "World.h"
#include "Actions/Drop.h"
#include "Actions/Engrave.h"
#include "Actions/ExplicitDrop.h"
#include "Actions/ListInventory.h"
#include "Actions/Look.h"
#include "Actions/Loot.h"
#include "Actions/Move.h"
#include "Events/ElberethQuery.h"
#include "Events/StashChanged.h"
#include "Events/ItemsOnGround.h"
#include "Events/WantItems.h"

#define VALUE_GOLD_100   1000
#define VALUE_GOLD_1K    4000
#define VALUE_GOLD_10K   9000
#define VALUE_WEIGHT_PEN 10 /* TODO: should be a sliding scale based on encumbrance */

using namespace analyzer;
using namespace event;
using namespace std;

/* constructors/destructor */
Loot::Loot() : Analyzer("Loot"), _visit() {
	/* register events */
	EventBus::registerEvent(StashChanged::ID, this);
}

/* methods */
void Loot::analyze() {
	/* check inventory if it's not updated */
	if (!Inventory::updated()) {
		World::setAction(new action::ListInventory(this));
		return;
	}

	// TODO: proper shopping code
	if (World::level().tile().symbol() != SHOP_TILE && action::Loot::canLoot())
		partitionFloorInventory();

	/* don't move around when blind/confused/stunned/hallucinating */
	if (Saiph::blind() || Saiph::confused() || Saiph::stunned() || Saiph::hallucinating())
		return;

	/* visit new/changed stashes */
	set<Coordinate>::iterator v = _visit.begin();
	while (v != _visit.end()) {
		map<Point, Stash>::const_iterator s = World::level().stashes().find(*v);
		if (s == World::level().stashes().end() || s->second.lastInspected() == World::turn()) {
			/* stash is gone or we recently looked at it */
			_visit.erase(v++);
			continue;
		}
		Tile tile = World::shortestPath(*v);
		if (tile.direction() == NOWHERE) {
			/* standing on stash, look and remove from visit */
			if (World::setAction(new action::Look(this))) {
				/* TODO: we may get something more important to do just as we step on the loot, making us move away from the stash. 
					 in that case she won't visit the stash again later. can't remove the stash from _visit here. */
				_visit.erase(v++);
				continue;
			}
		} else if (tile.cost() < UNPASSABLE) {
			/* move to stash */
			World::setAction(new action::Move(this, tile, action::Move::calculatePriority(PRIORITY_LOOT_VISIT, tile.cost())));
		}
		++v;
	}
}

void Loot::parseMessages(const string& messages) {
	if (messages.find(MESSAGE_SEVERAL_OBJECTS_HERE) != string::npos || messages.find(MESSAGE_MANY_OBJECTS_HERE) != string::npos || messages.find(MESSAGE_SEVERAL_MORE_OBJECTS_HERE) != string::npos || messages.find(MESSAGE_MANY_MORE_OBJECTS_HERE) != string::npos) {
		/* several/many objects herek, take a look around */
		World::setAction(new action::Look(this));
	}
}

void Loot::onEvent(Event* const event) {
	if (event->id() == StashChanged::ID) {
		/* stash changed, we need to visit it again */
		StashChanged* e = static_cast<StashChanged*> (event);
		_visit.insert(e->position());
	}
}

/* handles pickup and dropping - if this returns no actions, then floor/inv is already an optimal partition */
void Loot::partitionFloorInventory() {
	vector<pair<int, Item> > cands;
	vector<int> where;

	if (Saiph::zorkmids() > 0) {
		cands.push_back(make_pair(Saiph::zorkmids(), Item("1 gold piece")));
		where.push_back('$');
	}

	for (map<unsigned char, Item>::iterator i = Inventory::items().begin(); i != Inventory::items().end(); ++i) {
		where.push_back(i->first);
		cands.push_back(make_pair(i->second.count(), i->second));
		cands.back().second.count(1);
	}

	const map<Point, Stash>& stashes = World::level().stashes();
	map<Point,Stash>::const_iterator si = stashes.find(Saiph::position());

	if (si != stashes.end()) {
		const list<Item>& items = si->second.items();
		int ix = 0;
		for (list<Item>::const_iterator ii = items.begin(); ii != items.end(); ++ii) {
			where.push_back(--ix);
			cands.push_back(make_pair(ii->count(), *ii));
			cands.back().second.count(1);
		}
	}

	vector<int> chosen;
	optimizePartition(chosen, cands, 0, 0);

	vector<pair<int, Item> > to_drop;
	for (unsigned i = 0; i < cands.size(); ++i) {
		if (where[i] < 0) break; // this is a floor item
		if (chosen[i] < cands[i].first) {
			to_drop.push_back(make_pair(cands[i].first - chosen[i], cands[i].second));
			to_drop.back().second.count(cands[i].first);

			Debug::custom(name()) << "Will drop " << to_drop.back().first << " x " << to_drop.back().second << endl;
		}
	}

	if (!to_drop.empty())
		World::setAction(new action::ExplicitDrop(this, PRIORITY_LOOT_PICKUP_DROP, to_drop));

	// TODO: pickup handling, armor removal
}


class LootInvValue : public InventoryValuator {
public:
	LootInvValue();
	virtual ~LootInvValue() { }

	virtual int addItem(const Item& item, int already, bool save);
private:
	int _slots_used;
	int _items_weight;
	int _gold;
};

LootInvValue::LootInvValue() : _slots_used(0), _items_weight(0), _gold(0) { }

int LootInvValue::addItem(const Item& item, int already, bool save) {
	int newgold = _gold;
	int newslot = _slots_used;
	int newinvw = _items_weight;

	if (item.name() == "gold piece") {
		newgold++;
	} else {
		if (already == 0) // first item of this stack
			newslot++;
		newinvw += 50; // TODO implement Item::weight
	}
	int totalw = newinvw + (newgold + 50) / 100;
	int value = 0;
	if (newgold < 100)
		value += (newgold * VALUE_GOLD_100) / 100;
	else if (newgold < 1000)
		value += VALUE_GOLD_100 + ((newgold - 100) * (VALUE_GOLD_1K - VALUE_GOLD_100)) / 900;
	else if (newgold < 10000)
		value += VALUE_GOLD_1K + ((newgold - 1000) * (VALUE_GOLD_10K - VALUE_GOLD_1K)) / 9000;
	else
		value += VALUE_GOLD_10K;

	value -= VALUE_WEIGHT_PEN * totalw;
	if (newslot > 52) value = -100000000;

	if (save) {
		_slots_used = newslot;
		_items_weight = newinvw;
		_gold = newgold;
	}
	return value;
}

void Loot::createValuators(vector<InventoryValuator*>& to) {
	to.push_back(new LootInvValue());
}

// This is the heart of the new (Aug 2011) inventory manager.  Given a set of items known to exist, it tries to find the best combination for us to carry, using
// a greedy hill-climbing algorithm.  It starts with an empty imaginary inventory, and at each step adds the item that gives the largest benefit, stopping when
// no item gives a benefit.

// The fundamental property of this algorithm is: Theorem 1.  If A = B \cup C and Part(D) = D, then Part(A \cup D) = D if and only if Part(B \cup D) = D and
// Part(C \cup D) = D (for the purposes of these proofs, Part(X) can be considered to return an ordered list)

// Proof(if): If Part(A \cup D) != D, then at some step either an item must be chosen which is not in D, or the algorithm must terminate early.  Early
// termination would contradict the assumption that Part(D) = D.  If a different item is chosen, then it must be in A, and therefore must be in B or C; without
// loss of generality assume B.  During the execution of Part(B \cup D), at the same step, the same item is available; since it gave the largest improvement in
// A \cup D, it must also be the largest in the smaller set B \cup D, and it must be included, violating the assumption that Part(B \cup D) = D.  As all cases
// lead to a contradiction, QED.

// Proof(only if): The argument is similar.  Without loss of generality, assume Part(B \cup D) != D; then, the first item in the result not in D is definitely
// also in A, and since the corresponding element in D is not the highest in B \cup D, it cannot be the highest in A \cup D, contradicting the assumption that
// Part(A \cup D) = D.

// Definition.  Saiph cares about a pile P if Part(I \cup P) != I, where I is Saiph's inventory.

// Corollary.  Saiph cares about a pile if and only if she cares about at least one of its items, considered as a singleton pile.

// Definition.  The termination order on partition results is the lexicographic order on intermediate scores.

// Theorem 2. Part(A \cup B) >= Part(B) in the termination order.

// Proof: If Part(A \cup B) < Part(B), then there would have to be a step where Part(A \cup B) takes a smaller item; but this is not possible as A \cup B is the
// larger set and must have a smaller maximum.

// Corollary.  Saiph cares about a pile if and only if Part(I \cup P) > Part(P).

// Definition:  "Global optimization" is an algorithm where Saiph, at each step, visits some pile she cares about and sets I = Part(I \cup P).

// Theorem 3. "Global optimization" always terminates (no infinite loops).

// Proof: At each step I increases in the termination order.  But I is defined on a finite universe with size \sum_{i=0}^{52} i \choose N, where N is the number
// of items in the game (XXX it's actually a bit higher if we use bags, but still finite), so there are no infinite increasing sequences.

// Theorem 4.: "Global optimization" ends with I = Part(U) where U = I_0 \cup \bigcup_{0 \leq i < N} P_i.

// Proof: Let F = U - I.  If I != Part(U), then I != Part(I \cup F), and therefore by iterative application of Theorem 1 there exists an item x \in F such that
// I != Part(I \cup x).  x is necessarily in some pile P_i; by Corollary 1.1 P_i is an interesting pile, contradicting the assumption that global optimization
// was complete.

// Corollary: Global optimization is independant of the order in which piles are visited, or how items are initially divided into piles.

// XXX The above block of comments was written without consideration for item stacks, which complicate issues somewhat.  We currently assume stacks of items
// will merge iff they are currently stacked; I don't know if this is sufficient to get useful antistacking.

void Loot::getValuators(vector<InventoryValuator*>& valuators) {
	// Get zero or more from each analyzer; Loot's own valuator(s) will handle encumbrance and other limits
	for (vector<Analyzer*>::const_iterator ai = World::analyzers().begin(); ai != World::analyzers().end(); ++ai)
		(*ai)->createValuators(valuators);
}

int Loot::valuate(vector<InventoryValuator*>& valuators, const Item& item, int already, bool save) {
	int res = 0;
	for (vector<InventoryValuator*>::iterator vi = valuators.begin(); vi != valuators.end(); ++vi)
		res += (*vi)->addItem(item, already, save);
	return res;
}

// This function implements Part() in the above description for a general set of items.
// It assumes that picking up a gold piece will never make another item more valuable
// This tries to be systematically biased to break ties in favor of items earlier in the list.  We might need a more formal tiebreak procedure.
// If spectators is non-null it points to a vector of items which are to be tested for relevance; spectator_out[i] = true iff Part(I \cup spectators[i]) != Part(I)
void Loot::optimizePartition(vector<int>& out, const vector<pair<int,Item> >& possibilities, vector<bool>* spectator_out, const vector<Item>* spectators) {
	vector<InventoryValuator*> valuators;
	getValuators(valuators);

	int score = 0;
	out.resize(possibilities.size());
	fill(out.begin(), out.end(), 0);

	if (spectators) {
		spectator_out->resize(spectators->size());
		fill(spectator_out->begin(), spectator_out->end(), false);
	}

	while (true) {
		int bestscore = score;
		int bestnext  = -1;
		int sndscore  = score;
		int sndnext   = -1;

		for (int i = 0; i < int(possibilities.size()); ++i) {
			if (possibilities[i].first == out[i])
				continue; // we already have all of this item
			int candscore = valuate(valuators, possibilities[i].second, out[i], false);
			if (candscore > bestscore) {
				sndscore  = bestscore;
				sndnext   = bestnext;
				bestscore = candscore;
				bestnext  = i;
			} else if (candscore > sndscore) {
				sndscore  = candscore;
				sndnext   = i;
			}
		}

		if (spectators) {
			for (int i = 0; i < int(spectators->size()); ++i) {
				int specscore = valuate(valuators, (*spectators)[i], 0, false);
				if (specscore > bestscore)
					(*spectator_out)[i] = true;
				else if (specscore > sndscore) {
					sndscore = specscore;
					sndnext  = possibilities.size() + i; // never has priority over main-set items
				}
			}
		}

		if (bestnext < 0)
			break; // nothing left useful to add

		if (possibilities[bestnext].second.name() == "gold piece") {
			// Since the inventory will often be 98% gold pieces or so, we want to treat them as efficiently as we can.  Since we've assumed that
			// gold pieces can never raise the value of other items, we keep adding gold pieces until the marginal utility is less than the
			// original second best marginal utility.
			int sndmarg = sndscore - score;

			do {
				score = valuate(valuators, possibilities[bestnext].second, out[bestnext], true);
				out[bestnext]++;

				bestscore = valuate(valuators, possibilities[bestnext].second, out[bestnext], false);
			} while (out[bestnext] < possibilities[bestnext].first &&
					((bestscore - score) > sndmarg || ((bestscore - score) == sndmarg && bestnext < sndnext)));
		} else {
			// No assumptions made so we can only add one before reentering the selection loop
			score = valuate(valuators, possibilities[bestnext].second, out[bestnext], true);
			out[bestnext]++;
		}
	}

	for (vector<InventoryValuator*>::iterator it = valuators.begin(); it != valuators.end(); ++it)
		delete *it;
}
