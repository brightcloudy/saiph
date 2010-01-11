#ifndef ANALYZER_WAND_H
#define ANALYZER_WAND_H

#include <string>
#include <vector>
#include "Item.h"
#include "Analyzers/Analyzer.h"
#include "Analyzers/Tracker.h"
#include "Data/Wand.h"

#define WAND_DEBUG_NAME "Wand] "

#define WAND_STATE_INIT 0
#define WAND_STATE_DUST_X 1
#define WAND_STATE_WANTS_LOOK 2
#define WAND_STATE_CONFIRM_LOOK 6
#define WAND_STATE_ENGRAVING 3
#define WAND_STATE_READY_TO_NAME 4
#define WAND_STATE_WANT_DIRTY_INVENTORY 5


namespace analyzer {
	class Wand : public Tracker<data::Wand> {
	public:
		Wand();

		virtual void parseMessages(const std::string& messages);
		virtual void analyze();
	private:
		std::map<const std::string, std::set<const data::Wand*> > _engrave_groups;

		bool engraveUseful(const std::string& appearance);
	};
}
#endif
