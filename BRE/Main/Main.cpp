#include <windows.h>

#include <App/App.h>
#include <ExampleScenes\AmbientOcclussionScene.h>
#include <ExampleScenes\ColorHeightScene.h>
#include <ExampleScenes/ColorMappingScene.h>
#include <ExampleScenes\ColorNormalScene.h>
#include <ExampleScenes\HeightScene.h>
#include <ExampleScenes\NormalScene.h>
#include <ExampleScenes\MaterialShowcaseScene.h>
#include <ExampleScenes\TextureScene.h>

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPSTR /*lpCmdLine*/, _In_ int /*nShowCmd*/) {
	MaterialShowcaseScene scene;
	App app(hInstance, &scene);

	return 0;
}