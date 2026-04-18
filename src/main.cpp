#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <vector>
#include <string>
#include <array>
#include <math.h>

#include "gameobject.h"

using namespace std;

struct SDLState
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    int width, height, logW, logH;
    const bool *keys;

    SDLState() : keys(SDL_GetKeyboardState(nullptr))
    {
    }
};

const size_t LAYER_IDX_LEVEL = 0;
const size_t LAYER_IDX_CHARACTERS = 1;
const int MAP_ROWS = 5;
const int MAP_COLS = 50;
const int TILE_SIZE = 64;


struct GameState
{
    std::array<std::vector<GameObject>, 2> layers;
    int playerIndex;

    GameState()
    {
        playerIndex = 0; // WILL CHANGE this when load maps
    }
};

struct Resources
{
    const int ANIM_PLAYER_IDLE = 0;
    const int ANIM_PLAYER_RUN = 1;
    std::vector<Animation> playerAnims;

    std::vector<SDL_Texture *> textures;
    SDL_Texture *texIdle, *texRun;

    SDL_Texture *loadTexture(SDL_Renderer *renderer, const std::string &filepath)
    {
        SDL_Texture *tex = IMG_LoadTexture(renderer, filepath.c_str());
        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
        textures.push_back(tex);
        return tex;
    }

    void load(SDLState &state)
    {
        playerAnims.resize(5);
        playerAnims[ANIM_PLAYER_IDLE] = Animation(2, 1.6f);
        playerAnims[ANIM_PLAYER_RUN] = Animation(8, 0.5f);

        texIdle = loadTexture(state.renderer, "assets/standard/idle.png");
        texRun = loadTexture(state.renderer, "assets/standard/run.png");
    }

    void unload()
    {
        for (SDL_Texture *tex : textures)
        {
            SDL_DestroyTexture(tex);
        }
    }
};

bool initialize(SDLState &state);
void cleanup(SDLState &state);
void drawObject(const SDLState &state, GameState &gs, GameObject &obj, float deltaTime);
void update(const SDLState &state, GameState &gs, Resources &res, GameObject &obj, float deltaTime);

int main(int argc, char *argv[])
{
    SDLState state;
    state.width = 1600;
    state.height = 900;
    state.logW = 640;
    state.logH = 320;

    if (!initialize(state))
    {
        return 1;
    }
    // load game assets
    Resources res;
    res.load(state);

    // setup game data
    GameState gs;
    GameObject player;
    player.type = ObjectType::player;
    player.data.player = PlayerData();
    player.texture = res.texIdle;
    player.animations = res.playerAnims;
    player.currentAnimation = res.ANIM_PLAYER_IDLE;
    player.acceleration = glm::vec2(300, 0);
    player.maxSpeedX = 100;
    gs.layers[LAYER_IDX_CHARACTERS].push_back(player);

    uint64_t prevTime = SDL_GetTicks();

    // start game loop
    bool running = true;
    while (running)
    {
        uint64_t nowTime = SDL_GetTicks();
        float deltaTime = (nowTime - prevTime) / 1000.0f; // convert to seconds

        SDL_Event event{0};
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
            {
                running = false;
                break;
            }
            case SDL_EVENT_WINDOW_RESIZED:
            {
                state.width = event.window.data1;
                state.height = event.window.data2;
                break;
            }
            }
        }

        // update all objects
        for (auto &layer : gs.layers)
        {
            for (GameObject &obj : layer)
            {
                update(state, gs, res, obj, deltaTime);
                // update the adnimation
                if (obj.currentAnimation != -1)
                {
                    obj.animations[obj.currentAnimation].step(deltaTime);
                }
            }
        }

        // Perform drawing commands
        SDL_SetRenderDrawColor(state.renderer, 20, 10, 30, 255);
        SDL_RenderClear(state.renderer);

        for (auto &layer : gs.layers)
        {
            for (GameObject &obj : layer)
            {
                drawObject(state, gs, obj, deltaTime);
            }
        }

        SDL_RenderPresent(state.renderer);

        prevTime = nowTime;
    }
    res.unload();
    cleanup(state);
    return 0;
}

