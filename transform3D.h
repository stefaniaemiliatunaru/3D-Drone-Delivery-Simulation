#pragma once

#include "utils/glm_utils.h"


namespace transform3D
{
    // translate matrix
    inline glm::mat4 Translate(float translateX, float translateY, float translateZ)
    {
        return glm::transpose(
            glm::mat4(1, 0, 0, translateX,
                0, 1, 0, translateY,
                0, 0, 1, translateZ,
                0, 0, 0, 1)
        );
    }

    // scale matrix
    inline glm::mat4 Scale(float scaleX, float scaleY, float scaleZ)
    {
        return glm::transpose(
            glm::mat4(scaleX, 0, 0, 0,
                0, scaleY, 0, 0,
                0, 0, scaleZ, 0,
                0, 0, 0, 1)
        );
    }

    // rotate matrix relative to the OZ axis
    inline glm::mat4 RotateOZ(float radians)
    {
        return glm::transpose(
            glm::mat4(cos(radians), -sin(radians), 0, 0,
                sin(radians), cos(radians), 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1)
        );
    }

    // rotate matrix relative to the OY axis
    inline glm::mat4 RotateOY(float radians)
    {
        return glm::transpose(
            glm::mat4(cos(radians), 0, sin(radians), 0,
                0, 1, 0, 0,
                -sin(radians), 0, cos(radians), 0,
                0, 0, 0, 1)
        );

    }

    // rotate matrix relative to the OX axis
    inline glm::mat4 RotateOX(float radians)
    {
        return glm::transpose(
            glm::mat4(1, 0, 0, 0,
                0, cos(radians), -sin(radians), 0,
                0, sin(radians), cos(radians), 0,
                0, 0, 0, 1)
        );

    }
}   // namespace transform3D
