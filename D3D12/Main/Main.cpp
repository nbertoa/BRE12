#include <windows.h>

#include <App/App.h>
#include <BasicScene/BasicScene.h>
#include <Demo1Scene\Demo1Scene.h>
#include <HeightScene\HeightScene.h>
#include <NormalScene\NormalScene.h>
#include <SkyBoxScene\SkyBoxScene.h>
#include <TextureScene\TextureScene.h>

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPSTR /*lpCmdLine*/, _In_ int /*nShowCmd*/) {
	Demo1Scene scene;
	App app(hInstance, &scene);

	return 0;
}