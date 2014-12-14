#include <math.h>

#include "filter.h"

FilterParameters PrecalculateFilter(float f)
{
	float w0 = 2.0*3.141592654*f;

	float Q = 2.8;
	float alpha = sinf(w0)/(2.0*Q);

	float b0 = 1;
	float b1 = -2.0*cosf(w0);
	float b2 = 1;
	float a0 = 1.0 + alpha;
	float a1 = -2.0*cosf(w0);
	float a2 = 1.0 - alpha;

	float scaling = float(1 << 25) / a0;

	FilterParameters result = {
			.a1 = int32_t(floor((a1 * scaling) + 0.5)),
			.a2 = int32_t(floor((a2 * scaling) + 0.5)),
			.b0 = int32_t(floor((b0 * scaling) + 0.5)),
			.b1 = int32_t(floor((b1 * scaling) + 0.5)),
			.b2 = int32_t(floor((b2 * scaling) + 0.5))
	};

	return result;
}

int32_t Filter(int32_t in, FilterState& state, const FilterParameters& params)
{
	int64_t scaledin = int64_t(in) << 5;
	int64_t out = 0;
	out += (int64_t(params.b0) * scaledin);
	out += (int64_t(params.b1) * int64_t(state.x1));
	out += (int64_t(params.b2) * int64_t(state.x2));
	out -= (int64_t(params.a1) * int64_t(state.y1));
	out -= (int64_t(params.a2) * int64_t(state.y2));

	out >>= 25;

	state.x2 = state.x1;
	state.x1 = scaledin;
	state.y2 = state.y1;
	state.y1 = out;

	return out >> 5;
}

