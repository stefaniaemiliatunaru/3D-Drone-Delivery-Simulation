#include "lab_m1/Tema2/Tema2.h"
#include "lab_m1/Tema2/transform3D.h"
#include "lab_m1/Tema2/Object3D.h"
#include <vector>
#include <string>
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <cmath>

using namespace std;
using namespace m1;

Tema2::Tema2() {
}

Tema2::~Tema2() {
}

// global text variables
int packagesDelivered = 0;
int score = 0;
int droneHealth = 100;
gfxc::TextRenderer* textRenderer = nullptr;
float lastHealthDecreaseTime = -0.1f;

// function to check collisions between all kinds of objects
bool CheckCollision(const glm::vec3& pos1, float scale1, const glm::vec3& pos2, float scale2, bool isHouse, bool isPackage = false, bool isDrone = false, bool isArrow = false) {
	// calculate the distance between the two objects
    float distX = std::abs(pos1.x - pos2.x);
    float distZ = std::abs(pos1.z - pos2.z);
	// collision with the houses, in form of a square
    if (isHouse) {
        float threshold = scale1 * 5.5f;
        return distX < threshold && distZ < threshold;
    }
	// collision with packet or arrow, the drone can pass through them
    if (isPackage || isArrow) {
        float radius1 = scale1 * 0.8f;
        float radius2 = scale2 * 0.8f;
        return distX < (radius1 + radius2) && distZ < (radius1 + radius2);
    }
    if (isDrone) {
        return false;
    }
	// collision with trees, in form of a cylinder
    float distance = std::sqrt(distX * distX + distZ * distZ);
    float combinedRadius = scale1 * 2.7f + scale2 * 2.7f;
    return distance < combinedRadius;
}

// function to generate random positions for the packages without collding with the other obsatcles
glm::vec3 GenerateRandomPackagePosition(const std::vector<glm::vec3>& treePositions, const std::vector<glm::vec3>& housePositions) {
    glm::vec3 packagePos = glm::vec3(0);
    bool validPosition = false;
    float boundaryMin = -25.0f;
    float boundaryMax = 25.0f;
	// generate random positions until a valid one is found
    while (!validPosition) {
        float x = (rand() % 50) - 25.0f;
        float z = (rand() % 50) - 25.0f;
		// check if the package is inside the map boundaries
        if (x < boundaryMin || x > boundaryMax || z < boundaryMin || z > boundaryMax) {
            continue;
        }
        packagePos = glm::vec3(x, 0, z);
        validPosition = true;
        // check collision with the trees 
        for (const auto& treePos : treePositions) {
            if (CheckCollision(packagePos, 1.0f, treePos, 1.0f, false)) {
                validPosition = false;
                break;
            }
        }
        // check collision with the houses
        for (const auto& housePos : housePositions) {
            if (CheckCollision(packagePos, 1.0f, housePos, 1.0f, true)) {
                validPosition = false;
                break;
            }
        }
    }
    return packagePos;
}

