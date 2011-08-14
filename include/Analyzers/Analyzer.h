#ifndef ANALYZER_ANALYZER_H
#define ANALYZER_ANALYZER_H

#include <string>
#include <vector>

#define ILLEGAL_ANALYZER_ID -1

namespace event {
	class Event;
}

namespace action {
	class Action;
}

class Item;

namespace analyzer {
	// Interface for analyzers to communicate desire for items.
	// An InventoryValuator instance tracks an "incomplete inventory" and assigns a value to it.
	// At creation, the valuator represents an empty inventory with a value of 0.
	// Items are added one at a time using addItem, which should return the new value; if save is false, then the valuator's state should be unchanged, allowing items to be tested for value.
	// The sum of all used valuators is used to determine composite inventory value; Loot aims to maximize this value.
	// already is the number of these items already added from the stack in question; we use this to estimate inventory slots used, conservatively assuming no new stacking
	class InventoryValuator {
	public:
		InventoryValuator();
		virtual ~InventoryValuator();

		virtual int addItem(const Item& i, int already, bool save) = 0;
	};

	class Analyzer {
	public:
		Analyzer(const std::string& name);
		virtual ~Analyzer();

		static void init();
		static void destroy();
		const std::string& name();
		virtual void parseMessages(const std::string&);
		virtual void analyze();
		virtual void lastChance(action::Action* const);
		virtual void createValuators(std::vector<InventoryValuator*>& into);
		virtual void onEvent(event::Event* const);
		virtual void actionCompleted(const std::string& messages);
		virtual void actionFailed();

	private:
		static std::vector<Analyzer*> _analyzers;
		std::string _name;
	};
}
#endif