bool initialize(SDLState &state)
{
    bool initSuccess = true;

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error Initializing SDL3", nullptr);
        initSuccess = false;
    }

    state.window = SDL_CreateWindow("SDL3 Demo", state.width, state.height, SDL_WINDOW_RESIZABLE);
    if (!state.window)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating window", nullptr);
        cleanup(state);
        initSuccess = false;
    }

    // create the renderer
    state.renderer = SDL_CreateRenderer(state.window, nullptr);
    if (!state.renderer)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating Renderer", state.window);
        initSuccess = false;
    }

    // configure presentation
    SDL_SetRenderLogicalPresentation(state.renderer, state.logW, state.logH, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    return initSuccess;
}

void cleanup(SDLState &state)
{
    SDL_DestroyRenderer(state.renderer);
    SDL_DestroyWindow(state.window);
    SDL_Quit();
}

void drawObject(const SDLState &state, GameState &gs, GameObject &obj, float deltaTime)
{
    const float spriteSize = 64;
    float srcX = obj.animations[obj.currentAnimation].currentFrame() * spriteSize;

    SDL_FRect src{
        .x = srcX,
        .y = 192,
        .w = spriteSize,
        .h = spriteSize};

    SDL_FRect dst{
        .x = obj.position.x,
        .y = obj.position.y,
        .w = spriteSize,
        .h = spriteSize};

    SDL_FlipMode flipMode = obj.direction == -1 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    SDL_RenderTextureRotated(state.renderer, obj.texture, &src, &dst, 0, nullptr, flipMode);
}

void update(const SDLState &state, GameState &gs, Resources &res, GameObject &obj, float deltaTime)
{
    if (obj.type == ObjectType::player)
    {
        float currentDirection = 0;
        if (state.keys[SDL_SCANCODE_A])
        {
            currentDirection += -1;
        }
        if (state.keys[SDL_SCANCODE_D])
        {
            currentDirection += 1;
        }

        if (currentDirection)
        {
            obj.direction = currentDirection;
        }

        switch (obj.data.player.state)
        {
        case PlayerState::idle:
        {
            if (currentDirection)
            {
                obj.data.player.state = PlayerState::running;
                obj.texture = res.texRun;
                obj.currentAnimation = res.ANIM_PLAYER_RUN;
            }
            else
            {
                // decelerate
                if (obj.velocity.x)
                {
                    const float factor = obj.velocity.x > 0 ? -1.5f : 1.5f;
                    float amount = factor * obj.acceleration.x * deltaTime;
                    if (std::abs(obj.velocity.x) < std::abs(amount))
                    {
                        obj.velocity.x = 0;
                    }
                    else
                    {
                        obj.velocity.x += amount;
                    }
                }
            }
            break;
        }
        case PlayerState::running:
        {
            if (!currentDirection)
            {
                obj.data.player.state = PlayerState::idle;
                obj.texture = res.texIdle;
                obj.currentAnimation = res.ANIM_PLAYER_IDLE;
            }
            break;
        }
        }
        // add acceleration to velocity
        obj.velocity += currentDirection * obj.acceleration * deltaTime;
        if (std::abs(obj.velocity.x) > obj.maxSpeedX)
        {
            obj.velocity.x = currentDirection * obj.maxSpeedX;
        }

        // add velocity to position
        obj.position += obj.velocity * deltaTime;
    }
}

void createTile(const SDLState &state, GameState &gs, const Resources &res)
{
    /*
        0 - Empty
        1 - Ground
        2 - Panel
        3 - Enemy
        4 - Player
        5 - Grass
        6 - Brick
    */
    short map[MAP_ROWS][MAP_COLS] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };

    // for (int )
}