#include <memory>           
#include <windows.h>

#include <App/App.h>
#include <BasicTechApp\BasicTechScene.h>

#include <tbb\tbb_thread.h>

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPSTR /*lpCmdLine*/, _In_ int /*nShowCmd*/) {
	BasicTechScene scene;
	App app(hInstance, &scene);
	app.Run();

	return 0;
}