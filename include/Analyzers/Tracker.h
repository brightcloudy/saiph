#ifndef TRACKER_H
#define	TRACKER_H

#include "Actions/Call.h"
#include "Analyzers/Analyzer.h"
#include "EventBus.h"
#include "Events/PriceLearned.h"
#include "Item.h"
#include "World.h"
#include <string>
#include <set>
#include <map>
#include <vector>
#include <algorithm>
#include <iterator>
#include <utility>

namespace analyzer {

	/*
	 * ItemType must be derived from data::Item, e.g.
	 * class Wand : public Tracker<data::Wand>
	 */
	template<class ItemType>
	class Tracker : public Analyzer {
	public:
		Tracker(const std::string& analyzerName) : Analyzer(analyzerName), _one_to_one_mapping(ItemType::items().size() == ItemType::appearances().size()) {
			const std::map<const std::string, const ItemType*>& identities = ItemType::items();
			const std::vector<std::string>& appearances = ItemType::appearances();

			for (typename std::map<const std::string, const ItemType*>::const_iterator i = identities.begin(); i != identities.end(); i++) {
				for (typename std::vector<std::string>::const_iterator j = appearances.begin(); j != appearances.end(); j++) {
					_possible_identities[*j].insert(i->second);
				}
				
				_price_groups[i->second->cost()].insert(i->second);
			}

			EventBus::registerEvent(event::PriceLearned::ID, this);
		}

		virtual ~Tracker() {
			EventBus::unregisterEvent(event::PriceLearned::ID, this);
		}

		virtual void analyze() {
			for (std::map<unsigned char, Item>::const_iterator i = Inventory::items().begin(); i != Inventory::items().end(); i++) {
				std::set<std::string>::const_iterator appearance = _name_queue.find(i->second.name());
				if (appearance != _name_queue.end()) {
					const ItemType* identity = *(_possible_identities[*appearance].begin());
					World::queueAction(new action::Call(this, i->first, identity->name()));
					_name_queue.erase(appearance);
				}
			}
		}

		virtual void onEvent(event::Event* const event) {
			if (event->id() == event::PriceLearned::ID) {
				event::PriceLearned* price_event = static_cast<event::PriceLearned* const>(event);
				const std::string& item = price_event->item();
				const std::set<int>& prices = price_event->prices();

				//if it isn't one of our items, skip it
				if (_possible_identities.find(item) == _possible_identities.end())
					return;

				std::set<const ItemType*> possibilities;
				for (std::set<int>::const_iterator i = prices.begin(); i != prices.end(); i++) {
					for (set_ci j = _price_groups[*i].begin(); j != _price_groups[*i].end(); j++)
						possibilities.insert(*j);
				}

				constrainWithin(price_event->item(), possibilities);
			}
		}

		/*
		 * Returns true if we know this item's appearance.
		 * e.g., checking if we've seen scrolls of charging before making a wand-wish.
		 */
		bool isIdentified(const std::string& identity) {
			int appearances = 0;
			for (map_ci i = _possible_identities.begin(); i != _possible_identities.end(); i++) {
				for (set_ci j = i->second.begin(); j != i->second.end(); j++)
					if (j->name() == identity)
						appearances++;
			}
			return appearances == 1;
		}
	protected:

		/*
		 * Appearance is item.
		 */
		void set(const std::string& appearance, const ItemType* item) {
			//add it to the name queue
			_name_queue.insert(appearance);

			//remove all other items for this appearance
			for (set_ci i = _possible_identities[appearance].begin(); i != _possible_identities[appearance].end();)
				if (*i != item)
					_possible_identities[appearance].erase(i++);
				else
					++i; //advance the loop

			//no other appearances can have this item
			for (map_i i = _possible_identities.begin(); i != _possible_identities.end(); i++)
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
		 * This is most commonly used for price groups, but might also be used
		 * for the ambiguous wand engrave messages etc.
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
		 * Since we might not have an item in our inventory when we make a deduction
		 * about it, save it in the #name queue.
		 */
		std::set<std::string> _name_queue;
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
		typedef typename std::map<const std::string, std::set<const ItemType*> >::const_iterator map_ci;
		typedef typename std::map<const std::string, std::set<const ItemType*> >::iterator map_i;
		typedef typename std::set<const ItemType*>::const_iterator set_ci;

		/*
		 * All identities must have an appearance, so if an identity is only
		 * possible for one appearance, that appearance must have that identity.
		 *
		 * Call this whenever a possibility is removed from an appearance.
		 */
		void checkIdentityOnlyPossibleForOneAppearance(const std::string& appearance) {
			//if any identity is only possible for one appearance, set it
			for (typename std::map<const std::string, const ItemType*>::const_iterator i = ItemType::items().begin(); i != ItemType::items().end(); i++) {
				std::string app = "";
				bool keep_looping = true;
				for (map_ci j = _possible_identities.begin(); j != _possible_identities.end() && keep_looping; j++)
					if (j->second.find(i->second) != j->second.end()) {
						if (app != "") //this is the second appearance for this identity
							keep_looping = false;
						else
							app = j->first;
					}
				if (app != "" && keep_looping) //exactly one appearance for this identity!
					set(app, i->second);
			}
		}

		/*
		 * Returns the complement of the set: all the items of this item type
		 * that are not present in the set.
		 */
		std::set<const ItemType*> complement(const std::set<const ItemType*>& set) {
			std::set<const ItemType*> result;
			//{all items} - set = set complement
			//if ItemType::items() was a std::set<ItemType*>, I'd use
			//std::set_difference here, but I have to write it out
			for (typename std::map<const std::string, const ItemType*>::const_iterator i = ItemType::items().begin(); i != ItemType::items().end(); i++)
				result.insert(i->second);
			for (set_ci i = set.begin(); i != set.end(); i++)
				result.erase(*i);
			return result;
		}
	};

}

#endif	/* TRACKER_H */

