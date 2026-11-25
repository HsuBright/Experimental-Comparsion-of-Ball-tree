#include "BallTree.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <vector>

// Temporary bounding ball
struct Ball {
    Point center;
    double radius = 0.0;
};

// Merge two balls
static void mergeBalls(
    const Ball* a,
    const Ball* b,
    Ball* out
) {
    double d = pointDistance(a->center, b->center);

    if (d + b->radius <= a->radius) {
        *out = *a;
        return;
    }

    if (d + a->radius <= b->radius) {
        *out = *b;
        return;
    }

    out->center = a->center;
    out->radius = (d + a->radius + b->radius) / 2.0;

    double t = (out->radius - a->radius) / d;

    for (int j = 0; j < (int)out->center.size(); j++) {
        out->center[j] +=
            t * (b->center[j] - a->center[j]);
    }
}

// Find best split position
static int* splitTopDown(
    BallTree* tree,
    int* begin,
    int* end
) {
    int n = end - begin;
    int dim = (int)tree->points[0].size();

    double bestCost =
        std::numeric_limits<double>::infinity();

    int bestAxis = 0;
    int bestK = 1;

    std::vector<int> sorted(begin, end);
    std::vector<Ball> prefix(n), suffix(n);

    int* ids = sorted.data();
    Ball* pre = prefix.data();
    Ball* suf = suffix.data();

    // Step 1: Try every dimension
    for (int axis = 0; axis < dim; axis++) {

        std::sort(ids, ids + n,
            [tree, axis](int a, int b) {
                if (tree->points[a][axis] !=
                    tree->points[b][axis]) {
                    return tree->points[a][axis] <
                           tree->points[b][axis];
                }
                return a < b;
            }
        );

        // Step 2: Calculate prefix balls
        pre[0] = {tree->points[ids[0]], 0.0};

        for (int i = 1; i < n; i++) {
            Ball p = {tree->points[ids[i]], 0.0};

            mergeBalls(
                &pre[i - 1],
                &p,
                &pre[i]
            );
        }

        // Step 3: Calculate suffix balls
        suf[n - 1] = {tree->points[ids[n - 1]], 0.0};

        for (int i = n - 2; i >= 0; i--) {
            Ball p = {tree->points[ids[i]], 0.0};

            mergeBalls(
                &p,
                &suf[i + 1],
                &suf[i]
            );
        }

        // Step 4: Compare all split positions
        for (int k = 1; k < n; k++) {
            double cost =
                std::pow(pre[k - 1].radius, dim) +
                std::pow(suf[k].radius, dim);

            if (cost < bestCost ||
                (cost == bestCost &&
                 std::abs(n - 2 * k) <
                 std::abs(n - 2 * bestK))) {

                bestCost = cost;
                bestAxis = axis;
                bestK = k;
            }
        }
    }

    // Step 5: Apply best split
    std::sort(begin, end,
        [tree, bestAxis](int a, int b) {
            if (tree->points[a][bestAxis] !=
                tree->points[b][bestAxis]) {
                return tree->points[a][bestAxis] <
                       tree->points[b][bestAxis];
            }
            return a < b;
        }
    );

    return begin + bestK;
}

// Recursive construction
static BallNode* buildRecursive(
    BallTree* tree,
    int* begin,
    int* end
) {
    BallNode* node = newNode(*tree);

    // Step 1: Leaf node
    if (end - begin == 1) {
        int id = *begin;

        node->pointIndex = id;
        node->center = tree->points[id];
        node->radius = 0.0;

        return node;
    }

    // Step 2: Find best split
    int* mid = splitTopDown(tree, begin, end);

    // Step 3: Construct child subtrees
    node->left = buildRecursive(
        tree, begin, mid
    );

    node->right = buildRecursive(
        tree, mid, end
    );

    // Step 4: Merge child balls
    Ball left = {
        node->left->center,
        node->left->radius
    };

    Ball right = {
        node->right->center,
        node->right->radius
    };

    Ball parent;

    mergeBalls(&left, &right, &parent);

    node->center = parent.center;
    node->radius = parent.radius;

    return node;
}

// Entry point
void buildTopDown(
    BallTree& tree,
    const std::vector<Point>& data
) {
    initTree(tree, data);

    std::vector<int> ids(data.size());
    std::iota(ids.begin(), ids.end(), 0);

    int* begin = ids.data();
    int* end = begin + ids.size();

    tree.root = buildRecursive(
        &tree, ids.data(), ids.data() + ids.size()
    );
}
