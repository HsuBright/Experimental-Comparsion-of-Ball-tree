#include "BallTree.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

// Minimum possible distance from q to any point inside this ball.
static double lowerBound(const BallNode* node, const Point* query) {
    double d = pointDistance(*query, node->center) - node->radius;
    return std::max(0.0, d);
}

// Search the nearer subtree first, pruning the other when possible.
static void searchRecursive(
    const BallTree* tree,
    const BallNode* node,
    const Point* query,
    double bound,
    int* bestIndex,
    double* bestDistance
) {
    if (node == nullptr || bound > *bestDistance)
        return;

    // Each leaf stores exactly one original data point.
    if (node->left == nullptr && node->right == nullptr) {
        int id = node->pointIndex;
        double d = pointDistance(*query, tree->points[id]);

        if (*bestIndex == -1 || d < *bestDistance ||
            (d == *bestDistance && id < *bestIndex)) {
            *bestDistance = d;
            *bestIndex = id;
        }
        return;
    }

    const BallNode* first = node->left;
    const BallNode* second = node->right;

    double d1 = first ? lowerBound(first, query)
                      : std::numeric_limits<double>::infinity();
    double d2 = second ? lowerBound(second, query)
                       : std::numeric_limits<double>::infinity();

    if (d2 < d1) {
        std::swap(first, second);
        std::swap(d1, d2);
    }

    if (first != nullptr && d1 <= *bestDistance)
        searchRecursive(tree, first, query, d1, bestIndex, bestDistance);

    // bestDistance may have improved after searching the first subtree.
    if (second != nullptr && d2 <= *bestDistance)
        searchRecursive(tree, second, query, d2, bestIndex, bestDistance);
}

// Shared exact nearest-neighbor search for all four construction methods.
int nearestNeighbor(const BallTree& tree, const Point& query) {
    if (tree.root == nullptr)
        return -1;

    if (query.size() != tree.points[0].size())
        throw std::invalid_argument("Query dimension does not match tree");

    int bestIndex = -1;
    double bestDistance = std::numeric_limits<double>::infinity();

    searchRecursive(&tree, tree.root, &query, 0.0,
                    &bestIndex, &bestDistance);

    return bestIndex;
}