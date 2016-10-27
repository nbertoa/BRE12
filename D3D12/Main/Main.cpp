#include <windows.h>

#include <App/App.h>
#include <ExampleScenes/BasicScene.h>
#include <ExampleScenes\ColorHeightScene.h>
#include <ExampleScenes\ColorNormalScene.h>
#include <ExampleScenes\Demo1Scene.h>
#include <ExampleScenes\HeightScene.h>
#include <ExampleScenes\NormalScene.h>
#include <ExampleScenes\SkyBoxScene.h>
#include <ExampleScenes\TextureScene.h>

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPSTR /*lpCmdLine*/, _In_ int /*nShowCmd*/) {
	Demo1Scene scene;
	App app(hInstance, &scene);

	return 0;
}