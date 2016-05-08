#pragma once

#include "App/D3dApp.h"

#if defined(DEBUG) || defined(_DEBUG)                                                                                                                                                            
#define _CRTDBG_MAP_ALLOC          
#include <cstdlib>             
#include <crtdbg.h>               
#endif 

// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

class InitDirect3DApp : public D3DApp {
public:
	InitDirect3DApp(HINSTANCE hInstance);
	InitDirect3DApp(const InitDirect3DApp& rhs) = delete;
	InitDirect3DApp& operator=(const InitDirect3DApp& rhs) = delete;
	
protected:
	void Update(const Timer& timer) noexcept override;
	void Draw(const Timer& timer) noexcept override;
};

