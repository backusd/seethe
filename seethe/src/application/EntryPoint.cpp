#include "pch.h"
#include "Application.h"
#include "utils/Log.h"

using seethe::Application;

#if defined(DEBUG)
int main(int argc, char** argv)
#elif defined(RELEASE)
int APIENTRY WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nShowCmd)
#else
#error Need to define the entry point
#endif
{
	try
	{
		std::unique_ptr<Application> app = std::make_unique<Application>();
		app->Initialize();
		return app->Run();
	}
	catch (std::runtime_error& e)
	{
		LOG_ERROR("Caught runtime error: {}", e.what());
		return 1;
	}
	catch (std::exception& e)
	{
		LOG_ERROR("Caught exception: {}", e.what());
		return 2;
	}
}