#ifndef ACTION_SEARCH_H
#define ACTION_SEARCH_H

#include "../World.h"
#include "Actions/Action.h"

#define MESSAGE_YOU_STOP_SEARCHING "  You stop searching.  "

namespace action {

	class Search : public Action {
	public:
		static const int ID;

		Search(analyzer::Analyzer* analyzer, int priority) : Action(analyzer), _search("16s", priority), _first_search_turn(World::turn()) {
		}

		virtual ~Search() {
		}

		virtual int id() {
			return ID;
		}

		virtual const Command& command() {
			switch (_sequence) {
			case 0:
				return _search;

			default:
				return Action::NOOP;
			}
		}

		virtual void update(const std::string& messages) {
			if (_sequence == 0) {
				/* increase search counter on level */
				if (messages.find(MESSAGE_YOU_STOP_SEARCHING) != std::string::npos)
					World::level().increaseAdjacentSearchCount(Saiph::position(), estimateSearchesMade());
				else
					World::level().increaseAdjacentSearchCount(Saiph::position(), 16);
				_sequence = 1;
			}
		}

	private:
		const Command _search;
		const int _first_search_turn;

		int estimateSearchesMade() {
			/* worst-case estimate based on elapsed turns */
			/* TODO: account for speed, which gives us some guaranteed movement points */
			return World::turn() - _first_search_turn;
		}
	};
}
#endif
