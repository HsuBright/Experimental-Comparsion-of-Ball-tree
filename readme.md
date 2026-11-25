# Experimental Comparsion of Ball tree

## Overview

This project implements and experimentally runs and compares algorithms from *Proximity Search in the Greedy Tree* (Chubet et al., 2023) . Four tree construction methods are implemented: Median Split, TopDown, Farthest Pivot, and Greedy Tree. The main purpose is to compare the greedy tree with three other construction. 

## Tree Construction

| Method | Description | Reference |
|---|---|---|
| Median Split |Recursively partitions points at the median of their coordinates along the dimension, with centroid-based bounding balls.| [1] |
| TopDown | Partition minimizing the combined estimated volume of two bounding balls. | [1] |
| Farthest Pivot | Distance-based partition in two distant pivots, with centroid-based bounding balls. | [2] |
| Greedy Tree | Construction based on an approximate greedy permutation and lazy predecessor updates. | [3] |

## Search Algorithms

The generic search algorithms use depth-first branch-and-bound traversal with distance. The Greedy-specific algorithms use radius-first traversal.

| Algorithm | Description | Reference |
|---|---|---|
| NN | Exact nearest neighbor search with pruning. | [1] |
| kNN | Exact k-nearest neighbor search using a bounded max-heap. | [1] |
| ANN | NN with approximation pruning. | [1, 4] |
| AkNN | kNN with approximation pruning. | [1, 4] |
| Greedy ANN | Largest-radius-first search. | [3] |
| Greedy AkNN | Extension of Greedy ANN in a bounded candidate max-heap. | Adapted from [3, 4] |

Greedy AkNN is a simplified in one-heap rather than a direct implementation of the algorithm in Section 7 of [3].

## Experiments

- **Tree construction:** Compare all four trees using the same NN, kNN, ANN, and AkNN algorithms.
- **Search:** Compare generic ANN and AkNN with their Greedy-specific counterparts on the same Greedy Tree.

Experiments use uniform, clustered datasets to campare construction time, query time, tree height, search statistics, approximation ratios, etc.

## AI Disclosure

The author used ChatGPT 6.0 to varying degrees throughout the development of this project. The Median Split and Farthest Pivot constructions, as well as the NN and kNN algorithms, were implemented independently without AI assistance. ChatGPT was consulted during the implementation and refinement of TopDown, Greedy Tree, ANN, AkNN, and the Greedy Tree-specific search strategies.

The C++ benchmark and Python experiments were developed with substantial AI assistance. (Sorry, I have no idea about how to write the code) ChatGPT was also used for debugging.

The implementation follows a traditional C++ style, primarily using C-style constructs and STL, closing to the author's programming background.

The author assumes full responsibility for the code, experimental results, and conclusions of this project.

## References

[1] S. M. Omohundro. [Five Balltree Construction Algorithms](https://steveomohundro.com/wp-content/uploads/2009/03/omohundro89_five_balltree_construction_algorithms.pdf). ICSI Technical Report TR-89-063, 1989.

[2] A. W. Moore. [The Anchors Hierarchy: Using the Triangle Inequality to Survive High Dimensional Data](https://publications.ri.cmu.edu/storage/publications/pub_files/pub3/moore_andrew_2000_1/moore_andrew_2000_1.pdf). UAI, 2000.

[3] O. Chubet, P. Parikh, D. R. Sheehy, and S. Sheth. [Proximity Search in the Greedy Tree](https://donsheehy.net/research/chubet23proximity.pdf). SOSA, 2023.

[4] S. Arya, D. M. Mount, N. S. Netanyahu, R. Silverman, and A. Y. Wu. [An Optimal Algorithm for Approximate Nearest Neighbor Searching in Fixed Dimensions](https://doi.org/10.1145/293347.293348). Journal of the ACM, 1998.
