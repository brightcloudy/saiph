#ifndef ACTION_PRAY_H
#define ACTION_PRAY_H

#include "Saiph.h"
#include "World.h"
#include "Actions/Action.h"

#define PRAY_PRAYER_TIMEOUT 1000
#define PRAY_MAJOR_TROUBLE 800

namespace action {

	class Pray : public Action {
	public:
		static const int ID;

		Pray(analyzer::Analyzer* analyzer, int priority) : Action(analyzer), _pray("#pray\n", priority) {
		}

		virtual ~Pray() {
		}

		static bool isSafeToPray() {
			return World::turn() - PRAY_PRAYER_TIMEOUT > Saiph::lastPrayed();
		}
		
		static bool isSafeToPrayMajorTrouble() {
			return World::turn() - PRAY_MAJOR_TROUBLE > Saiph::lastPrayed();
		}

		virtual int id() {
			return ID;
		}

		virtual const Command& command() {
			switch (_sequence) {
			case 0:
				return _pray;

			default:
				return Action::NOOP;
			}
		}

		virtual void update(const std::string& messages) {
			if (messages.find(MESSAGE_YOU_FINISH_YOUR_PRAYER) != std::string::npos) {
				Saiph::lastPrayed(World::turn());
				_sequence = 1;
			}
		}

	private:
		const Command _pray;
	};
}
#endif
