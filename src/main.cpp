#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

using namespace std;

int main(int argc, char* argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error Initializing SDL3", nullptr);
        return 1;
    }

    int width = 800;
    int height = 600;

    SDL_Window *window = SDL_CreateWindow("SDL3 Demo", width, height, 0);

    SDL_Quit();
    return 0;

}
