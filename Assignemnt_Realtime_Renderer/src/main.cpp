#include "Assignemnt_Realtime_Renderer.h"

#define DEFAULT_SCREENWIDTH 1920
#define DEFAULT_SCREENHEIGHT 1080
// main that controls the creation/destruction of an application
int WinMain(int argc, char* argv[])
{
	// explicitly control the creation of our application
	Assignemnt_Realtime_Renderer* app = new Assignemnt_Realtime_Renderer();
	app->run("Assignemnt_Realtime_Renderer Project", DEFAULT_SCREENWIDTH, DEFAULT_SCREENHEIGHT, false);

	// explicitly control the destruction of our application
	delete app;

	return 0;
}