#pragma once

#include <glm/glm.hpp>

namespace chr {

    struct PointLightDesc {
        glm::vec3 position;
        glm::vec3 color;
        float intensity;
        float range;
    };

}
