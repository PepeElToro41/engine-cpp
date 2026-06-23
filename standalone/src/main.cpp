#include <cstdio>

#include <platform/platform.hpp>

#include <application/application.hpp>

int main() {
    DISPLAY_WINDOW::sdl_initialize();
    
	
	
    Application application{"Engine C++"};
    application.initialize();
    application.run();

    DISPLAY_WINDOW::sdl_shutdown();
    return 0;
}