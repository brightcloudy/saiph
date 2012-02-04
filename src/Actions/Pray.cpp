#include "Actions/Pray.h"
#include "Data/Monster.h"

#include "Saiph.h"
#include "World.h"
#include "Item.h"

using namespace action;
using namespace std;

/* constructors/destructor */
Pray::Pray(analyzer::Analyzer* analyzer, int priority) : Action(analyzer, TIME_NOMUL), _pray("#pray\n", priority) {
}

Pray::~Pray() {
}

/* methods */
bool Pray::isSafeToPray() {
	return (World::turn() - PRAY_PRAYER_TIMEOUT > Saiph::lastPrayed()) && prayerCanSucceed();
}

bool Pray::isSafeToPrayMajorTrouble() {
	return (World::turn() - PRAY_MAJOR_TROUBLE > Saiph::lastPrayed()) && prayerCanSucceed();
}

bool Pray::prayerCanSucceed() {
	if (Saiph::polymorphed()) {
		if (data::Monster::monster(Saiph::race())->symbol() == S_DEMON)
			return false;
	}
	return Saiph::anger() == 0;
}

int Pray::id() {
	return ID;
}

const Command& Pray::command() {
	switch (_sequence) {
	case 0:
		return _pray;

	default:
		return Action::NOOP;
	}
}

void Pray::update(const std::string& messages) {
	if (messages.find(MESSAGE_YOU_FINISH_YOUR_PRAYER) != std::string::npos) {
		Saiph::lastPrayed(World::turn());
		_sequence = 1;
	}
}
