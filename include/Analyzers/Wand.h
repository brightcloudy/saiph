#ifndef ANALYZER_WAND_H
#define ANALYZER_WAND_H

#include <string>
#include <vector>
#include "Item.h"
#include "Analyzers/Analyzer.h"
#include "Analyzers/Tracker.h"
#include "Data/Wand.h"

#define PRIORITY_WAND_ENGRAVE_IDENTIFY 200

#define WAND_POLYMORPH_NAME "wand of polymorph"
#define WAND_VANISHER_NAME "wand of vanishing"
#define WAND_SLEEP_DEATH_NAME "wand of sleep/death"
#define WAND_NO_EFFECT_NAME "wand of no engrave effect"

#define WAND_STATE_INIT 0
#define WAND_STATE_DUST_X 1
#define WAND_STATE_WANTS_LOOK 2
#define WAND_STATE_CONFIRM_LOOK 6
#define WAND_STATE_ENGRAVING 3
#define WAND_STATE_READY_TO_NAME 4
#define WAND_STATE_WANT_DIRTY_INVENTORY 5
#define WAND_DEBUG_NAME "Wand] "

#define WAND_STATE_PREPARE 0
#define WAND_STATE_DUST_E 1
#define WAND_STATE_ENGRAVE_WITH_WAND 2
#define WAND_STATE_PARSE_MESSAGES 3

namespace analyzer {
	class Wand : public Tracker<data::Wand> {
	public:
		Wand();

		virtual void parseMessages(const std::string& messages);
		virtual void analyze();
		virtual void actionCompleted(const std::string& messages);
		virtual void actionFailed();
		virtual void onEvent(event::Event* const event);
	private:
		std::map<const std::string, std::set<const data::Wand*> > _engrave_groups;
//		std::map<const int, std::set<const data::Wand*> > _zap_type_groups;
		//the inventory letters of unidentified wands in our inventory
		//TODO: when zap-type ID is implemented, change this to _engrave_useful_wands
		std::set<unsigned char> _unidentified_wands;
		int _state;
		unsigned char _wand_key;

		bool engraveUseful(const std::string& appearance);
//		bool zapUseful(const std::string& appearance);
		void refreshUnidentifiedWands();
	};
}
#endif
