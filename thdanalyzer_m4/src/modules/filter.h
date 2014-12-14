#ifndef FILTER_H_
#define FILTER_H_

#include <stdint.h>

struct FilterParameters
{
	int32_t a1, a2, b0, b1, b2;
};

struct FilterState
{
	int64_t x1, x2;
	int64_t y1, y2;

	FilterState() {
		x1 = 0;
		x2 = 0;
		y1 = 0;
		y2 = 0;
	}
};

FilterParameters PrecalculateFilter(float f);
int32_t Filter(int32_t in, FilterState& state, const FilterParameters& params);

#endif /* FILTER_H_ */
