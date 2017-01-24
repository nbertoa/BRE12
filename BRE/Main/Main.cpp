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

int WINAPI WinMain(_In_ HINSTANCE moduleInstanceHandle, 
				   _In_opt_ HINSTANCE /*previousModuleInstanceHandle*/, 
				   _In_ LPSTR /*commandLine*/, 
	               _In_ int /*showCommand*/) {
	AmbientOcclussionScene scene;
	App app(moduleInstanceHandle, &scene);

	return 0;
}