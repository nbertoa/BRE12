#include <memory>           
#include <windows.h>

#include <App/App.h>
#include <ShapesApp\ShapesApp.h>

#include <tbb\tbb_thread.h>

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPSTR /*lpCmdLine*/, _In_ int /*nShowCmd*/) {
	App app(hInstance);
	ShapesApp shapesApp;
	shapesApp.Run(app);

	return 0;
}