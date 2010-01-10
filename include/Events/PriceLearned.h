#ifndef PRICELEARNED_H
#define	PRICELEARNED_H

#include <string>
#include <set>

namespace event {
	class PriceLearned : public Event {
	public:
		static const int ID;

		PriceLearned(const std::string& item, std::set<const int> prices) : Event("PriceLearned"), _item(item), _prices(prices) {
		}
		virtual ~PriceLearned() {}

		virtual int id() {
			return ID;
		}

		const std::string& item() {
			return _item;
		}

		const std::set<const int>& prices() {
			return _prices;
		}
	private:
		const std::string _item;
		const std::set<const int> _prices;
	};
}

#endif	/* PRICELEARNED_H */

