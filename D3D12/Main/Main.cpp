#include <windows.h>

#include <App/App.h>
#include <BasicScene/BasicScene.h>
#include <TextureScene\TextureScene.h>

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPSTR /*lpCmdLine*/, _In_ int /*nShowCmd*/) {
	BasicScene scene;
	App app(hInstance, &scene);
	app.Run();

	return 0;
}