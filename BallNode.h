
#ifndef BALLNODE_H
#define BALLNODE_H

#include "Point.h"

struct BallNode {
    Point center;
    double radius = 0.0;

    BallNode* left = nullptr;
    BallNode* right = nullptr;

    // -1 for internal nodes
    int pointIndex = -1;

    int centerIndex = -1;
};

#endif
