#ifndef OCTREE_H
#define OCTREE_H

#include "star_data.h" // We need access to your Star struct

// A 3D Vector for Physics Calculations
typedef struct {
    double x, y, z;
} Vec3;

// The Octree Node (The "Cube")
typedef struct OctreeNode {
    // 1. Spatial Boundaries of this specific cube
    Vec3 min_bounds; 
    Vec3 max_bounds;
    
    // 2. Physics Data for this cube
    double total_mass;
    Vec3 center_of_mass;
    
    // 3. Data Storage
    // If this is a "Leaf" node, it holds a star. If it's a "Branch", id = -1.
    int star_id; 
    
    // 4. The Recursive Pointers (The 8 sub-cubes)
    // Front: Top-Left, Top-Right, Bottom-Left, Bottom-Right
    // Back:  Top-Left, Top-Right, Bottom-Left, Bottom-Right
    struct OctreeNode* children[8]; 
    
} OctreeNode;

#endif
