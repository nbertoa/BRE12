#ifndef HEIGHT_MAPPING_CBUFFER
#define HEIGHT_MAPPING_CBUFFER

// Height mapping constant buffer
struct HeightMappingCBuffer {
    float mMinTessellationDistance;
    float mMaxTessellationDistance;
    float mMinTessellationFactor;
    float mMaxTessellationFactor;
    float mHeightScale;
};

#endif