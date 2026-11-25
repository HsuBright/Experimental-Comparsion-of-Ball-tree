
#include "BallTree.h"

#include <algorithm>
#include <numeric>
#include <vector>

// Construct a subtree for points in [begin, end)
static BallNode* buildRecursive(
    BallTree& tree,
    std::vector<int>& indices,
    int begin,
    int end
) {
    int n = end - begin;

    // Step 1: Create a node
    BallNode* node = newNode(tree);

    // Step 2: Leaf node
    if (n == 1) {
        int id = indices[begin];

        node->pointIndex = id;
        node->center = tree.points[id];
        node->radius = 0.0;

        return node;
    }

    int dim = (int)tree.points[0].size();

    // Step 3: Calculate bounding ball center
    node->center.assign(dim, 0.0);

    for (int i = begin; i < end; i++) {
        const Point& p = tree.points[indices[i]];

        for (int j = 0; j < dim; j++) {
            node->center[j] += p[j];
        }
    }

    for (int j = 0; j < dim; j++) {
        node->center[j] /= n;
    }

    // Calculate bounding ball radius
    node->radius = 0.0;

    for (int i = begin; i < end; i++) {
        double d = pointDistance(
            node->center,
            tree.points[indices[i]]
        );

        node->radius = std::max(node->radius, d);
    }

    // Step 4: Find dimension with largest spread
    int bestAxis = 0;
    double bestSpread = -1.0;

    for (int j = 0; j < dim; j++) {
        double minVal = tree.points[indices[begin]][j];
        double maxVal = minVal;

        for (int i = begin + 1; i < end; i++) {
            double value = tree.points[indices[i]][j];

            minVal = std::min(minVal, value);
            maxVal = std::max(maxVal, value);
        }

        double spread = maxVal - minVal;

        if (spread > bestSpread) {
            bestSpread = spread;
            bestAxis = j;
        }
    }

    // Step 5: Split at median
    int mid = begin + n / 2;

    std::nth_element(
        indices.begin() + begin,
        indices.begin() + mid,
        indices.begin() + end,
        [&](int a, int b) {
            if (tree.points[a][bestAxis] != tree.points[b][bestAxis]) {
                return tree.points[a][bestAxis] < tree.points[b][bestAxis];
            }
            return a < b;
        }
    );

    // Step 6: Construct left subtree
    node->left = buildRecursive(
        tree, indices, begin, mid
    );

    // Step 7: Construct right subtree
    node->right = buildRecursive(
        tree, indices, mid, end
    );

    return node;
}

// Public construction function
void buildMedian(
    BallTree& tree,
    const std::vector<Point>& data
) {
    initTree(tree, data);

    int n = (int)data.size();

    std::vector<int> indices(n);
    std::iota(indices.begin(), indices.end(), 0);

    tree.root = buildRecursive(
        tree, indices, 0, n
    );
}
