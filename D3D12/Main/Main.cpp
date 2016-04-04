#include <memory>           
#include <windows.h>
#if defined(DEBUG) || defined(_DEBUG)                                                                                                                                                            
#define _CRTDBG_MAP_ALLOC          
#include <cstdlib>             
#include <crtdbg.h>               
#endif    

int WINAPI WinMain(HINSTANCE /*hInstance*/, HINSTANCE, LPSTR, int /*nCmdShow*/) {
	return 0;
}