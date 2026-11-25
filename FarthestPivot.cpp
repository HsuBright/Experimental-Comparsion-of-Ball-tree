
#include "BallTree.h"

#include <algorithm>
#include <numeric>
#include <vector>

// Split points using two distant pivots
static int* splitFarthestPivot(
    BallTree* tree,
    BallNode* node,
    int* begin,
    int* end
) {
    int n = end - begin;

    // Step 1: Find first pivot
    int pivot1 = *begin;
    double maxDist1 = -1.0;

    for (int* p = begin; p < end; p++) {
        double d = squaredDistance(
            tree->points[*p],
            node->center
        );

        if (d > maxDist1) {
            maxDist1 = d;
            pivot1 = *p;
        }
    }

    // Step 2: Find second pivot
    int pivot2 = *begin;
    double maxDist2 = -1.0;

    for (int* p = begin; p < end; p++) {
        double d = squaredDistance(
            tree->points[*p],
            tree->points[pivot1]
        );

        if (d > maxDist2) {
            maxDist2 = d;
            pivot2 = *p;
        }
    }

    // Step 3: Handle identical points
    if (maxDist2 == 0.0)
        return begin + n / 2;

    // Step 4: Partition by nearest pivot
    int* mid = std::partition(
        begin, end,
        [&](int id) {
            double d1 = squaredDistance(
                tree->points[id],
                tree->points[pivot1]
            );

            double d2 = squaredDistance(
                tree->points[id],
                tree->points[pivot2]
            );

            return d1 <= d2;
        }
    );

    // Avoid empty subtrees
    if (mid == begin || mid == end)
        mid = begin + n / 2;

    return mid;
}

// Construct a subtree for points in [begin, end)
static BallNode* buildRecursive(
    BallTree* tree,
    int* begin,
    int* end
) {
    int n = end - begin;

    // Step 1: Create node
    BallNode* node = newNode(*tree);

    // Step 2: Leaf node
    if (n == 1) {
        int id = *begin;

        node->pointIndex = id;
        node->center = tree->points[id];
        node->radius = 0.0;

        return node;
    }

    int dim = (int)tree->points[0].size();

    // Step 3: Calculate bounding ball center
    node->center.assign(dim, 0.0);

    for (int* p = begin; p < end; p++) {
        for (int j = 0; j < dim; j++) {
            node->center[j] += tree->points[*p][j];
        }
    }

    for (int j = 0; j < dim; j++) {
        node->center[j] /= n;
    }

    // Calculate bounding ball radius
    node->radius = 0.0;

    for (int* p = begin; p < end; p++) {
        double d = pointDistance(
            node->center,
            tree->points[*p]
        );

        node->radius = std::max(node->radius, d);
    }

    // Step 4 & 5: Split by farthest pivots
    int* mid = splitFarthestPivot(
        tree, node, begin, end
    );

    // Step 6: Construct left subtree
    node->left = buildRecursive(
        tree, begin, mid
    );

    // Step 7: Construct right subtree
    node->right = buildRecursive(
        tree, mid, end
    );

    return node;
}

// Public construction function
void buildFarthestPivot(
    BallTree& tree,
    const std::vector<Point>& data
) {
    initTree(tree, data);

    std::vector<int> indices(data.size());
    std::iota(indices.begin(), indices.end(), 0);

    int* begin = indices.data();
    int* end = begin + indices.size();

    tree.root = buildRecursive(
        &tree, begin, end
    );
}
