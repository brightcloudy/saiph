#ifndef ACTION_EXPLICIT_PICKUP_H
#define ACTION_EXPLICIT_PICKUP_H

#include "Actions/Action.h"
#include "Item.h"
#include <vector>


namespace action {
	class ExplicitPickup : public Action {
	public:
		static const int ID;

		ExplicitPickup(analyzer::Analyzer* analyzer, int priority, const std::vector<std::pair<int, Item> >& what);
		virtual ~ExplicitPickup();

		static bool canPickup();
		virtual int id();
		virtual const Command& command();
		virtual void update(const std::string& messages);

	private:
		Command _variable;
		Command _look;
		std::vector<std::pair<int, Item> > _pick_what;
	};
}
#endif
