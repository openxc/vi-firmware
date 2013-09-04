#include "util/statistics.h"

#include <algorithm>
#include <stddef.h>
#include <float.h>

void openxc::util::statistics::initialize(DeltaStatistic* stat) {
    stat->total = 0;
    initialize(&stat->statistic);
}

void openxc::util::statistics::initialize(Statistic* stat) {
    stat->min = FLT_MAX;
    stat->max = FLT_MIN;
    stat->movingAverage = 0;
    stat->alpha = .1;
}

void openxc::util::statistics::update(Statistic* stat, float newValue) {
    stat->min = std::min(newValue, stat->min);
    stat->max = std::max(newValue, stat->max);

    stat->movingAverage = (stat->alpha * newValue) +
            (1.0 - stat->alpha) * stat->movingAverage;
}

void openxc::util::statistics::update(DeltaStatistic* stat, float newValue) {
    float delta = newValue - stat->total;
    stat->total = newValue;
    update(&stat->statistic, delta);
}

float openxc::util::statistics::exponentialMovingAverage(const Statistic* stat) {
    return stat->movingAverage;
}

float openxc::util::statistics::exponentialMovingAverage(const DeltaStatistic* stat) {
    return exponentialMovingAverage(&stat->statistic);
}

float openxc::util::statistics::minimum(const Statistic* stat) {
    return stat->min;
}

float openxc::util::statistics::minimum(const DeltaStatistic* stat) {
    return stat->statistic.min;
}

float openxc::util::statistics::maximum(const Statistic* stat) {
    return stat->max;
}

float openxc::util::statistics::maximum(const DeltaStatistic* stat) {
    return stat->statistic.max;
}
