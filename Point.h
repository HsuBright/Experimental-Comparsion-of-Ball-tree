
#ifndef POINT_H
#define POINT_H

#include <cassert>
#include <cmath>
#include <vector>

using Point = std::vector<double>;

// Squared Euclidean distance
inline double squaredDistance(
    const Point& a, const Point& b
) {
    assert(a.size() == b.size());

    double sum = 0.0;

    for (size_t i = 0; i < a.size(); ++i) {
        double diff = a[i] - b[i];
        sum += diff * diff;
    }

    return sum;
}

// Euclidean distance
inline double pointDistance(
    const Point& a, const Point& b
) {
    return std::sqrt(squaredDistance(a, b));
}

#endif
