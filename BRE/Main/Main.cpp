#include <windows.h>

#include <ExampleScenes\AmbientOcclussionScene.h>
#include <ExampleScenes\ColorHeightScene.h>
#include <ExampleScenes/ColorMappingScene.h>
#include <ExampleScenes\ColorNormalScene.h>
#include <ExampleScenes\HeightScene.h>
#include <ExampleScenes\NormalScene.h>
#include <ExampleScenes\MaterialShowcaseScene.h>
#include <ExampleScenes\TextureScene.h>
#include <SceneExecutor/SceneExecutor.h>

#include <SceneLoader\SceneLoader.h>

int WINAPI WinMain(_In_ HINSTANCE moduleInstanceHandle, 
				   _In_opt_ HINSTANCE /*previousModuleInstanceHandle*/, 
				   _In_ LPSTR /*commandLine*/, 
	               _In_ int /*showCommand*/) {

	SceneExecutor sceneExecutor(moduleInstanceHandle, new ColorMappingScene());
	
	//SceneLoader sceneLoader;
	//sceneLoader.LoadScene("resources/scenes/test.yml");
	
	sceneExecutor.Execute();

	return 0;
}