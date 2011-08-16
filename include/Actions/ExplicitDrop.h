#ifndef ACTION_EXPLICIT_DROP_H
#define ACTION_EXPLICIT_DROP_H

#include "Actions/Action.h"
#include "Item.h"
#include <vector>


namespace action {
	class ExplicitDrop : public Action {
	public:
		static const int ID;

		ExplicitDrop(analyzer::Analyzer* analyzer, int priority, const std::vector<std::pair<int, Item> >& what);
		virtual ~ExplicitDrop();

		virtual int id();
		virtual const Command& command();
		virtual void update(const std::string& messages);

	private:
		Command _drop;
		Command _look;
		Command _page_select;
		std::vector<std::pair<int, Item> > _drop_what;
	};
}
#endif
