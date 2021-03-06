#ifndef ANALYZER_SOKOBAN_H
#define ANALYZER_SOKOBAN_H

#include <map>
#include <deque>
#include <vector>
#include "Point.h"
#include "Analyzers/Analyzer.h"

#define SOKOBAN_SOLVE_PRIORITY 100
#define SOKOBAN_CLEAR_ITEMS_PRIORITY 200
#define SOKOBAN_REST_PRIORITY 25

namespace analyzer {

	class Sokoban : public Analyzer {
	public:
		Sokoban();

		void analyze(void);
		void parseMessages(const std::string& messages);

	private:
		std::vector<std::deque<std::pair<Point, char> > > _moves;
		std::map<int, int> _levelmap;
		int _retry_count;
		int _retry_turn;
		bool _given_up;

		void addMoves(int level, Point& boulder, const std::string& moves);
	};
}
#endif
