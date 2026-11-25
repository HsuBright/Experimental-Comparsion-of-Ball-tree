#include "BallTree.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <limits>
#include <vector>

// Quadratic scan with alpha-lazy predecessor updates.
static void greedyPermutation(
    const std::vector<Point>& points,
    std::vector<int>& order,
    std::vector<int>& pred,
    double alpha
) {
    if (!std::isfinite(alpha) || alpha < 1.0)
        throw std::invalid_argument("alpha must be finite and >= 1");

    int n = (int)points.size();
    std::vector<double> assigned2(n, std::numeric_limits<double>::infinity());
    std::vector<char> selected(n, 0);
    const double invAlpha = 1.0 / alpha;
    const double invAlpha2 = invAlpha * invAlpha;
    int current = 0;

    for (int k = 0; k < n; ++k) {
        order[k] = current;
        selected[current] = 1;
        int next = -1;

        for (int i = 0; i < n; ++i) {
            if (selected[i]) continue;
            const double d2 = squaredDistance(points[i], points[current]);

            // Reassign only if the distance improves by a factor alpha.
            if (pred[i] == -1 || d2 < assigned2[i] * invAlpha2) {
                assigned2[i] = d2;
                pred[i] = current;
            }

            // Farthest according to assigned predecessor distances.
            if (next == -1 || assigned2[i] > assigned2[next])
                next = i;
        }
        current = next;
    }
}

// Create a leaf in the preallocated node pool
static int addLeaf(
    BallTree& tree,
    int pointId
    
) {
    int index = tree.used;
    BallNode* node = newNode(tree);
    node->centerIndex = pointId;
    node->center = tree.points[pointId];
    node->radius = 0.0;
    node->pointIndex = pointId;
    node->left = nullptr;
    node->right = nullptr;

    return index;
}

// Step 2: Construct the Greedy Tree
void buildGreedy(
    BallTree& tree,
    const std::vector<Point>& data,
    double alpha
) {
    initTree(tree, data);

    int n = (int)tree.points.size();

    std::vector<int> order(n);
    std::vector<int> pred(n, -1);

    greedyPermutation(tree.points, order, pred, alpha);

    // Track the current leaf of each point
    std::vector<int> leafOf(n, -1);

    // Parent index of each tree node
    std::vector<int> parent(tree.nodes.size(), -1);

    // The first selected point is the root
    int rootIndex = addLeaf(tree, order[0]);
    leafOf[order[0]] = rootIndex;

    // Insert remaining points
    for (int k = 1; k < n; k++) {
        int p = order[k];
        int a = pred[p];

        // Find the leaf corresponding to predecessor a
        int v = leafOf[a];

        // Create two leaves: old point a and new point p
        int left = addLeaf(tree, a);
        int right = addLeaf(tree, p);

        // Convert the old leaf into an internal node
        BallNode* node = &tree.nodes[v];

        node->pointIndex = -1;
        node->left = &tree.nodes[left];
        node->right = &tree.nodes[right];

        // Update parent information
        parent[left] = v;
        parent[right] = v;

        // Update leaf locations
        leafOf[a] = left;
        leafOf[p] = right;

        // Step 3: Update bounding balls
        for (int u = v; u != -1; u = parent[u]) {
            BallNode* ancestor = &tree.nodes[u];

            double d = pointDistance(
                ancestor->center,
                tree.points[p]
            );

            ancestor->radius =
                std::max(ancestor->radius, d);
        }
    }

    tree.root = &tree.nodes[rootIndex];
}

// Preserve the existing benchmark interface.
void buildGreedy(BallTree& tree, const std::vector<Point>& data) {
    buildGreedy(tree, data, 1.5);
}