void Tema2::Init() {
    polygonMode = GL_FILL;
    camera = new homework::Camera();
    camera->Set(glm::vec3(0, 6.5f, 5), glm::vec3(0, 5, 0), glm::vec3(0, 1, 0));
	cameraMinimap = new homework::Camera();
    cameraMinimap->forward = glm::normalize(glm::vec3(0, 0, 0) - glm::vec3(0, 50, 0));
	cameraMinimap->Set(glm::vec3(0, 50, 0), glm::vec3(0, 0, 0), glm::vec3(0, 0, -1));
    srand((unsigned int)time(nullptr));

    // load shaders
    Shader* shader = new Shader("LabShader");
    shader->AddShader(PATH_JOIN(window->props.selfDir, SOURCE_PATH::M1, "Tema2", "shaders", "VertexShader.glsl"), GL_VERTEX_SHADER);
    shader->AddShader(PATH_JOIN(window->props.selfDir, SOURCE_PATH::M1, "Tema2", "shaders", "FragmentShader.glsl"), GL_FRAGMENT_SHADER);
    shader->CreateAndLink();
    shaders[shader->GetName()] = shader;

    // load meshes for the used objects
    meshes["terrain"] = Object3D::CreateTerrainMesh(55, 55, 1.0f, true);
    meshes["cylinder"] = Object3D::CreateCylinderMesh(20, 2.0f, 0.2f);
    meshes["cone"] = Object3D::CreateConeMesh(20, 1.5f, 0.9f, 2.0f);
    meshes["pyramid"] = Object3D::CreatePyramidMesh(2.0f, 2.6f);
    meshes["box"] = new Mesh("box");
    meshes["box"]->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "box.obj");
    meshes["sphere"] = new Mesh("sphere");
    meshes["sphere"]->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "sphere.obj");

	// initialize variables
    // drone variables
    translateX = 0, translateY = 0, translateZ = 0;
    angularStepOX = 0, angularStepOY = 0, angularStepOZ = 0;
    airscrewRotation = 0.0f;
    projectionMatrix = glm::perspective(RADIANS(60), window->props.aspectRatio, 0.01f, 200.0f);
	projectionMatrixMinimap = glm::ortho(-25.0f, 25.0f, -25.0f, 25.0f, 0.01f, 200.0f);
    dronePosition = glm::vec3(0, 6.5f, 0);
    droneRotation = glm::vec3(0.0f);
    // cloud variables
    minHeight = 100.0f;
	// packet variables
    packageAttached = false;
    attachedPackageOffset = glm::vec3(0.0f, -1.0f, 0.0f);
    attachedPackageIndex = -1;
    packageDelivered = false;

	// generate random positions for the trees that don't collide with other obstacles
    for (int i = 0; i < 20; i++) {
        glm::vec3 treePos;
        bool validPosition = false;
        while (!validPosition) {
            float x = (rand() % 50) - 25.0f;
            float z = (rand() % 50) - 25.0f;
			// check if the tree is inside the map boundaries
            if (x < -25.0f || x > 25.0f || z < -25.0f || z > 25.0f) {
                continue;
            }
            treePos = glm::vec3(x, 0, z);
            validPosition = true;
			// check if the tree collides with any other trees
            for (const auto& existingPos : treePositions) {
                if (CheckCollision(treePos, 1.0f, existingPos, 1.0f, false)) {
                    validPosition = false;
                    break;
                }
            }
			// check if the tree collides with any houses
            for (const auto& housePos : housePositions) {
                if (CheckCollision(treePos, 1.0f, housePos, 1.0f, true)) {
                    validPosition = false;
                    break;
                }
            }
        }
        treePositions.push_back(treePos);
    }


	// generate random positions for the houses that don't collide with other obstacles
    for (int i = 0; i < 12; i++) {
        glm::vec3 housePos;
        bool validPosition = false;
        while (!validPosition) {
            float x = (rand() % 50) - 25.0f;
            float z = (rand() % 50) - 25.0f;
			// check if the house is inside the map boundaries
            if (x < -25.0f || x > 25.0f || z < -25.0f || z > 25.0f) {
                continue;
            }
            housePos = glm::vec3(x, 0, z);
            validPosition = true;
			// check collision with other houses
            for (const auto& existingPos : housePositions) {
                if (CheckCollision(housePos, 1.0f, existingPos, 1.0f, true)) {
                    validPosition = false;
                    break;
                }
            }
			// check collision with trees
            for (const auto& treePos : treePositions) {
                if (CheckCollision(housePos, 1.0f, treePos, 1.0f, false)) {
                    validPosition = false;
                    break;
                }
            }
        }
        housePositions.push_back(housePos);
    }

    
    // generate random spawn positions for the packages
    packageSpawnPosition = GenerateRandomPackagePosition(treePositions, housePositions);
    packagePositions.push_back(packageSpawnPosition);

	// generate random positions for the clouds (a cloud is made out of 5 spheres, so we generate one random position for each cloud and then add other 4 positions relative to the first one)
    cloudPositions.clear();
    for (int i = 0; i < 20; i++) {
        std::vector<glm::vec3> cloud;
        glm::vec3 basePos((rand() % 50) - 25.0f, 10.0f + (rand() % 5), (rand() % 50) - 25.0f);
        cloud.push_back(basePos);
        cloud.push_back(basePos + glm::vec3(2.0f, 0.5f, 1.0f));
        cloud.push_back(basePos + glm::vec3(-2.0f, 0.5f, 1.0f));
        cloud.push_back(basePos + glm::vec3(0.0f, 0.0f, -2.0f));
        cloudPositions.push_back(cloud);
    }

	// generate random positions for the snowflakes
    snowflakes.clear();
    for (int i = 0; i < 200; i++) {
        Snowflake flake;
        flake.position = glm::vec3((rand() % 50) - 25.0f, 10.0f, (rand() % 50) - 25.0f);
        flake.velocity = glm::vec3(0.0f, -1.0f - ((rand() % 50) / 50.0f), 0.0f);
        snowflakes.push_back(flake);
    }

    // generate random positions for the destination
    glm::vec3 destinationPos;
    bool validPosition = false;
    while (!validPosition) {
        float x = (rand() % 50) - 25.0f;
        float z = (rand() % 50) - 25.0f;
		// check if the destination is inside the map boundaries
        if (x < -25.0f || x > 25.0f || z < -25.0f || z > 25.0f) {
            continue;
        }
        destinationPos = glm::vec3(x, 0, z);
        validPosition = true;
		// check collision with trees
        for (const auto& treePos : treePositions) {
            if (CheckCollision(destinationPos, 1.0f, treePos, 1.0f, false)) {
                validPosition = false;
                break;
            }
        }
		// check collision with houses
        for (const auto& housePos : housePositions) {
            if (CheckCollision(destinationPos, 1.0f, housePos, 1.0f, true)) {
                validPosition = false;
                break;
            }
        }
    }
    destinationPositions.push_back(destinationPos);

    treeScales.clear();
    for (int i = 0; i < treePositions.size(); i++) {
        float treeScale = 0.8f + (rand() % 100) / 100.0f;
        treeScales.push_back(treeScale);
    }
    houseScales.clear();
    for (int i = 0; i < housePositions.size(); i++) {
        float houseScale = 0.8f + (rand() % 100) / 100.0f;
        houseScales.push_back(houseScale);
    }

    glm::ivec2 resolution = window->GetResolution();
    textRenderer = new gfxc::TextRenderer(window->props.selfDir, resolution.x, resolution.y);
    textRenderer->Load(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::FONTS, "Hack-Bold.ttf"), 18);
    miniViewportArea = ViewportArea(50, 50, resolution.x / 5.f, resolution.y / 5.f);
}

// function for drawing the text on the screen
void DrawHUD() {
    textRenderer->RenderText("Packets delvered: " + std::to_string(packagesDelivered), 10.f, 10.f, 1.0f, glm::vec3(1, 1, 1));
    textRenderer->RenderText("Score: " + std::to_string(score), 10.f, 35.f, 1.0f, glm::vec3(1, 1, 1));
    if (droneHealth >= 50) {
        textRenderer->RenderText("Drone Health: " + std::to_string(droneHealth), 10.f, 60.f, 1.0f, glm::vec3(1, 1, 1));
	}
	else if (droneHealth < 50 && droneHealth >= 20) {
		textRenderer->RenderText("Drone Health: " + std::to_string(droneHealth), 10.f, 60.f, 1.0f, glm::vec3(0.996f, 0.597f, 0.199f));
	}
	else {
		textRenderer->RenderText("Drone Health: " + std::to_string(droneHealth), 10.f, 60.f, 1.0f, glm::vec3(1, 0, 0));
	}
}

void Tema2::FrameStart() {
    glClearColor(0.472, 0.8, 0.894, 0.996);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, window->props.resolution.x, window->props.resolution.y);
}

