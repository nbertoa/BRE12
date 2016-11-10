#pragma once

#include <GeometryPass/GeometryPassCmdListRecorder.h>

struct Material;

// Recorder that does color mapping
class ColorCmdListRecorder : public GeometryPassCmdListRecorder {
public:
	explicit ColorCmdListRecorder(ID3D12Device& device);

	~ColorCmdListRecorder() = default;
	ColorCmdListRecorder(const ColorCmdListRecorder&) = delete;
	const ColorCmdListRecorder& operator=(const ColorCmdListRecorder&) = delete;
	ColorCmdListRecorder(ColorCmdListRecorder&&) = default;
	ColorCmdListRecorder& operator=(ColorCmdListRecorder&&) = default;

	// This method is to initialize PSO that is a shared between all this kind
	// of recorders.
	// This method is initialized by its corresponding pass.
	static void InitPSO(const DXGI_FORMAT* geometryBufferFormats, const std::uint32_t geometryBufferCount) noexcept;

	// This method must be called before calling RecordAndPushCommandLists()
	void Init(
		const GeometryData* geometryDataVec,
		const std::uint32_t numGeomData,
		const Material* materials,
		const std::uint32_t numMaterials) noexcept;

	void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept final override;

private:
	void BuildBuffers(const Material* materials, const std::uint32_t numMaterials) noexcept;
};