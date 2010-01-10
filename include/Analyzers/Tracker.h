#ifndef TRACKER_H
#define	TRACKER_H

#include "Analyzers/Analyzer.h"
#include "EventBus.h"
#include "Events/PriceLearned.h"
#include <string>
#include <set>
#include <map>
#include <queue>
#include <algorithm>
#include <iterator>
#include <utility>

namespace analyzer {

	/*
	 * ItemType must be derived from data::Item, e.g.
	 * class Wand : public Tracker<data::Wand>
	 */
	template<class ItemType>
	class Tracker : Analyzer {
	public:
		Tracker() : _one_to_one_mapping (ItemType::items().size() == ItemType::appearances().size()) {
			const std::set<const ItemType*>& identities = ItemType::items();
			const std::set<const std::string>& appearances = ItemType::appearances();

			for (std::set<const ItemType*>::const_iterator i = identities.begin(); i != identities.end(); i++) {
				for (std::set<const std::string>::const_iterator j = appearances.begin(); j != appearances.end(); j++) {
					_possible_identities[*j] = *i;
				}
				
				_price_groups[(*i)->cost()].insert(*i);
			}

			EventBus::registerEvent(event::PriceLearned::ID, this);
		}

		virtual ~Tracker() {
			EventBus::unregisterEvent(event::PriceLearned::ID, this);
		}

		virtual void analyze() {
			//TODO: name items when we make deductions
		}

		virtual void onEvent(event::Event* const event) {
			if (event->id() == event::PriceLearned::ID) {
				event::PriceLearned* price_event = static_cast<event::PriceLearned* const>(event);
				const std::string& item = price_event->item();
				const std::set<const int>& prices = price_event->prices();

				//if it isn't one of our items, skip it
				if (_possible_identities.find(item) == _possible_identities.end())
					return;

				std::set<const ItemType*> possibilities;
				for (const std::set<const int>::const_iterator i = prices.begin(); i != prices.end(); i++) {
					for (set_ci j = _price_groups[*i].begin(); j != _price_groups[*i].end(); j++)
						possibilities.insert(*j);
				}

				constrainWithin(price_event->item(), possibilities);
			}
		}
	protected:

		/*
		 * Appearance is item.
		 */
		void set(const std::string& appearance, const ItemType* item) {
			//add it to the name queue
			_name_queue.push(std::make_pair(appearance, item));

			//remove all other items for this appearance
			for (set_ci i = _possible_identities[appearance].begin(); i != _possible_identities[appearance].end();)
				if (*i != item)
					_possible_identities[appearance].erase(i++);
				else
					++i; //advance the loop

			//no other appearances can have this item
			for (map_ci i = _possible_identities.begin(); i != _possible_identities.end(); i++)
				if (i->first != appearance)
					if (i->second.erase(item) && _one_to_one_mapping && i->second.size() == 1)
						set(i->first, *(i->second.begin()));

			checkIdentityOnlyPossibleForOneAppearance(appearance);
		}

		/*
		 * Appearance is not item.
		 */
		void ruleOut(const std::string& appearance, const ItemType* item) {
			if (_possible_identities[appearance].erase(item)) {
				if (_one_to_one_mapping && _possible_identities[appearance].size() == 1)
					set(appearance, *(_possible_identities[appearance].begin()));
				
				checkIdentityOnlyPossibleForOneAppearance(appearance);
			}
		}

		/*
		 * Appearance is one of the possibilities inside the set.
		 *
		 * This is most commonly used for price groups.
		 */
		void constrainWithin(const std::string& appearance, const std::set<const ItemType*>& set) {
			std::set<const ItemType*> toBeRuledOut = complement(set);
			for (set_ci i = toBeRuledOut.begin(); i != toBeRuledOut.end(); i++)
				ruleOut(appearance, *i);
		}
	private:
		/*
		 * Some item classes have more appearances than identities (e.g., scrolls),
		 * while others have exactly as many appearances as identities. In the
		 * latter case, we can make a deduction when an appearance has only one
		 * possible identity.
		 */
		const bool _one_to_one_mapping;
		/*
		 * Since we make deductions in onEvent() or our derived class' parseMessages(),
		 * keep a queue of #names to issue in our analyze().
		 */
		std::queue<const std::pair<const std::string, const ItemType*> > _name_queue;
		/*
		 * A map from each price to the set of items having that price, for
		 * price-ID purposes.
		 */
		std::map<const int, std::set<const ItemType*> > _price_groups;
		/*
		 * A map from appearance to the set of possible identities, and typedefs
		 * to make loops more readable.
		 */
		std::map<const std::string, std::set<const ItemType*> > _possible_identities;
		typedef std::map<const std::string, std::set<const ItemType*> >::const_iterator map_ci;
		typedef std::set<const ItemType*>::const_iterator set_ci;

		/*
		 * All identities must have an appearance, so if an identity is only
		 * possible for one appearance, that appearance must have that identity.
		 *
		 * Call this whenever a possibility is removed from an appearance.
		 */
		void checkIdentityOnlyPossibleForOneAppearance(const std::string& appearance) {
			//if any identity is only possible for one appearance, set it
			for (set_ci i = ItemType::items().begin(); i != ItemType::items().end(); i++) {
				std::string app = "";
				bool keep_looping = true;
				for (map_ci j = _possible_identities.begin(); j != _possible_identities.end() && keep_looping; j++)
					if (j->second.find(*i) != j->second.end()) {
						if (app != "") //this is the second appearance for this identity
							keep_looping = false;
						else
							app = j->first;
					}
				if (app != "" && keep_looping) //exactly one appearance for this identity!
					set(app, *i);
			}
		}

		/*
		 * Returns the complement of the set: all the items of this item type
		 * that are not present in the set.
		 */
		std::set<const ItemType*> complement(const std::set<const ItemType*>& set) {
			std::set<const ItemType*> result;
			//{all items} - set = set complement
			std::set_difference(ItemType::items().begin(), ItemType::items.end(),
				set.begin(), set.end(),
				std::inserter(result, result.end()));
			return result;
		}
	};

}

#endif	/* TRACKER_H */

