#include <vector>
#include "core/engine.h"
#include "utils/gl_utils.h"
#include "lab_m1/Tema2/Tema2.h"

class Object3D {
public:
    static Mesh* CreateCylinderMesh(int slices, float trunkHeight, float trunkRadius);
    static Mesh* CreateTerrainMesh(int m, int n, float cellSize, bool fill);
    static Mesh* CreateConeMesh(int slices, float coneHeight, float coneRadius, float baseHeight);
    static Mesh* CreatePyramidMesh(float baseSize, float height);
};
