#include "Object3D.h"
#include <vector>
#include <math.h>

// function to create terrain mesh
Mesh* Object3D::CreateTerrainMesh(int m, int n, float cellSize, bool fill) {
    std::vector<VertexFormat> vertices;
    std::vector<unsigned int> indices;
    float width = (m - 1) * cellSize;
    float height = (n - 1) * cellSize;
    float xOffset = -width / 2.0f;
    float zOffset = -height / 2.0f;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            float x = j * cellSize + xOffset;
            float z = i * cellSize + zOffset;
            glm::vec3 position = glm::vec3(x, 0.0f, z);
            glm::vec3 normal = glm::vec3(0, 1, 0);
            glm::vec2 texture = glm::vec2(0.0f);
            vertices.emplace_back(position, glm::vec3(0), normal, texture);
        }
    }

    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < m - 1; j++) {
            int current = i * m + j;
            int next = current + m;
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);
            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }

    Mesh* terrain = new Mesh("terrain");
    terrain->InitFromData(vertices, indices);
    if (!fill) {
        terrain->SetDrawMode(GL_LINE_LOOP);
    }
    return terrain;
}

// function to create cylinde rmesh
Mesh* Object3D::CreateCylinderMesh(int slices, float trunkHeight, float trunkRadius) {
    std::vector<VertexFormat> vertices;
    std::vector<unsigned int> indices;
    for (int i = 0; i < slices; i++) {
        float angle = glm::radians(360.0f * i / slices);
        float x = trunkRadius * cos(angle);
        float z = trunkRadius * sin(angle);
        vertices.emplace_back(glm::vec3(x, 0, z), glm::vec3(0), glm::vec3(0, -1, 0), glm::vec2(0));
        vertices.emplace_back(glm::vec3(x, trunkHeight, z), glm::vec3(0), glm::vec3(0, 1, 0), glm::vec2(0));
    }

    for (int i = 0; i < slices; i++) {
        int next = (i + 1) % slices;
        indices.push_back(i * 2);
        indices.push_back(i * 2 + 1);
        indices.push_back(next * 2);
        indices.push_back(i * 2 + 1);
        indices.push_back(next * 2 + 1);
        indices.push_back(next * 2);
    }

    Mesh* trunkMesh = new Mesh("cylinder");
    trunkMesh->InitFromData(vertices, indices);
    return trunkMesh;
}

// function to create cone mesh
Mesh* Object3D::CreateConeMesh(int slices, float coneHeight, float coneRadius, float baseHeight) {
    std::vector<VertexFormat> vertices;
    std::vector<unsigned int> indices;
    int baseIndex = vertices.size();
    vertices.emplace_back(glm::vec3(0, baseHeight, 0), glm::vec3(0), glm::vec3(0, -1, 0), glm::vec2(0));
    for (int i = 0; i < slices; i++) {
        float angle = glm::radians(360.0f * i / slices);
        float x = coneRadius * cos(angle);
        float z = coneRadius * sin(angle);
        vertices.emplace_back(glm::vec3(x, baseHeight, z), glm::vec3(0), glm::vec3(x, coneHeight, z), glm::vec2(0));
        if (i > 0) {
            indices.push_back(baseIndex);
            indices.push_back(baseIndex + i);
            indices.push_back(baseIndex + i + 1);
        }
    }

    int tipIndex = vertices.size();
    vertices.emplace_back(glm::vec3(0, baseHeight + coneHeight, 0), glm::vec3(0), glm::vec3(0, 1, 0), glm::vec2(0));
    for (int i = 1; i <= slices; i++) {
        indices.push_back(tipIndex);
        indices.push_back(baseIndex + i);
        indices.push_back(baseIndex + ((i % slices) + 1));
    }

    Mesh* coneMesh = new Mesh("cone");
    coneMesh->InitFromData(vertices, indices);
    return coneMesh;
}

// function to create pyramid mesh
Mesh* Object3D::CreatePyramidMesh(float baseSize, float height) {
    std::vector<VertexFormat> vertices;
    std::vector<unsigned int> indices;
    vertices.emplace_back(glm::vec3(-baseSize, 0, -baseSize), glm::vec3(0), glm::vec3(0, -1, 0), glm::vec2(0));
    vertices.emplace_back(glm::vec3(baseSize, 0, -baseSize), glm::vec3(0), glm::vec3(0, -1, 0), glm::vec2(1, 0));
    vertices.emplace_back(glm::vec3(baseSize, 0, baseSize), glm::vec3(0), glm::vec3(0, -1, 0), glm::vec2(1, 1));
    vertices.emplace_back(glm::vec3(-baseSize, 0, baseSize), glm::vec3(0), glm::vec3(0, -1, 0), glm::vec2(0, 1));
    vertices.emplace_back(glm::vec3(0, height, 0), glm::vec3(0), glm::vec3(0, 1, 0), glm::vec2(0.5f, 0.5f));
    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(2);
    indices.push_back(2);
    indices.push_back(3);
    indices.push_back(0);
    for (int i = 0; i < 4; i++) {
        indices.push_back(i);
        indices.push_back((i + 1) % 4);
        indices.push_back(4);
    }

    Mesh* pyramidMesh = new Mesh("pyramidMesh");
    pyramidMesh->InitFromData(vertices, indices);
    return pyramidMesh;
}