void Tema2::RenderScene() {
    // render terrain
    RenderSimpleMesh(meshes["terrain"], shaders["LabShader"], glm::mat4(1), 6);
    // render trees
    for (size_t i = 0; i < treePositions.size(); i++) {
        if (i >= treeScales.size())
            continue;
        // render tree trunk in form of a cylinder
        glm::mat4 modelMatrix = transform3D::Translate(treePositions[i].x, treePositions[i].y, treePositions[i].z);
        float treeScale = treeScales[i];
        glm::mat4 trunkMatrix = modelMatrix * transform3D::Scale(treeScale, treeScale, treeScale);
        RenderSimpleMesh(meshes["cylinder"], shaders["LabShader"], trunkMatrix, 3);
        // render tree leaves in form of a two cones
        glm::mat4 treeLeaves1 = modelMatrix * transform3D::Translate(0.0f, -0.1f, 0.0f) * transform3D::Scale(treeScale, treeScale, treeScale);
        RenderSimpleMesh(meshes["cone"], shaders["LabShader"], treeLeaves1, 4);
        glm::mat4 treeLeaves2 = modelMatrix * transform3D::Translate(0, 0.8f, 0) * transform3D::Scale(treeScale, treeScale, treeScale);
        RenderSimpleMesh(meshes["cone"], shaders["LabShader"], treeLeaves2, 4);
    }

	// render houses
    for (size_t i = 0; i < housePositions.size(); i++) {
        if (i >= houseScales.size())
            continue;
		// render house body in form of a cube
        glm::mat4 modelMatrix = transform3D::Translate(housePositions[i].x, housePositions[i].y, housePositions[i].z);
        float houseScale = houseScales[i];
        glm::mat4 houseMatrix = modelMatrix * transform3D::Scale(houseScale, houseScale, houseScale);
        glm::mat4 bodyMatrix = houseMatrix * transform3D::Scale(3.0f, 5.0f, 3.0f);
        RenderSimpleMesh(meshes["box"], shaders["LabShader"], bodyMatrix, 5);
		// render house roof in form of a pyramid
        glm::mat4 roofMatrix = houseMatrix * transform3D::Translate(0, 2.3f, 0) * transform3D::RotateOZ(glm::radians(5.0f));
        RenderSimpleMesh(meshes["pyramid"], shaders["LabShader"], roofMatrix, 3);
    }

    // render drone
    glm::mat4 droneMatrix = glm::mat4(1);
    droneMatrix *= transform3D::Translate(translateX, 5.0f + translateY, translateZ);

    glm::mat4 droneBaseMatrix1 = droneMatrix * transform3D::RotateOY(glm::radians(45.0f)) * transform3D::RotateOY(angularStepOY);
    droneBaseMatrix1 *= transform3D::Scale(1.2f, 0.2f, 0.2f);
    RenderSimpleMesh(meshes["box"], shaders["LabShader"], droneBaseMatrix1, 2);

    glm::mat4 droneBaseMatrix2 = droneMatrix * transform3D::RotateOY(glm::radians(135.0f)) * transform3D::RotateOY(angularStepOY);
    droneBaseMatrix2 *= transform3D::Scale(1.2f, 0.2f, 0.2f);
    RenderSimpleMesh(meshes["box"], shaders["LabShader"], droneBaseMatrix2, 2);

    glm::mat4 droneBaseMatrix3 = droneMatrix * transform3D::RotateOY(glm::radians(45.0f)) * transform3D::RotateOY(angularStepOY);
    droneBaseMatrix3 *= transform3D::Translate(-0.5f, 0.2f, 0.0f);
    droneBaseMatrix3 *= transform3D::Scale(0.2f, 0.2f, 0.2f);
    RenderSimpleMesh(meshes["box"], shaders["LabShader"], droneBaseMatrix3, 2);

    glm::mat4 droneBaseMatrix4 = droneMatrix * transform3D::RotateOY(glm::radians(45.0f)) * transform3D::RotateOY(angularStepOY);
    droneBaseMatrix4 *= transform3D::Translate(0.5f, 0.2f, 0.0f);
    droneBaseMatrix4 *= transform3D::Scale(0.2f, 0.2f, 0.2f);
    RenderSimpleMesh(meshes["box"], shaders["LabShader"], droneBaseMatrix4, 2);

    glm::mat4 droneBaseMatrix5 = droneMatrix * transform3D::RotateOY(glm::radians(135.0f)) * transform3D::RotateOY(angularStepOY);
    droneBaseMatrix5 *= transform3D::Translate(-0.5f, 0.2f, 0.0f);
    droneBaseMatrix5 *= transform3D::Scale(0.2f, 0.2f, 0.2f);
    RenderSimpleMesh(meshes["box"], shaders["LabShader"], droneBaseMatrix5, 2);

    glm::mat4 droneBaseMatrix6 = droneMatrix * transform3D::RotateOY(glm::radians(135.0f)) * transform3D::RotateOY(angularStepOY);
    droneBaseMatrix6 *= transform3D::Translate(0.5f, 0.2f, 0.0f);
    droneBaseMatrix6 *= transform3D::Scale(0.2f, 0.2f, 0.2f);
    RenderSimpleMesh(meshes["box"], shaders["LabShader"], droneBaseMatrix6, 2);

    glm::mat4 droneAirscrewMatrix1 = droneBaseMatrix3 * transform3D::Translate(0.0f, 0.7f, 0.0f);
    droneAirscrewMatrix1 *= transform3D::RotateOY(glm::radians(135.0));
    droneAirscrewMatrix1 *= transform3D::RotateOY(airscrewRotation);
    droneAirscrewMatrix1 *= transform3D::Scale(1.0f, 0.25f, 0.25f);
    RenderSimpleMesh(meshes["box"], shaders["LabShader"], droneAirscrewMatrix1, 1);

    glm::mat4 droneAirscrewMatrix2 = droneBaseMatrix4 * transform3D::Translate(-0.0f, 0.7f, 0.0f);
    droneAirscrewMatrix2 *= transform3D::RotateOY(glm::radians(135.0));
    droneAirscrewMatrix2 *= transform3D::RotateOY(airscrewRotation);
    droneAirscrewMatrix2 *= transform3D::Scale(1.0f, 0.25f, 0.25f);
    RenderSimpleMesh(meshes["box"], shaders["LabShader"], droneAirscrewMatrix2, 1);

    glm::mat4 droneAirscrewMatrix3 = droneBaseMatrix5 * transform3D::Translate(0.0f, 0.7f, 0.0f);
    droneAirscrewMatrix3 *= transform3D::RotateOY(glm::radians(45.0));
    droneAirscrewMatrix3 *= transform3D::RotateOY(airscrewRotation);
    droneAirscrewMatrix3 *= transform3D::Scale(1.0f, 0.25f, 0.25f);
    RenderSimpleMesh(meshes["box"], shaders["LabShader"], droneAirscrewMatrix3, 1);

    glm::mat4 droneAirscrewMatrix4 = droneBaseMatrix6 * transform3D::Translate(0.0f, 0.7f, 0.0f);
    droneAirscrewMatrix4 *= transform3D::RotateOY(glm::radians(45.0));
    droneAirscrewMatrix4 *= transform3D::RotateOY(airscrewRotation);
    droneAirscrewMatrix4 *= transform3D::Scale(1.0f, 0.25f, 0.25f);
    RenderSimpleMesh(meshes["box"], shaders["LabShader"], droneAirscrewMatrix4, 1);

	// calculate the direction to the package for the arrow indicator to package
    glm::vec3 directionToPackage = glm::normalize(packageSpawnPosition - glm::vec3(translateX, 5.0f + translateY, translateZ));
    float angleToPackage = atan2(directionToPackage.x, directionToPackage.z);

	// render packages
    for (size_t i = 0; i < packagePositions.size(); i++) {
        if (i == attachedPackageIndex && packageAttached) {
            continue;
        }
		// if the package is not attached, render the arrow indicator to the direction of the package
        if (!packageAttached) {
            glm::mat4 arrowBody1 = droneMatrix * transform3D::RotateOY(angleToPackage) * transform3D::RotateOX(glm::radians(90.0f));
            arrowBody1 *= transform3D::Translate(0.0f, 0.0f, -0.5f);
            arrowBody1 *= transform3D::Scale(0.2f, 0.2f, 0.2f);
            RenderSimpleMesh(meshes["cylinder"], shaders["LabShader"], arrowBody1, 1);

            glm::mat4 arrowBody2 = droneMatrix * transform3D::RotateOY(angleToPackage) * transform3D::RotateOX(glm::radians(90.0f));
            arrowBody2 *= transform3D::Translate(0.0f, 0.1f, -0.5f);
            arrowBody2 *= transform3D::Scale(0.1f, 0.1f, 0.1f);
            RenderSimpleMesh(meshes["cone"], shaders["LabShader"], arrowBody2, 1);
        }
		// render the package in a random spot on the map if it has not been picked up by the drone, or it has already been delivered
        glm::mat4 modelMatrix = transform3D::Translate(packagePositions[i].x, packagePositions[i].y + 0.7f, packagePositions[i].z);
        glm::mat4 packageMatrix = modelMatrix * transform3D::Scale(1.0f, 0.3f, 1.0f);
        packageSpawnPosition = packagePositions[i];
        RenderSimpleMesh(meshes["box"], shaders["LabShader"], packageMatrix, 1);
    }

	// if the package has been picked up, render the package attached to the drone
    if (packageAttached && attachedPackageIndex != -1) {
        glm::mat4 attachedPackageMatrix = droneMatrix * transform3D::RotateOY(angularStepOY);
        attachedPackageMatrix *= transform3D::Translate(0.0f, -0.3f, 0.0f);
        attachedPackageMatrix *= transform3D::Scale(1.0f, 0.3f, 1.0f);
        RenderSimpleMesh(meshes["box"], shaders["LabShader"], attachedPackageMatrix, 1);
		// render the arrow indicator to the destination
        for (const auto& destinationPos : destinationPositions) {
            glm::mat4 modelMatrix1 = transform3D::Translate(destinationPos.x, destinationPos.y + 10.0f, destinationPos.z);
            glm::mat4 modelMatrix2 = modelMatrix1 * transform3D::RotateOX(glm::radians(180.0f));
            modelMatrix2 *= transform3D::Scale(2.0f, 2.0f, 2.0f);
            RenderSimpleMesh(meshes["cone"], shaders["LabShader"], modelMatrix2, 7);
            glm::mat4 modelMatrix3 = modelMatrix1 * transform3D::Translate(0.0f, -6.0f, 0.0f);
            modelMatrix3 *= transform3D::Scale(2.0f, 3.0f, 2.0f);
            RenderSimpleMesh(meshes["cylinder"], shaders["LabShader"], modelMatrix3, 7);
        }
    }

    // render clouds
    for (const auto& cloudPos : cloudPositions) {
        for (const auto& spherePos : cloudPos) {
            glm::mat4 modelMatrix = transform3D::Translate(spherePos.x, spherePos.y, spherePos.z);
            modelMatrix *= transform3D::Scale(5.0f, 5.0f, 5.0f);
            RenderSimpleMesh(meshes["sphere"], shaders["LabShader"], modelMatrix, 2);
        }
    }

    // render snowflakes
    for (const auto& snowflakePos : snowflakes) {
        glm::mat4 modelMatrix = glm::mat4(1);
        modelMatrix = glm::translate(modelMatrix, snowflakePos.position);
        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.1f));
        RenderSimpleMesh(meshes["sphere"], shaders["LabShader"], modelMatrix, 2);
    }
}

