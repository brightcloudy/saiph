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
#include "Actions/ListInventory.h"
#include "Actions/Look.h"
#include "Actions/Loot.h"
#include "Actions/Move.h"
#include "Events/ElberethQuery.h"
#include "Events/StashChanged.h"
#include "Events/ItemsOnGround.h"
#include "Events/WantItems.h"

using namespace analyzer;
using namespace event;
using namespace std;

/* constructors/destructor */
Loot::Loot() : Analyzer("Loot"), _visit() {
	/* register events */
	EventBus::registerEvent(StashChanged::ID, this);
	EventBus::registerEvent(ItemsOnGround::ID, this);
}

/* methods */
void Loot::analyze() {
	/* check inventory if it's not updated */
	if (!Inventory::updated()) {
		World::setAction(new action::ListInventory(this));
		return;
	}

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

	if (World::level().tile().symbol() == STAIRS_UP) {
		/* stairs up, ask if anyone wants to drop anything and if they do, get down 3 E's and drop stuff */
		WantItems wi(true, true);
		for (map<unsigned char, Item>::iterator i = Inventory::items().begin(); i != Inventory::items().end(); ++i) {
			/* only add items not in a slot */
			if (Inventory::slotForKey(i->first) == ILLEGAL_SLOT)
				wi.addItem(i->first, i->second);
		}
		bool drop_on_stairs = false;
		EventBus::broadcast(&wi);
		for (map<unsigned char, Item>::iterator i = wi.items().begin(); !drop_on_stairs && i != wi.items().end(); ++i) {
			if (i->second.want() <= 0 || i->second.want() < i->second.count())
				drop_on_stairs = true;
		}
		if (drop_on_stairs) {
			ElberethQuery eq;
			EventBus::broadcast(&eq);
			if (eq.type() == ELBERETH_MUST_CHECK) {
				/* we don't know, we must look */
				World::setAction(new action::Look(this));
			} else if (eq.type() == ELBERETH_DUSTED || eq.type() == ELBERETH_NONE) {
				/* no elbereth or dusted elbereth */
				if (eq.count() < 3) {
					/* less than 3 elbereths, engrave */
					World::setAction(new action::Engrave(this, ELBERETH "\n", HANDS, PRIORITY_LOOT_DROP, (eq.count() > 0)));
				} else {
					/* 3 or more elbereths, drop stuff we don't want */
					World::setAction(new action::Drop(this, PRIORITY_LOOT_DROP, true));
				}
			}
		}
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
	} else if (event->id() == ItemsOnGround::ID) {
		/* standing on stash, ask if anyone want anything */
		// TODO: proper shopping code
		if (World::level().tile().symbol() != SHOP_TILE && action::Loot::canLoot()) {
			ItemsOnGround* e = static_cast<ItemsOnGround*> (event);
			int index = 0;
			bool looting = false;
			list<Item>::const_iterator i = e->items().begin();
			/* if we're standing on STAIRS_UP we'll pretend it's a safe stash */
			bool safe_stash = World::level().tile().symbol() == STAIRS_UP;
			WantItems wi(false, safe_stash);
			while (!looting) {
				wi.addItem(index++, *i);
				++i;
				if (index == UCHAR_MAX || i == e->items().end()) {
					EventBus::broadcast(&wi);
					for (map<unsigned char, Item>::iterator i = wi.items().begin(); i != wi.items().end(); ++i) {
						if (i->second.want() <= 0 || i->second.count() <= 0)
							continue;
						/* someone want an item in this stash */
						World::setAction(new action::Loot(this, PRIORITY_LOOT, safe_stash));
						looting = true;
						break;
					}
					wi.clear();
					index = 0;
				}
				if (i == e->items().end())
					break;
			}
		}
	}
}

// Interface for analyzers to communicate desire for items.
// An InventoryValuator instance tracks an "incomplete inventory" and assigns a value to it.
// At creation, the valuator represents an empty inventory with a value of 0.
// Items are added one at a time using addItem, which should return the new value; if save is false, then the valuator's state should be unchanged, allowing items to be tested for value.
// The sum of all used valuators is used to determine composite inventory value; Loot aims to maximize this value.
class InventoryValuator {
public:
	InventoryValuator();
	virtual ~InventoryValuator();

	virtual int addItem(const Item& i, bool save) = 0;
};

// This is the heart of the new (Aug 2011) inventory manager.  Given a set of items known to exist, it tries to find the best combination for us to carry, using a greedy hill-climbing algorithm.
// It starts with an empty imaginary inventory, and at each step adds the item that gives the largest benefit, stopping when no item gives a benefit.

