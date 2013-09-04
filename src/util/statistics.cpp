#include "util/statistics.h"

#include <algorithm>
#include <stddef.h>
#include <float.h>

using namespace std;

void openxc::util::statistics::initialize(Statistic* stat) {
    stat->min = FLT_MAX;
    stat->max = FLT_MIN;
    stat->movingAverage = 0;
    stat->alpha = .1;
}

void openxc::util::statistics::update(Statistic* stat, float newValue) {
    stat->min = min(newValue, stat->min);
    stat->max = max(newValue, stat->max);

    stat->movingAverage = (stat->alpha * newValue) +
            (1.0 - stat->alpha) * stat->movingAverage;
}