void Tema2::RenderScene2() {
    // render terrain
    RenderSimpleMesh2(meshes["terrain"], shaders["LabShader"], glm::mat4(1), 6);
    // render trees
    for (size_t i = 0; i < treePositions.size(); i++) {
        if (i >= treeScales.size())
            continue;
        // render tree trunk in form of a cylinder
        glm::mat4 modelMatrix = transform3D::Translate(treePositions[i].x, treePositions[i].y, treePositions[i].z);
        float treeScale = treeScales[i];
        glm::mat4 trunkMatrix = modelMatrix * transform3D::Scale(treeScale, treeScale, treeScale);
        RenderSimpleMesh2(meshes["cylinder"], shaders["LabShader"], trunkMatrix, 3);
        // render tree leaves in form of a two cones
        glm::mat4 treeLeaves1 = modelMatrix * transform3D::Translate(0.0f, -0.1f, 0.0f) * transform3D::Scale(treeScale, treeScale, treeScale);
        RenderSimpleMesh2(meshes["cone"], shaders["LabShader"], treeLeaves1, 4);
        glm::mat4 treeLeaves2 = modelMatrix * transform3D::Translate(0, 0.8f, 0) * transform3D::Scale(treeScale, treeScale, treeScale);
        RenderSimpleMesh2(meshes["cone"], shaders["LabShader"], treeLeaves2, 4);
    }

    // render houses
    for (size_t i = 0; i < housePositions.size(); i++) {
        if (i >= houseScales.size())
            continue;
        // render house body in form of a cube
        glm::mat4 modelMatrix = transform3D::Translate(housePositions[i].x, housePositions[i].y, housePositions[i].z);
        float houseScale = houseScales[i];
        glm::mat4 houseMatrix = modelMatrix * transform3D::Scale(houseScale, houseScale, houseScale);
        glm::mat4 bodyMatrix = houseMatrix * transform3D::Scale(3.0f, 5.0f, 3.0f);
        RenderSimpleMesh2(meshes["box"], shaders["LabShader"], bodyMatrix, 5);
        // render house roof in form of a pyramid
        glm::mat4 roofMatrix = houseMatrix * transform3D::Translate(0, 2.3f, 0) * transform3D::RotateOZ(glm::radians(5.0f));
        RenderSimpleMesh2(meshes["pyramid"], shaders["LabShader"], roofMatrix, 3);
    }

    // render drone
    glm::mat4 droneMatrix = glm::mat4(1);
    droneMatrix *= transform3D::Translate(translateX, 5.0f + translateY, translateZ);

    glm::mat4 droneBaseMatrix1 = droneMatrix * transform3D::RotateOY(glm::radians(45.0f)) * transform3D::RotateOY(angularStepOY);
    droneBaseMatrix1 *= transform3D::Scale(1.2f, 0.2f, 0.2f);
    RenderSimpleMesh2(meshes["box"], shaders["LabShader"], droneBaseMatrix1, 2);

    glm::mat4 droneBaseMatrix2 = droneMatrix * transform3D::RotateOY(glm::radians(135.0f)) * transform3D::RotateOY(angularStepOY);
    droneBaseMatrix2 *= transform3D::Scale(1.2f, 0.2f, 0.2f);
    RenderSimpleMesh2(meshes["box"], shaders["LabShader"], droneBaseMatrix2, 2);

    glm::mat4 droneBaseMatrix3 = droneMatrix * transform3D::RotateOY(glm::radians(45.0f)) * transform3D::RotateOY(angularStepOY);
    droneBaseMatrix3 *= transform3D::Translate(-0.5f, 0.2f, 0.0f);
    droneBaseMatrix3 *= transform3D::Scale(0.2f, 0.2f, 0.2f);
    RenderSimpleMesh2(meshes["box"], shaders["LabShader"], droneBaseMatrix3, 2);

    glm::mat4 droneBaseMatrix4 = droneMatrix * transform3D::RotateOY(glm::radians(45.0f)) * transform3D::RotateOY(angularStepOY);
    droneBaseMatrix4 *= transform3D::Translate(0.5f, 0.2f, 0.0f);
    droneBaseMatrix4 *= transform3D::Scale(0.2f, 0.2f, 0.2f);
    RenderSimpleMesh2(meshes["box"], shaders["LabShader"], droneBaseMatrix4, 2);

    glm::mat4 droneBaseMatrix5 = droneMatrix * transform3D::RotateOY(glm::radians(135.0f)) * transform3D::RotateOY(angularStepOY);
    droneBaseMatrix5 *= transform3D::Translate(-0.5f, 0.2f, 0.0f);
    droneBaseMatrix5 *= transform3D::Scale(0.2f, 0.2f, 0.2f);
    RenderSimpleMesh2(meshes["box"], shaders["LabShader"], droneBaseMatrix5, 2);

    glm::mat4 droneBaseMatrix6 = droneMatrix * transform3D::RotateOY(glm::radians(135.0f)) * transform3D::RotateOY(angularStepOY);
    droneBaseMatrix6 *= transform3D::Translate(0.5f, 0.2f, 0.0f);
    droneBaseMatrix6 *= transform3D::Scale(0.2f, 0.2f, 0.2f);
    RenderSimpleMesh2(meshes["box"], shaders["LabShader"], droneBaseMatrix6, 2);

    glm::mat4 droneAirscrewMatrix1 = droneBaseMatrix3 * transform3D::Translate(0.0f, 0.7f, 0.0f);
    droneAirscrewMatrix1 *= transform3D::RotateOY(glm::radians(135.0));
    droneAirscrewMatrix1 *= transform3D::RotateOY(airscrewRotation);
    droneAirscrewMatrix1 *= transform3D::Scale(1.0f, 0.25f, 0.25f);
    RenderSimpleMesh2(meshes["box"], shaders["LabShader"], droneAirscrewMatrix1, 1);

    glm::mat4 droneAirscrewMatrix2 = droneBaseMatrix4 * transform3D::Translate(-0.0f, 0.7f, 0.0f);
    droneAirscrewMatrix2 *= transform3D::RotateOY(glm::radians(135.0));
    droneAirscrewMatrix2 *= transform3D::RotateOY(airscrewRotation);
    droneAirscrewMatrix2 *= transform3D::Scale(1.0f, 0.25f, 0.25f);
    RenderSimpleMesh2(meshes["box"], shaders["LabShader"], droneAirscrewMatrix2, 1);

    glm::mat4 droneAirscrewMatrix3 = droneBaseMatrix5 * transform3D::Translate(0.0f, 0.7f, 0.0f);
    droneAirscrewMatrix3 *= transform3D::RotateOY(glm::radians(45.0));
    droneAirscrewMatrix3 *= transform3D::RotateOY(airscrewRotation);
    droneAirscrewMatrix3 *= transform3D::Scale(1.0f, 0.25f, 0.25f);
    RenderSimpleMesh2(meshes["box"], shaders["LabShader"], droneAirscrewMatrix3, 1);

    glm::mat4 droneAirscrewMatrix4 = droneBaseMatrix6 * transform3D::Translate(0.0f, 0.7f, 0.0f);
    droneAirscrewMatrix4 *= transform3D::RotateOY(glm::radians(45.0));
    droneAirscrewMatrix4 *= transform3D::RotateOY(airscrewRotation);
    droneAirscrewMatrix4 *= transform3D::Scale(1.0f, 0.25f, 0.25f);
    RenderSimpleMesh2(meshes["box"], shaders["LabShader"], droneAirscrewMatrix4, 1);

    // calculate the direction to the package for the arrow indicator to package
    glm::vec3 directionToPackage = glm::normalize(packageSpawnPosition - glm::vec3(translateX, 5.0f + translateY, translateZ));
    float angleToPackage = atan2(directionToPackage.x, directionToPackage.z);

    // render packages
    for (size_t i = 0; i < packagePositions.size(); i++) {
        if (i == attachedPackageIndex && packageAttached) {
            continue;
        }
        // if the package is not attached, render the arrow indicator to the direction of the package
        if (!packageAttached) {
            glm::mat4 arrowBody1 = droneMatrix * transform3D::RotateOY(angleToPackage) * transform3D::RotateOX(glm::radians(90.0f));
            arrowBody1 *= transform3D::Translate(0.0f, 0.0f, -0.5f);
            arrowBody1 *= transform3D::Scale(0.2f, 0.2f, 0.2f);
            RenderSimpleMesh2(meshes["cylinder"], shaders["LabShader"], arrowBody1, 1);

            glm::mat4 arrowBody2 = droneMatrix * transform3D::RotateOY(angleToPackage) * transform3D::RotateOX(glm::radians(90.0f));
            arrowBody2 *= transform3D::Translate(0.0f, 0.1f, -0.5f);
            arrowBody2 *= transform3D::Scale(0.1f, 0.1f, 0.1f);
            RenderSimpleMesh2(meshes["cone"], shaders["LabShader"], arrowBody2, 1);
        }
        // render the package in a random spot on the map if it has not been picked up by the drone, or it has already been delivered
        glm::mat4 modelMatrix = transform3D::Translate(packagePositions[i].x, packagePositions[i].y + 0.7f, packagePositions[i].z);
        glm::mat4 packageMatrix = modelMatrix * transform3D::Scale(1.0f, 0.3f, 1.0f);
        packageSpawnPosition = packagePositions[i];
        RenderSimpleMesh2(meshes["box"], shaders["LabShader"], packageMatrix, 1);
    }

    // if the package has been picked up, render the package attached to the drone
    if (packageAttached && attachedPackageIndex != -1) {
        glm::mat4 attachedPackageMatrix = droneMatrix * transform3D::RotateOY(angularStepOY);
        attachedPackageMatrix *= transform3D::Translate(0.0f, -0.3f, 0.0f);
        attachedPackageMatrix *= transform3D::Scale(1.0f, 0.3f, 1.0f);
        RenderSimpleMesh2(meshes["box"], shaders["LabShader"], attachedPackageMatrix, 1);
        // render the arrow indicator to the destination
        for (const auto& destinationPos : destinationPositions) {
            glm::mat4 modelMatrix1 = transform3D::Translate(destinationPos.x, destinationPos.y + 10.0f, destinationPos.z);
            glm::mat4 modelMatrix2 = modelMatrix1 * transform3D::RotateOX(glm::radians(180.0f));
            modelMatrix2 *= transform3D::Scale(2.0f, 2.0f, 2.0f);
            RenderSimpleMesh2(meshes["cone"], shaders["LabShader"], modelMatrix2, 7);
            glm::mat4 modelMatrix3 = modelMatrix1 * transform3D::Translate(0.0f, -6.0f, 0.0f);
            modelMatrix3 *= transform3D::Scale(2.0f, 3.0f, 2.0f);
            RenderSimpleMesh2(meshes["cylinder"], shaders["LabShader"], modelMatrix3, 7);
        }
    }
}

