#ifndef ANALYZER_LOOT_H
#define ANALYZER_LOOT_H

#include <set>
#include <string>
#include <vector>
#include "Coordinate.h"
#include "Analyzers/Analyzer.h"

#define PRIORITY_LOOT_VISIT 200
#define PRIORITY_LOOT_DROP 150

#define PRIORITY_LOOT_PICKUP_DROP 200
#define PRIORITY_LOOT_DROP_CONTINUE 300 // higher than wearing armor

namespace event {
	class Event;
}

class Item;

namespace analyzer {

	class Loot : public Analyzer {
	public:
		Loot();

		void analyze();
		void parseMessages(const std::string& messages);
		void onEvent(event::Event* const event);
		virtual void createValuators(std::vector<InventoryValuator*>& to);

	private:
		std::set<Coordinate> _visit;

		void getValuators(std::vector<InventoryValuator*>& valuators);
		int valuate(std::vector<InventoryValuator*>& valuators, const Item& item, int already, bool save);
		void optimizePartition(std::vector<int>& out, const std::vector<Item>& possibilities, const std::vector<int>& forced, std::vector<bool>* spectator_out, const std::vector<Item>* spectators);
		void partitionFloorInventory();
		void analyzeInventory(std::vector<Item>& cands, std::vector<int>& forced);
	};
}
#endif
