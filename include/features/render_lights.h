#pragma once

#include <glm/glm.hpp>

// Point light definition: position, color, intensity, falloff
namespace chr {

    struct PointLightDesc {
        glm::vec3 position;
        glm::vec3 color;
        float intensity;
        float range;
    };

}
