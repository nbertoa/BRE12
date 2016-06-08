#pragma once

#include <DirectXMath.h>
#include <memory>
#include <tbb/concurrent_queue.h>

#include <App/App.h>
#include <ResourceManager\UploadBuffer.h>
#include <ShapesApp/ShapeInitTask.h>
#include <ShapesApp/ShapeTask.h>

#if defined(DEBUG) || defined(_DEBUG)                                                                                                                                                            
#define _CRTDBG_MAP_ALLOC          
#include <cstdlib>             
#include <crtdbg.h>               
#endif 

class ShapesApp : public App {
public:
	explicit ShapesApp(HINSTANCE hInstance);
	ShapesApp(const ShapesApp& rhs) = delete;
	ShapesApp& operator=(const ShapesApp& rhs) = delete;

	void Initialize() noexcept override;
	
protected:
	struct Vertex {
		Vertex() {}
		Vertex(const DirectX::XMFLOAT4& p) 
			: mPosition(p)
		{}

		DirectX::XMFLOAT4 mPosition{ 0.0f, 0.0f, 0.0f, 0.0f };
	};
		
	std::vector<CmdBuilderTaskInput> mCmdBuilderTaskInputs;
	std::vector<ShapeTask*> mShapeTasks;

	tbb::concurrent_queue<ID3D12CommandList*> mCmdLists;

	void Update(const float dt) noexcept override;
	void Draw(const float dt) noexcept override;
};

