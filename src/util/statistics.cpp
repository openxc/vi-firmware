#include "util/statistics.h"

#include <limits.h>
#include <stddef.h>

#include "config.h"

void openxc::util::statistics::initialize(DeltaStatistic* stat) {
    stat->total = 0;
    initialize(&stat->statistic);
}

void openxc::util::statistics::initialize(Statistic* stat) {
    stat->min = INT_MAX;
    stat->max = INT_MIN;
    stat->movingAverage = 0;
    stat->alpha = .1;
}

void openxc::util::statistics::update(Statistic* stat, int newValue) {
    if(stat->min == INT_MAX && stat->max == INT_MIN) {
        stat->movingAverage = newValue;
    } else {
        stat->movingAverage = (stat->alpha * newValue) +
                (1.0 - stat->alpha) * stat->movingAverage;
    }

    stat->min = MIN(newValue, stat->min);
    stat->max = MAX(newValue, stat->max);
}

void openxc::util::statistics::update(DeltaStatistic* stat, int newValue) {
    int delta = newValue - stat->total;
    stat->total = newValue;
    update(&stat->statistic, delta);
}

float openxc::util::statistics::exponentialMovingAverage(const Statistic* stat) {
    return stat->movingAverage;
}

float openxc::util::statistics::exponentialMovingAverage(const DeltaStatistic* stat) {
    return exponentialMovingAverage(&stat->statistic);
}

int openxc::util::statistics::minimum(const Statistic* stat) {
    return stat->min;
}

int openxc::util::statistics::minimum(const DeltaStatistic* stat) {
    return stat->statistic.min;
}

int openxc::util::statistics::maximum(const Statistic* stat) {
    return stat->max;
}

int openxc::util::statistics::maximum(const DeltaStatistic* stat) {
    return stat->statistic.max;
}
