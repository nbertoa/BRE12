#pragma once

#include <memory>
#include <vector>

#include <GeometryPass/GeometryPassCmdListRecorder.h>

class Scene {
public:
    using GeometryPassCommandListRecorders = std::vector<std::unique_ptr<GeometryPassCmdListRecorder>>;

    Scene() = default;
    Scene(const Scene&) = delete;
    const Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) = delete;
    Scene& operator=(Scene&&) = delete;

    GeometryPassCommandListRecorders& GetGeometryPassCommandListRecorders() noexcept;

    ID3D12Resource* &GetSkyBoxCubeMap() noexcept;
    ID3D12Resource* &GetDiffuseIrradianceCubeMap() noexcept;
    ID3D12Resource* &GetSpecularPreConvolvedCubeMap() noexcept;

private:
    GeometryPassCommandListRecorders mGeometryCommandListRecorders;

    ID3D12Resource* mSkyBoxCubeMap{ nullptr };
    ID3D12Resource* mDiffuseIrradianceCubeMap{ nullptr };
    ID3D12Resource* mSpecularPreConvolvedCubeMap{ nullptr };
};
