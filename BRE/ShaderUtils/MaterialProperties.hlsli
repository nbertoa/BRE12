#ifndef MATERIALPROPERTIES_H
#define MATERIALPROPERTIES_H

struct MaterialProperties {
    // We use 4 floats: metalness + smoothnes + padding
    float4 mMetalnessSmoothness;
};

#endif 