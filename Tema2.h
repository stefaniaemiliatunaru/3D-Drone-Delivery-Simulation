#pragma once
#include "components/simple_scene.h"
#include "lab_m1/Tema2/homework_camera.h"
#include <vector>
#include <glm/glm.hpp>
#include "lab_m1/Tema2/basic_text.h"

namespace m1
{
    class Tema2 : public gfxc::SimpleScene
    {
    public:
        struct Snowflake {
            glm::vec3 position;
            glm::vec3 velocity;
        };
        struct ViewportArea
        {
            ViewportArea() : x(0), y(0), width(1), height(1) {}
            ViewportArea(int x, int y, int width, int height)
                : x(x), y(y), width(width), height(height) {}
            int x;
            int y;
            int width;
            int height;
        };

        Tema2();
        ~Tema2();

        void Init() override;

    private:
        void FrameStart() override;
        void Update(float deltaTimeSeconds) override;
        void FrameEnd() override;
        void RenderSimpleMesh(Mesh* mesh, Shader* shader, const glm::mat4& modelMatrix, int isAirscrew);
        void RenderSimpleMesh2(Mesh* mesh, Shader* shader, const glm::mat4& modelMatrix, int objectType);
        void OnInputUpdate(float deltaTime, int mods) override;
        void OnKeyPress(int key, int mods) override;
        void OnKeyRelease(int key, int mods) override;
        void OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY) override;
        void OnMouseBtnPress(int mouseX, int mouseY, int button, int mods) override;
        void OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods) override;
        void OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY) override;
        void OnWindowResize(int width, int height) override;
        void RenderScene();
        void RenderScene2();
        void UpdateSnowflakes(float deltaTimeSeconds);
        void RestartGame();

    protected:
        // terrain variables
        Mesh* terrainMesh;
        std::vector<glm::vec3> treePositions;
        std::vector<glm::vec3> housePositions;
        std::vector<float> treeScales;
        std::vector<float> houseScales;
        // Drone variables
        float translateX, translateY, translateZ;
        float scaleX, scaleY, scaleZ;
        float angularStepOX, angularStepOY, angularStepOZ;
        float airscrewRotation;
        GLenum polygonMode;
        glm::vec3 dronePosition;
        glm::vec3 droneRotation;
        float droneSpeed;
        // cloud variables
        std::vector<Snowflake> snowflakes;
        std::vector<std::vector<glm::vec3>> cloudPositions;
		// camera variables
		homework::Camera* camera;
		glm::mat4 projectionMatrix;
        homework::Camera* cameraMinimap;
        glm::mat4 projectionMatrixMinimap;
        // terrain colision variables
        float minHeight;
		// Packet variables
        std::vector<glm::vec3> packagePositions;
        std::vector<float> packageScales;
        glm::vec3 packageSpawnPosition;
        bool packageAttached;
        glm::vec3 attachedPackageOffset;
        size_t attachedPackageIndex;
		bool packageDelivered;
        std::vector<glm::vec3> destinationPositions;
		// minimap variables
        ViewportArea miniViewportArea;
    };
}   // namespace m1

