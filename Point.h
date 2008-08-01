#ifndef POINT_H
/* defines */
#define POINT_H

/* foward declare */
class Point;

/* includes */

/* namespaces */
using namespace std;

/* Point */
class Point {
	public:
		/* variables */
		int row;
		int col;

		/* constructors */
		Point(int row = -1, int col = -1);

		/* operator overloading */
		bool operator<(const Point &point) const;
		bool operator>(const Point &point) const;
		bool operator<=(const Point &point) const;
		bool operator>=(const Point &point) const;
		bool operator==(const Point &point) const;
		bool operator!=(const Point &point) const;
};
#endif
