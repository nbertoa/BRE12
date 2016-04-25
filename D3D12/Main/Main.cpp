#include <memory>           
#include <windows.h>

#include <InitDirect3D\InitDirect3DApp.h>

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPSTR /*lpCmdLine*/, _In_ int /*nShowCmd*/) {
	InitDirect3DApp app(hInstance);
	app.Initialize();
	app.Run();

	return 0;
}