// function to check collision between the drone and the obstacles (the houses and trees)
bool CheckCollisionDrone(const glm::vec3& dronePos, const glm::vec3& obstaclePos, float droneScale, float obstacleScale, bool isHouse) {
    float distX = std::abs(dronePos.x - obstaclePos.x);
    float distZ = std::abs(dronePos.z - obstaclePos.z);
    if (!isHouse) {
        // check collision with trees (collision domain in form of a cylinder)
        float radius = obstacleScale * 0.8f;
        float height = obstacleScale * 4.0f;
        float distY = std::abs(dronePos.y - obstaclePos.y);
        float horizontalDistance = std::sqrt(distX * distX + distZ * distZ);
        return horizontalDistance < (droneScale + radius) && distY < (droneScale + height);
    } else {
        // check collision with houses (collision domain in form of a square)
        float halfSide = obstacleScale * 1.4f;
        return distX < (droneScale + halfSide) && distZ < (droneScale + halfSide);
    }
}
// function to update the position of the snowflakes
void Tema2::UpdateSnowflakes(float deltaTimeSeconds) {
    for (auto& snowflakePos : snowflakes) {
        snowflakePos.position += snowflakePos.velocity * deltaTimeSeconds;
        if (snowflakePos.position.y <= 0.0f) {
            snowflakePos.position = glm::vec3((rand() % 50) - 25.0f, 10.0f, (rand() % 50) - 25.0f);
        }
    }
}

