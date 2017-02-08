#pragma once

#include <LightingPass/LightingPassCmdListRecorder.h>

class PunctualLightCmdListRecorder : public LightingPassCmdListRecorder {
public:
	PunctualLightCmdListRecorder() = default;
	~PunctualLightCmdListRecorder() = default;
	PunctualLightCmdListRecorder(const PunctualLightCmdListRecorder&) = delete;
	const PunctualLightCmdListRecorder& operator=(const PunctualLightCmdListRecorder&) = delete;
	PunctualLightCmdListRecorder(PunctualLightCmdListRecorder&&) = default;
	PunctualLightCmdListRecorder& operator=(PunctualLightCmdListRecorder&&) = default;
	
	static void InitSharedPSOAndRootSignature() noexcept;

	// Preconditions:
	// - "geometryBuffers" must not be nullptr
	// - "geometryBuffersCount" must be greater than zero
	// - "lights" must not be nullptr
	// - "numLights" must be greater than zero
	// - This method must be called once
	// - InitSharedPSOAndRootSignature() must be called first and once
	void Init(
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
		const std::uint32_t geometryBuffersCount,
		ID3D12Resource& depthBuffer,
		const void* lights,
		const std::uint32_t numLights) noexcept final override;

	// Preconditions:
	// - Init() must be called first
	void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept final override;

	bool IsDataValid() const noexcept override;

private:
	void InitConstantBuffers() noexcept;

	// Preconditions:
	// - "lights" must not be nullptr
	void CreateLightBuffersAndViews(const void* lights) noexcept;

	// Preconditions:
	// - "geometryBuffers" must not be nullptr
	// - "geometryBuffersCount" must be greater than zero
	void InitShaderResourceViews(
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
		const std::uint32_t geometryBuffersCount,
		ID3D12Resource& depthBuffer) noexcept;

	D3D12_GPU_DESCRIPTOR_HANDLE mStartPixelShaderResourceView{ 0UL };
};