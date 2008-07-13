#include "Explore.h"

/* constructors */
Explore::Explore(Saiph *saiph) {
	this->saiph = saiph;
	memset(search, 0, sizeof (search));
	memset(ep_added, false, sizeof (ep_added));
	memset(visited, false, sizeof (visited));
	move = ILLEGAL_MOVE;
	vector<unsigned char> symbols;
	symbols.push_back(CORRIDOR);
	symbols.push_back(FLOOR);
	symbols.push_back(OPEN_DOOR);
	symbols.push_back(PLAYER);
	saiph->registerAnalyzerSymbols(this, symbols);
}

/* methods */
void Explore::command(string *command) {
	if (move == SEARCH)
		++search[saiph->current_branch][saiph->current_level][saiph->world->row][saiph->world->col];
	command->push_back(move);
}

int Explore::finish() {
	/* figure out which place to explore */
	int b = saiph->current_branch;
	int l = saiph->current_level;
	int best_priority = -1;
	int best_distance = INT_MAX;
	move = ILLEGAL_MOVE;
	for (list<Point>::iterator e = explore.begin(); e != explore.end(); ++e) {
		if (search[b][l][e->row][e->col] >= EXPLORE_SEARCH_COUNT) {
			/* this place is fully searched out. remove it from the list */
			explore.erase(e);
			continue;
		}
		unsigned char hs = saiph->map[b][l].dungeon[e->row][e->col - 1];
		unsigned char js = saiph->map[b][l].dungeon[e->row + 1][e->col];
		unsigned char ks = saiph->map[b][l].dungeon[e->row - 1][e->col];
		unsigned char ls = saiph->map[b][l].dungeon[e->row][e->col + 1];
		int priority = 1;
		int count = 0;
		switch (saiph->map[b][l].dungeon[e->row][e->col]) {
			case CORRIDOR:
				if (!visited[b][l][e->row][e->col]) {
					priority = EXPLORE_VISIT_CORRIDOR;
					break;
				}
				if (hs == SOLID_ROCK)
					++count;
				if (js == SOLID_ROCK)
					++count;
				if (ks == SOLID_ROCK)
					++count;
				if (ls == SOLID_ROCK)
					++count;
				if (count == 3) {
					/* dead end */
					priority = EXPLORE_SEARCH_CORRIDOR_DEAD_END;
				} else if (!((hs != SOLID_ROCK && ls != SOLID_ROCK) || (js != SOLID_ROCK && ks != SOLID_ROCK))) {
					/* turning corridor */
					priority = EXPLORE_SEARCH_CORRIDOR_CORNER;
				} else {
					/* this place is of no interest to us */
					explore.erase(e);
					continue;
				}
				break;

			case OPEN_DOOR:
				if (!visited[b][l][e->row][e->col]) {
					priority = EXPLORE_VISIT_OPEN_DOOR;
					break;
				}
				if (hs == SOLID_ROCK || js == SOLID_ROCK || ks == SOLID_ROCK || ls == SOLID_ROCK) {
					/* door with no exit */
					priority = EXPLORE_SEARCH_DOOR_DEAD_END;
				} else {
					/* door with exit, uninteresting */
					explore.erase(e);
					continue;
				}
				break;

			case FLOOR:
				if (visited[b][l][e->row][e->col] && search[b][l][e->row][e->col] >= EXPLORE_SEARCH_COUNT) {
					/* been here & searched, uninteresting place */
					explore.erase(e);
					continue;
				}
				if (hs == SOLID_ROCK || js == SOLID_ROCK || ks == SOLID_ROCK || ls == SOLID_ROCK) {
					/* next to unlit place */
					priority = EXPLORE_VISIT_UNLIT_AREA;
				} else if ((hs == VERTICAL_WALL || ls == VERTICAL_WALL) && (js == HORIZONTAL_WALL || ks == HORIZONTAL_WALL)) {
					/* corner of a room */
					priority = EXPLORE_SEARCH_ROOM_CORNER;
				} else if (hs == VERTICAL_WALL || js == HORIZONTAL_WALL || ks == HORIZONTAL_WALL || ls == VERTICAL_WALL) {
					/* wall next to floor */
					priority = EXPLORE_SEARCH_WALL;
				}
				break;

			default:
				/* this never happens */
				explore.erase(e);
				continue;
		}
		if (priority < best_priority)
			continue;
		int distance = 0;
		bool straight_line = false;
		unsigned char nextmove = saiph->shortestPath(*e, false, &distance, &straight_line);
		if (priority == best_priority && distance > best_distance)
			continue;
		if (nextmove == ILLEGAL_MOVE)
			continue;
		if (nextmove == REST)
			move = SEARCH;
		else
			move = nextmove;
		best_priority = priority;
		best_distance = distance;
	}
	return best_priority;
}

void Explore::inspect(int row, int col, unsigned char symbol) {
	int b = saiph->current_branch;
	int l = saiph->current_level;
	if (symbol == PLAYER) {
		/* make this place "visited" */
		visited[b][l][row][col] = true;
		return;
	}
	if (ep_added[b][l][row][col])
		return; // already added this place
	ep_added[b][l][row][col] = true;
	Point p;
	p.row = row;
	p.col = col;
	explore.push_back(p);
}