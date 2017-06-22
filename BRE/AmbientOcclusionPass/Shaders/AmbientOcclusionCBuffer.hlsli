#ifndef AMBIENT_OCCLUSION_CBUFFER_H
#define AMBIENT_OCCLUSION_CBUFFER_H

struct AmbientOcclusionCBuffer {
    float mScreenWidth;
    float mScreenHeight;
    uint mSampleKernelSize;
    float mNoiseTextureDimension;
    float mOcclusionRadius;
    float mSsaoPower;
};

#endif