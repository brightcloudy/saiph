#include "Analyzers/Analyzer.h"

#include "Analyzers/Amulet.h"
#include "Analyzers/Armor.h"
#include "Analyzers/Beatitude.h"
#include "Analyzers/Boulder.h"
#include "Analyzers/DiggingTool.h"
#include "Analyzers/Donate.h"
#include "Analyzers/Door.h"
#include "Analyzers/Elbereth.h"
#include "Analyzers/Enhance.h"
#include "Analyzers/Excalibur.h"
#include "Analyzers/Explore.h"
#include "Analyzers/Fight.h"
#include "Analyzers/Food.h"
#include "Analyzers/Health.h"
#include "Analyzers/Lamp.h"
#include "Analyzers/Loot.h"
#include "Analyzers/Medusa.h"
#include "Analyzers/MonsterInfo.h"
#include "Analyzers/Quest.h"
#include "Analyzers/RandomWalk.h"
#include "Analyzers/Shop.h"
#include "Analyzers/Sokoban.h"
#include "Analyzers/Vault.h"
#include "Analyzers/Wand.h"
#include "Analyzers/Weapon.h"
#include "Analyzers/Wish.h"
#include "World.h"

using namespace analyzer;

/* static variables */
std::vector<Analyzer*> Analyzer::_analyzers;

/* constructors/destructor */
Analyzer::Analyzer(const std::string& name) : _name(name) {
}

Analyzer::~Analyzer() {
}

/* static methods */
void Analyzer::init() {
	/* init analyzers */
	_analyzers.push_back(new Amulet());
	_analyzers.push_back(new Armor());
	_analyzers.push_back(new Beatitude());
	_analyzers.push_back(new Boulder());
	_analyzers.push_back(new Donate());
	_analyzers.push_back(new DiggingTool());
	_analyzers.push_back(new Door());
	_analyzers.push_back(new Elbereth());
	_analyzers.push_back(new Enhance());
	_analyzers.push_back(new Excalibur());
	_analyzers.push_back(new Explore());
	_analyzers.push_back(new Fight());
	_analyzers.push_back(new Food());
	_analyzers.push_back(new Health());
	_analyzers.push_back(new Lamp());
	_analyzers.push_back(new Loot());
	_analyzers.push_back(new Medusa());
	_analyzers.push_back(new MonsterInfo());
	_analyzers.push_back(new Quest());
	_analyzers.push_back(new RandomWalk());
	_analyzers.push_back(new Shop());
	_analyzers.push_back(new Sokoban());
	_analyzers.push_back(new Vault());
	_analyzers.push_back(new Wand());
	_analyzers.push_back(new Weapon());
	_analyzers.push_back(new Wish());

	for (std::vector<Analyzer*>::iterator a = _analyzers.begin(); a != _analyzers.end(); ++a)
		World::registerAnalyzer(*a);
}

void Analyzer::destroy() {
	for (std::vector<Analyzer*>::iterator a = _analyzers.begin(); a != _analyzers.end(); ++a)
		delete *a;
}

/* methods */
const std::string& Analyzer::name() {
	return _name;
}

void Analyzer::parseMessages(const std::string&) {
}

void Analyzer::analyze() {
}

void Analyzer::createValuators(std::vector<InventoryValuator*>&) {
}

void Analyzer::lastChance(action::Action* const) {
}

void Analyzer::onEvent(event::Event* const) {
}

void Analyzer::actionCompleted(const std::string&) {
}

void Analyzer::actionFailed() {
}

/* inner classes */
InventoryValuator::InventoryValuator() { }
InventoryValuator::~InventoryValuator() { }

// points should refer to an array like { { 0, 0 }, { 100, 100 }, { 1000, 200 }, { -1, -1 } }
// min. 2 rows before sentinel; first row probably should start at 0; each row is a control point
// past the last control point piecewiseLinear does extrapolation
int InventoryValuator::piecewiseLinear(int in, const int points[][2]) {
	int range = 0;

	if (in < 0)
		return points[0][1];

	while (points[range+2][0] >= 0 && in >= points[range+1][0])
		++range;

	return points[range][1] + (points[range+1][1] - points[range][1]) * (in - points[range][0]) / (points[range+1][0] - points[range][0]);
}
