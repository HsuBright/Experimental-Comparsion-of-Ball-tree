
#include "BallTree.h"

#include <algorithm>
#include <limits>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

using Candidate = std::pair<double, int>;
using MaxHeap = std::priority_queue<Candidate>;

static double lowerBound(
    const BallNode* node,
    const Point* query
) {
    return std::max(
        0.0,
        pointDistance(*query, node->center) - node->radius
    );
}

static void searchKNN(
    const BallTree* tree,
    const BallNode* node,
    const Point* query,
    double bound,
    int k,
    MaxHeap* best
) {
    if (node == nullptr)
        return;

    // Prune only when the heap is full
    if ((int)best->size() == k &&
        bound > best->top().first)
        return;

    // Leaf node
    if (node->left == nullptr &&
        node->right == nullptr) {

        int id = node->pointIndex;
        double d = pointDistance(
            *query, tree->points[id]
        );

        Candidate candidate = {d, id};

        if ((int)best->size() < k) {
            best->push(candidate);
        }
        else if (candidate < best->top()) {
            best->pop();
            best->push(candidate);
        }

        return;
    }

    // Calculate child lower bounds
    const BallNode* first = node->left;
    const BallNode* second = node->right;

    double d1 = first
        ? lowerBound(first, query)
        : std::numeric_limits<double>::infinity();

    double d2 = second
        ? lowerBound(second, query)
        : std::numeric_limits<double>::infinity();

    // Search nearer child first
    if (d2 < d1) {
        std::swap(first, second);
        std::swap(d1, d2);
    }

    searchKNN(tree, first, query, d1, k, best);
    searchKNN(tree, second, query, d2, k, best);
}

// Public function: must match BallTree.h
std::vector<int> kNN(
    const BallTree& tree,
    const Point& query,
    int k
) {
    if (k <= 0)
        throw std::invalid_argument("k must be positive");

    if (tree.root == nullptr)
        return {};

    if (query.size() != tree.points[0].size())
        throw std::invalid_argument("Query dimension mismatch");

    k = std::min(k, (int)tree.points.size());

    MaxHeap best;

    searchKNN(
        &tree, tree.root, &query,
        0.0, k, &best
    );

    std::vector<int> result;

    while (!best.empty()) {
        result.push_back(best.top().second);
        best.pop();
    }

    std::reverse(result.begin(), result.end());

    return result;
}