void Tema2::Update(float deltaTimeSeconds) {
    airscrewRotation += deltaTimeSeconds * 10.0f;

    glm::vec3 dronePosition = glm::vec3(translateX, 5.0f + translateY, translateZ);

    static float currentTime = 0.0f;
    currentTime += deltaTimeSeconds;

    // check collision between the drone and the packet
    for (size_t i = 0; i < packagePositions.size(); i++) {
        // if the drone collides into the packet collision domain and 'P' key is pressed, the package becomes attached to the drone
        if (!packageAttached && CheckCollision(dronePosition, 0.5f, packagePositions[i], 0.5f, false, true)) {
            if (window->KeyHold(GLFW_KEY_P)) {
                packageAttached = true;
                attachedPackageIndex = i;
                break;
            }
        }
    }

	// check collision between the drone and the destination
    for (size_t i = 0; i < destinationPositions.size(); i++) {
		// if the drone collides into the destination collision domain while the package is attached and 'P' key is pressed, the package becomes delivered
        if (packageAttached && CheckCollision(dronePosition, 0.5f, destinationPositions[i], 1.0f, false)) {
            if (window->KeyHold(GLFW_KEY_P)) {
                packagePositions.erase(packagePositions.begin() + attachedPackageIndex);
                packageAttached = false;
                attachedPackageIndex = -1;
				// a new package is generated once the package is delivered
                glm::vec3 newPackagePosition = GenerateRandomPackagePosition(treePositions, housePositions);
                packagePositions.push_back(newPackagePosition);
				// a new destiantion is generated once the package is delivered
                glm::vec3 newDestinationPosition = GenerateRandomPackagePosition(treePositions, housePositions);
                destinationPositions[i] = newDestinationPosition;
                // update game stats
                packagesDelivered++;
                score += 100;
                break;
            }
        }
    }
    // update package position relative to the drone if it is attached to the drone
    if (packageAttached && attachedPackageIndex != -1) {
        packagePositions[attachedPackageIndex] = dronePosition + glm::vec3(0, -0.3f, 0);
    }

    // check collisions with trees
    bool collided = false;
    for (size_t i = 0; i < treePositions.size(); i++) {
        glm::vec3 treePosition = treePositions[i];
        float treeScale = treeScales[i];
        if (CheckCollisionDrone(dronePosition, treePosition, 0.5f, treeScale, false)) {
            glm::vec3 collisionDirection = glm::normalize(dronePosition - treePosition);
            translateX += collisionDirection.x * deltaTimeSeconds * 3.0f;
            translateZ += collisionDirection.z * deltaTimeSeconds * 3.0f;
            camera->Set(glm::vec3(translateX, translateY, translateZ) + glm::mat3(glm::rotate(glm::mat4(1), angularStepOY, glm::vec3(0, 1, 0)) * glm::rotate(glm::mat4(1), 0.0f, glm::vec3(1, 0, 0)) * glm::rotate(glm::mat4(1), 0.0f, glm::vec3(0, 0, 1))) * glm::vec3(0, 6.5f, 5), glm::vec3(translateX, translateY, translateZ), glm::vec3(0, 1, 0));
			// if the drone is above the tree, move it upwards
            if (translateY > treePosition.y) {
                translateY += collisionDirection.y * deltaTimeSeconds * 3.0f;
                camera->TranslateUpward(collisionDirection.y * deltaTimeSeconds * 3.0f);
            }
            // update collision variable
            collided = true;
            break;
        }
    }

	// check collisions with houses
    for (size_t i = 0; i < housePositions.size(); i++) {
        glm::vec3 housePosition = housePositions[i];
        float houseScale = houseScales[i];
        if (CheckCollisionDrone(dronePosition, housePosition, 0.5f, houseScale, true)) {
            glm::vec3 collisionDirection = glm::normalize(dronePosition - housePosition);
            translateX += collisionDirection.x * deltaTimeSeconds * 3.0f;
            translateZ += collisionDirection.z * deltaTimeSeconds * 3.0f;
            camera->Set(glm::vec3(translateX, translateY, translateZ) + glm::mat3(glm::rotate(glm::mat4(1), angularStepOY, glm::vec3(0, 1, 0)) * glm::rotate(glm::mat4(1), 0.0f, glm::vec3(1, 0, 0)) * glm::rotate(glm::mat4(1), 0.0f, glm::vec3(0, 0, 1))) * glm::vec3(0, 6.5f, 5), glm::vec3(translateX, translateY, translateZ), glm::vec3(0, 1, 0));
			// if the drone is above the house, move it upwards
            if (translateY > housePosition.y) {
                translateY += collisionDirection.y * deltaTimeSeconds * 3.0f;
                camera->TranslateUpward(collisionDirection.y * deltaTimeSeconds * 3.0f);
            }
			// update collision variable
            collided = true;
            break;
        }
    }
	// check collision with the ground
    if (dronePosition.y < 100.0f) {
        dronePosition.y = 100.0f;
    }
	// update drone health if a collision has been detected
    if (collided && currentTime - lastHealthDecreaseTime >= 1.0f) {
        droneHealth -= 10;
        lastHealthDecreaseTime = currentTime;
    }
	// check if the drone health is below 0
    if (droneHealth <= 0) {
        droneHealth = 0;
    }

	// only draw game stats if the drone is still alive
    if (droneHealth > 0) {
        DrawHUD();
    }

	// if drone health is below 0, display game over screen
    if (droneHealth <= 0) {
        glClearColor(0.472f, 0.8f, 0.894f, 0.996f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glm::ivec2 resolution = window->GetResolution();
        // display 'Game over!' text
        textRenderer->RenderText("Game Over!", resolution.x / 2.0f - 90.0f, resolution.y / 2.0f - 30.0f, 2.0f, glm::vec3(1, 1, 1));
        // display 'Replay' text
        textRenderer->RenderText("Press 'R' to restart game.", resolution.x / 2.0f - 240.0f, resolution.y / 2.0f, 2.0f, glm::vec3(1, 1, 1));
        return;
    }
    
    UpdateSnowflakes(deltaTimeSeconds);
    glClearColor(0.472f, 0.8f, 0.894f, 0.996f);
    glm::ivec2 resolution = window->GetResolution();
    glViewport(0, 0, resolution.x, resolution.y);
    RenderScene();
    glClear(GL_DEPTH_BUFFER_BIT);
    glViewport(miniViewportArea.x, miniViewportArea.y, miniViewportArea.width, miniViewportArea.height);
    glm::mat4 droneMatrix = glm::mat4(1);
    droneMatrix *= transform3D::Translate(translateX, 5.0f + translateY, translateZ);

    glm::mat4 droneBaseMatrix1 = droneMatrix * transform3D::RotateOY(glm::radians(45.0f)) * transform3D::RotateOY(angularStepOY);
    droneBaseMatrix1 *= transform3D::Scale(50.2f, 50.2f, 50.2f);
	RenderScene2();
}

void Tema2::RenderSimpleMesh(Mesh* mesh, Shader* shader, const glm::mat4& modelMatrix, int objectType) {
    if (!mesh || !shader || !shader->GetProgramID())
        return;

    glUseProgram(shader->program);

    int modelLoc = glGetUniformLocation(shader->GetProgramID(), "Model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));

    glm::mat4 viewMatrix = camera->GetViewMatrix();
    int viewLoc = glGetUniformLocation(shader->GetProgramID(), "View");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));

    int projectionLoc = glGetUniformLocation(shader->GetProgramID(), "Projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

    glUniform1i(glGetUniformLocation(shader->program, "objectType"), objectType);

    mesh->Render();
}

void Tema2::RenderSimpleMesh2(Mesh* mesh, Shader* shader, const glm::mat4& modelMatrix, int objectType) {
    if (!mesh || !shader || !shader->GetProgramID())
        return;

    glUseProgram(shader->program);

    int modelLoc = glGetUniformLocation(shader->GetProgramID(), "Model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));

    glm::mat4 viewMatrix = cameraMinimap->GetViewMatrix();
    int viewLoc = glGetUniformLocation(shader->GetProgramID(), "View");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));

    int projectionLoc = glGetUniformLocation(shader->GetProgramID(), "Projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrixMinimap));

    glUniform1i(glGetUniformLocation(shader->program, "objectType"), objectType);

    mesh->Render();
}

void Tema2::FrameEnd() {
}

void Tema2::RestartGame() {
    // reset game variables
    packagesDelivered = 0;
    score = 0;
    droneHealth = 100;
    translateX = 0.0f;
    translateY = 0.0f;
    translateZ = 0.0f;
    packageAttached = false;
    attachedPackageIndex = -1;

    // regenerate obstacles positions
    treePositions.clear();
    housePositions.clear();
    destinationPositions.clear();
    packagePositions.clear();
    // call Init function to restart game
    Init();
}

void Tema2::OnInputUpdate(float deltaTime, int mods) {
    // drone movements bounded by terrain boundaries
    float boundaryMin = -26.5f;
    float boundaryMax = 26.5f;
    if (window->KeyHold(GLFW_KEY_W)) {
        float newTranslateX = translateX - deltaTime * droneSpeed * sin(angularStepOY);
        float newTranslateZ = translateZ - deltaTime * droneSpeed * cos(angularStepOY);

        if (newTranslateX >= boundaryMin && newTranslateX <= boundaryMax &&
            newTranslateZ >= boundaryMin && newTranslateZ <= boundaryMax) {
            translateX = newTranslateX;
            translateZ = newTranslateZ;
            camera->TranslateForward(deltaTime * droneSpeed);
        }
    }
    if (window->KeyHold(GLFW_KEY_S)) {
        float newTranslateX = translateX + deltaTime * droneSpeed * sin(angularStepOY);
        float newTranslateZ = translateZ + deltaTime * droneSpeed * cos(angularStepOY);

        if (newTranslateX >= boundaryMin && newTranslateX <= boundaryMax &&
            newTranslateZ >= boundaryMin && newTranslateZ <= boundaryMax) {
            translateX = newTranslateX;
            translateZ = newTranslateZ;
            camera->TranslateForward(-deltaTime * droneSpeed);
        }
    }
    if (window->KeyHold(GLFW_KEY_A)) {
        float newTranslateX = translateX - deltaTime * droneSpeed * cos(angularStepOY);
        float newTranslateZ = translateZ + deltaTime * droneSpeed * sin(angularStepOY);

        if (newTranslateX >= boundaryMin && newTranslateX <= boundaryMax &&
            newTranslateZ >= boundaryMin && newTranslateZ <= boundaryMax) {
            translateX = newTranslateX;
            translateZ = newTranslateZ;
            camera->TranslateRight(-deltaTime * droneSpeed);
        }
    }
    if (window->KeyHold(GLFW_KEY_D)) {
        float newTranslateX = translateX + deltaTime * droneSpeed * cos(angularStepOY);
        float newTranslateZ = translateZ - deltaTime * droneSpeed * sin(angularStepOY);

        if (newTranslateX >= boundaryMin && newTranslateX <= boundaryMax &&
            newTranslateZ >= boundaryMin && newTranslateZ <= boundaryMax) {
            translateX = newTranslateX;
            translateZ = newTranslateZ;
            camera->TranslateRight(deltaTime * droneSpeed);
        }
    }
    if (window->KeyHold(GLFW_KEY_E)) {
        translateY += deltaTime;
        camera->TranslateUpward(deltaTime);
    }
    if (window->KeyHold(GLFW_KEY_Q)) {
        if (translateY > -4.5f) {
            translateY -= deltaTime;
            camera->TranslateUpward(-deltaTime);
        }
    }
    // drone speed-up
    if (window->KeyHold(GLFW_KEY_LEFT_SHIFT)) {
        droneSpeed = 3.5f;
    }
    else {
        droneSpeed = 1.5f;
    }
    if (window->KeyHold(GLFW_KEY_H)) {
        angularStepOY += deltaTime;
        camera->RotateThirdPerson_OY(deltaTime);
    }
    if (window->KeyHold(GLFW_KEY_J)) {
        angularStepOY -= deltaTime;
        camera->RotateThirdPerson_OY(-deltaTime);
    }
}

void Tema2::OnKeyPress(int key, int mods) {
    // game restart feature
    if (key == GLFW_KEY_R) {
        RestartGame();
        return;
    }
	// package pick-up/drop feature
    if (key == GLFW_KEY_P) {
        for (const auto& cylinderPos : destinationPositions) {
            if (packageAttached && CheckCollision(dronePosition, 0.5f, cylinderPos, 1.0f, false)) {
                packageAttached = false;
                attachedPackageIndex = -1;
                return;
            }
        }

        for (size_t i = 0; i < packagePositions.size(); i++) {
            if (!packageAttached && CheckCollision(dronePosition, 0.5f, packagePositions[i], 0.5f, false, true)) {
                packageAttached = true;
                attachedPackageIndex = i;
                return;
            }
        }
    }
}

void Tema2::OnKeyRelease(int key, int mods) {}
void Tema2::OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY) {}
void Tema2::OnMouseBtnPress(int mouseX, int mouseY, int button, int mods) {}
void Tema2::OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods) {}
void Tema2::OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY) {}
void Tema2::OnWindowResize(int width, int height) {}