// The fundamental property of this algorithm is:
// Theorem 1.  If A = B \cup C and Part(D) = D, then Part(A \cup D) = D if and only if Part(B \cup D) = D and Part(C \cup D) = D
// (for the purposes of these proofs, Part(X) can be considered to return an ordered list)

// Proof(if): If Part(A \cup D) != D, then at some step either an item must be chosen which is not in D, or the algorithm must terminate early.  Early termination would contradict the
// assumption that Part(D) = D.  If a different item is chosen, then it must be in A, and therefore must be in B or C; without loss of generality assume B.  During the execution of
// Part(B \cup D), at the same step, the same item is available; since it gave the largest improvement in A \cup D, it must also be the largest in the smaller set B \cup D, and
// it must be included, violating the assumption that Part(B \cup D) = D.  As all cases lead to a contradiction, QED.

// Proof(only if): The argument is similar.  Without loss of generality, assume Part(B \cup D) != D; then, the first item in the result not in D is definitely also in A, and since
// the corresponding element in D is not the highest in B \cup D, it cannot be the highest in A \cup D, contradicting the assumption that Part(A \cup D) = D.

// Definition.  Saiph cares about a pile P if Part(I \cup P) != I, where I is Saiph's inventory.

// Corollary.  Saiph cares about a pile if and only if she cares about at least one of its items, considered as a singleton pile.

// Definition.  The termination order on partition results is the lexicographic order on intermediate scores.

// Theorem 2. Part(A \cup B) >= Part(B) in the termination order.

// Proof: If Part(A \cup B) < Part(B), then there would have to be a step where Part(A \cup B) takes a smaller item; but this is not possible as A \cup B is the larger set and
// must have a smaller maximum.

// Corollary.  Saiph cares about a pile if and only if Part(I \cup P) > Part(P).

// Definition:  "Global optimization" is an algorithm where Saiph, at each step, visits some pile she cares about and sets I = Part(I \cup P).

// Theorem 3. "Global optimization" always terminates (no infinite loops).

// Proof: At each step I increases in the termination order.  But I is defined on a finite universe with size \sum_{i=0}^{52} i \choose N, where N is the number of items in the game
// (XXX it's actually a bit higher if we use bags, but still finite), so there are no infinite increasing sequences.

// Theorem 4.: "Global optimization" ends with I = Part(U) where U = I_0 \cup \bigcup_{0 \leq i < N} P_i.

// Proof: Let F = U - I.  If I != Part(U), then I != Part(I \cup F), and therefore by iterative application of Theorem 1 there exists an item x \in F such that I != Part(I \cup x).
// x is necessarily in some pile P_i; by Corollary 1.1 P_i is an interesting pile, contradicting the assumption that global optimization was complete.

// Corollary: Global optimization is independant of the order in which piles are visited, or how items are initially divided into piles.



static void _get_valuators(vector<InventoryValuator*>& valuators) {
	// Get zero or more from each analyzer; Loot's own valuator(s) will handle encumbrance and other limits
}

static int _valuate(vector<InventoryValuator*>& valuators, const Item& item, bool save) {
	int res = 0;
	for (vector<InventoryValuator*>::iterator vi = valuators.begin(); vi != valuators.end(); ++vi)
		res += (*vi)->addItem(item, save);
	return res;
}

// This function implements Part() in the above description for a general set of items.
// It assumes that picking up a gold piece will never make another item more valuable
// This tries to be systematically biased to break ties in favor of items earlier in the list.  We might need a more formal tiebreak procedure.
// If spectators is non-null it points to a vector of items which are to be tested for relevance; spectator_out[i] = true iff Part(I \cup spectators[i]) != Part(I)
static void _optimize_partition(vector<int>& out, const vector<pair<int,Item> >& possibilities, vector<bool>* spectator_out, const vector<Item>* spectators) {
	vector<InventoryValuator*> valuators;
	_get_valuators(valuators);

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
			int candscore = _valuate(valuators, possibilities[i].second, false);
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
				int specscore = _valuate(valuators, (*spectators)[i], false);
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
				out[bestnext]++;
				score = _valuate(valuators, possibilities[bestnext].second, true);

				bestscore = _valuate(valuators, possibilities[bestnext].second, false);
			} while (out[bestnext] < possibilities[bestnext].first &&
					((bestscore - score) > sndmarg || ((bestscore - score) == sndmarg && bestnext < sndnext)));
		} else {
			// No assumptions made so we can only add one before reentering the selection loop
			out[bestnext]++;
			score = _valuate(valuators, possibilities[bestnext].second, true);
		}
	}

	for (vector<InventoryValuator*>::iterator it = valuators.begin(); it != valuators.end(); ++it)
		delete *it;
}
