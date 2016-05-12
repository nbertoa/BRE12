#pragma once

#include "App/App.h"

#if defined(DEBUG) || defined(_DEBUG)                                                                                                                                                            
#define _CRTDBG_MAP_ALLOC          
#include <cstdlib>             
#include <crtdbg.h>               
#endif 

class InitDirect3DApp : public App {
public:
	InitDirect3DApp(HINSTANCE hInstance);
	InitDirect3DApp(const InitDirect3DApp& rhs) = delete;
	InitDirect3DApp& operator=(const InitDirect3DApp& rhs) = delete;
	
protected:
	void Update(const float dt) noexcept override;
	void Draw(const float dt) noexcept override;
};

