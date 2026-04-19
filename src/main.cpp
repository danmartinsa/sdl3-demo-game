#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <vector>
#include <array>
#include <string>
#include <format>
#include <math.h>

#include "gameobject.h"

using namespace std;

struct SDLState
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    int width, height, logW, logH;
    const bool *keys;

    SDLState() : keys(SDL_GetKeyboardState(nullptr)) {}
};

const size_t LAYER_IDX_LEVEL = 0;
const size_t LAYER_IDX_CHARACTERS = 1;
const int MAP_ROWS = 5;
const int MAP_COLS = 50;
const int TILE_SIZE = 64;
const int SPRITE_SHEET_CHAR_LEFT = 193;

struct GameState
{
    std::array<std::vector<GameObject>, 2> layers;
    std::vector<GameObject> backgroundTiles;
    std::vector<GameObject> foregroundTiles;

    int playerIndex;
    SDL_FRect mapViewPort;
    float bg2Scroll, bg3Scroll, bg4Scroll;

    GameState(const SDLState &state)
    {
        playerIndex = -1;
        mapViewPort = SDL_FRect{
            .x = 0,
            .y = 0,
            .w = static_cast<float>(state.logW),
            .h = static_cast<float>(state.logH)};
        bg2Scroll = bg3Scroll = bg4Scroll = 0;
    }

    GameObject &player() { return layers[LAYER_IDX_CHARACTERS][playerIndex]; }
};

struct Resources
{
    const int ANIM_PLAYER_IDLE = 0;
    const int ANIM_PLAYER_RUN = 1;
    const int ANIM_PLAYER_SLIDE = 2;
    std::vector<Animation> playerAnims;

    std::vector<SDL_Texture *> textures;
    SDL_Texture *texIdle, *texRun, *texSlide,
        *texBrick, *texGrass, *texGround, *texPanel,
        *texBg1, *texBg2, *texBg3, *texBg4;

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
        playerAnims[ANIM_PLAYER_SLIDE] = Animation(3, 1.0f);

        // player animations
        texIdle = loadTexture(state.renderer, "assets/standard/idle.png");
        texRun = loadTexture(state.renderer, "assets/standard/run.png");
        texRun = loadTexture(state.renderer, "assets/standard/run.png");
        texSlide = loadTexture(state.renderer, "assets/standard/emote.png");

        // tile textures
        texBrick = loadTexture(state.renderer, "assets/tiles/quartzite tile.png");
        texGrass = loadTexture(state.renderer, "assets/tiles/grass.png");
        texGround = loadTexture(state.renderer, "assets/tiles/basalt.png");
        texPanel = loadTexture(state.renderer, "assets/tiles/wood tile.png");

        // bg textuires
        texBg1 = loadTexture(state.renderer, "assets/background/city 1/1.png");
        texBg2 = loadTexture(state.renderer, "assets/background/city 1/4.png");
        texBg3 = loadTexture(state.renderer, "assets/background/city 1/3.png");
        texBg4 = loadTexture(state.renderer, "assets/background/city 1/2.png");
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
void createTiles(const SDLState &state, GameState &gs, const Resources &res);
void checkCollision(const SDLState &state, GameState &gs, Resources &res, GameObject &objA, GameObject &objB, float deltaTime);
void handleKeyInput(const SDLState &state, GameState &gs, GameObject &obj, SDL_Scancode key, bool keyDown);
void drawParalaxBackground(SDL_Renderer *renderer, SDL_Texture *texture, float xVelocity, float &scrollPos, float scrollFactor, float deltaTime);

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
    GameState gs(state);
    createTiles(state, gs, res);
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
            case SDL_EVENT_KEY_DOWN:
            {
                handleKeyInput(state, gs, gs.player(), event.key.scancode, true);
                break;
            }
            case SDL_EVENT_KEY_UP:
            {
                handleKeyInput(state, gs, gs.player(), event.key.scancode, false);
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
        // calculate viewport position
        gs.mapViewPort.x = (gs.player().position.x + TILE_SIZE / 2) - gs.mapViewPort.w / 2;

        // draw background

        // Perform drawing commands
        SDL_SetRenderDrawColor(state.renderer, 20, 10, 30, 255);
        SDL_RenderClear(state.renderer);

        // draw background image
        SDL_RenderTexture(state.renderer, res.texBg1, nullptr, nullptr);
        drawParalaxBackground(state.renderer, res.texBg4, gs.player().velocity.x, gs.bg4Scroll, 0.075f, deltaTime);
        drawParalaxBackground(state.renderer, res.texBg3, gs.player().velocity.x, gs.bg3Scroll, 0.15f, deltaTime);
        drawParalaxBackground(state.renderer, res.texBg2, gs.player().velocity.x, gs.bg2Scroll, 0.3f, deltaTime);

        for (GameObject &obj : gs.backgroundTiles)
        {
            SDL_FRect dst{
                .x = obj.position.x -gs.mapViewPort.x,
                .y = obj.position.y,
                .w = static_cast<float>(obj.texture->w),
                .h = static_cast<float>(obj.texture->h)};

            SDL_RenderTexture(state.renderer, obj.texture, nullptr, &dst);
        }

        // draw all objects
        for (auto &layer : gs.layers)
        {
            for (GameObject &obj : layer)
            {
                drawObject(state, gs, obj, deltaTime);
            }
        }

        for (GameObject &obj : gs.foregroundTiles)
        {
            SDL_FRect dst{
                .x = obj.position.x - gs.mapViewPort.x,
                .y = obj.position.y,
                .w = static_cast<float>(obj.texture->w),
                .h = static_cast<float>(obj.texture->h)};

            SDL_RenderTexture(state.renderer, obj.texture, nullptr, &dst);
        }

        // display debug info
        SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255);
        // SDL_RenderDebugText(state.renderer, 5, 5,
        //                     std::format("State: {}", static_cast<int>(gs.player().data.player.state)).c_str());
        // SDL_RenderDebugText(state.renderer, 5, 5,
        //                     std::format("State: {}", static_cast<int>(gs.player().velocity.x)).c_str());

        int pos = 5;
        for (auto &layer : gs.layers)
        {
            for (GameObject &obj : layer)
            {
                if (obj.type == ObjectType::player)
                {
                    SDL_RenderDebugText(state.renderer, 5, pos,
                                        std::format("State: {}", static_cast<int>(obj.data.player.state)).c_str());
                    SDL_RenderDebugText(state.renderer, 5, pos + 10,
                                        std::format("velocity: {}", obj.velocity.x).c_str());
                    SDL_RenderDebugText(state.renderer, 5, pos + 20,
                                        std::format("acceleration: {}", obj.acceleration.x).c_str());
                    SDL_RenderDebugText(state.renderer, 5, pos + 30,
                                        std::format("velocity: {}", obj.velocity.y).c_str());
                    SDL_RenderDebugText(state.renderer, 5, pos + 40,
                                        std::format("acceleration: {}", obj.acceleration.y).c_str());
                    SDL_RenderDebugText(state.renderer, 5, pos + 50,
                                        std::format("grounded: {}", obj.grounded).c_str());
                    pos += 55;
                }
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
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error",
                                 "Error Initializing SDL3", nullptr);
        initSuccess = false;
    }

    state.window = SDL_CreateWindow("SDL3 Demo", state.width, state.height,
                                    SDL_WINDOW_RESIZABLE);
    if (!state.window)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error",
                                 "Error creating window", nullptr);
        cleanup(state);
        initSuccess = false;
    }

    // create the renderer
    state.renderer = SDL_CreateRenderer(state.window, nullptr);
    if (!state.renderer)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error",
                                 "Error creating Renderer", state.window);
        initSuccess = false;
    }

    SDL_SetRenderVSync(state.renderer, 1);

    // configure presentation
    SDL_SetRenderLogicalPresentation(state.renderer, state.logW, state.logH,
                                     SDL_LOGICAL_PRESENTATION_LETTERBOX);

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
    float srcX = obj.currentAnimation != -1
                     ? obj.animations[obj.currentAnimation].currentFrame() * spriteSize
                     : 0.0f;

    const float TILE_REF_POS = obj.type == ObjectType::player
                                   ? static_cast<float>(SPRITE_SHEET_CHAR_LEFT)
                                   : 0.0f;

    SDL_FRect src{.x = srcX,
                  .y = TILE_REF_POS,
                  .w = spriteSize,
                  .h = spriteSize};

    SDL_FRect dst{.x = obj.position.x - gs.mapViewPort.x,
                  .y = obj.position.y,
                  .w = spriteSize,
                  .h = spriteSize};

    SDL_FlipMode flipMode = obj.direction == -1 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    SDL_RenderTextureRotated(state.renderer, obj.texture, &src, &dst, 0, nullptr, flipMode);
}

void update(const SDLState &state, GameState &gs, Resources &res,
            GameObject &obj, float deltaTime)
{
    if (obj.dynamic)
    {
        obj.velocity += glm::vec2(0, 500) * deltaTime;
    }
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
            }
            else
            {
                // decelerate
                if (obj.velocity.x)
                {
                    const float factor = obj.velocity.x > 0 ? -2.0f : 2.0f;
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
            obj.texture = res.texIdle;
            obj.currentAnimation = res.ANIM_PLAYER_IDLE;
            break;
        }
        case PlayerState::running:
        {
            // switch ideli state
            if (!currentDirection)
            {
                obj.data.player.state = PlayerState::idle;
            }
            if (obj.velocity.x * obj.direction < 0 && obj.grounded)
            {
                obj.texture = res.texSlide;
                obj.currentAnimation = res.ANIM_PLAYER_SLIDE;
            }
            else
            {
                obj.texture = res.texRun;
                obj.currentAnimation = res.ANIM_PLAYER_RUN;
            }
            break;
        }
        case PlayerState::jumping:
        {
            obj.texture = res.texRun;
            obj.currentAnimation = res.ANIM_PLAYER_RUN;
            break;
        }
        }
        // add acceleration to velocity
        obj.velocity += currentDirection * obj.acceleration * deltaTime;
        if (std::abs(obj.velocity.x) > obj.maxSpeedX)
        {
            obj.velocity.x = currentDirection * obj.maxSpeedX;
        }
    }

    // add velocity to position
    obj.position += obj.velocity * deltaTime;

    // handle collision detection
    bool foundGround = false;
    for (auto &layer : gs.layers)
    {
        for (GameObject &objB : layer)
        {
            if (&obj != &objB)
            {
                checkCollision(state, gs, res, obj, objB, deltaTime);

                // ground sensor
                SDL_FRect sensor{
                    .x = obj.position.x + obj.collider.x,
                    .y = obj.position.y + obj.collider.y + obj.collider.h,
                    .w = obj.collider.w,
                    .h = 1};
                SDL_FRect rectB{
                    .x = objB.position.x + objB.collider.x,
                    .y = objB.position.y + objB.collider.y,
                    .w = objB.collider.w,
                    .h = objB.collider.h};
                SDL_FRect rectC{0};
                if (SDL_GetRectIntersectionFloat(&sensor, &rectB, &rectC))
                {
                    if (rectC.w > rectC.h)
                    {
                        foundGround = true;
                    }
                }
            }
        }
    }
    if (obj.grounded != foundGround)
    {
        // Switch grounded state
        obj.grounded = foundGround;
        if (foundGround && obj.type == ObjectType::player)
        {
            obj.data.player.state = PlayerState::running;
        }
    }
}

void collisionResponse(const SDLState &state,
                       GameState &gs,
                       Resources &res,
                       const SDL_FRect &rectA,
                       const SDL_FRect &rectB,
                       const SDL_FRect &rectC,
                       GameObject &objA,
                       GameObject &objB,
                       float deltaTime)
{

    if (objA.type == ObjectType::player)
    {
        switch (objB.type)
        {
        case ObjectType::level:
        {
            if (rectC.w < rectC.h)
            {
                // horizontal collision
                if (objA.velocity.x > 0) // going right
                {
                    objA.position.x -= rectC.w;
                }
                else if (objA.velocity.x < 0) // going lefet
                {
                    objA.position.x += rectC.w;
                }
                objA.velocity.x = 0;
            }
            else
            {
                // vertical collision
                if (objA.velocity.y > 0) // going down
                {
                    objA.position.y -= rectC.h;
                }
                else if (objA.velocity.y < 0) // going up
                {
                    objA.position.y += rectC.h;
                }
                objA.velocity.y = 0;
            }
            break;
        }
        }
    }
}

void checkCollision(const SDLState &state,
                    GameState &gs,
                    Resources &res,
                    GameObject &objA,
                    GameObject &objB,
                    float deltaTime)
{
    SDL_FRect rectA{
        .x = objA.position.x + objA.collider.x,
        .y = objA.position.y + objA.collider.y,
        .w = objA.collider.w,
        .h = objA.collider.h};

    SDL_FRect rectB{
        .x = objB.position.x + objB.collider.x,
        .y = objB.position.y + objB.collider.x,
        .w = objB.collider.w,
        .h = objB.collider.h};

    SDL_FRect rectC{0};

    if (SDL_GetRectIntersectionFloat(&rectA, &rectB, &rectC))
    {
        // found an intersection
        collisionResponse(state, gs, res, rectA, rectB, rectC, objA, objB, deltaTime);
    }
}

void createTiles(const SDLState &state, GameState &gs, const Resources &res)
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
    // clang-format off
    short map[MAP_ROWS][MAP_COLS] = {
        0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0
    };
    short foreground[MAP_ROWS][MAP_COLS] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        5, 0, 5, 5, 5, 5, 0, 0, 5, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };
    short background[MAP_ROWS][MAP_COLS] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 6, 0, 6, 0, 0, 6, 0, 0, 0, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 6, 0, 6, 0, 0, 6, 0, 0, 0, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };
    // clang-format on

    const auto loadMap = [&state, &gs, &res](short layer[MAP_ROWS][MAP_COLS])
    {
        const auto createObject = [&state](int r, int c, SDL_Texture *tex, ObjectType type)
        {
            GameObject o;
            o.type = type;
            o.position = glm::vec2(c * TILE_SIZE, state.logH - (MAP_ROWS - r) * TILE_SIZE);
            o.texture = tex;
            o.collider = {.x = 0, .y = 0, .w = TILE_SIZE, .h = TILE_SIZE};
            return o;
        };
        for (int r = 0; r < MAP_ROWS; r++)
        {
            for (int c = 0; c < MAP_COLS; c++)
            {
                switch (layer[r][c])
                {
                case 1: // ground
                {
                    GameObject o = createObject(r, c, res.texGround, ObjectType::level);
                    gs.layers[LAYER_IDX_LEVEL].push_back(o);
                    break;
                }
                case 2: // panel
                {
                    GameObject o = createObject(r, c, res.texPanel, ObjectType::level);
                    gs.layers[LAYER_IDX_LEVEL].push_back(o);
                    break;
                }
                case 4: // player
                {
                    GameObject player = createObject(r, c, res.texIdle, ObjectType::player);
                    player.data.player = PlayerData();
                    player.animations = res.playerAnims;
                    player.currentAnimation = res.ANIM_PLAYER_IDLE;
                    player.acceleration = glm::vec2(400, 0);
                    player.maxSpeedX = 200;
                    player.dynamic = true;
                    player.collider = {
                        .x = 22,
                        .y = 12,
                        .w = 20,
                        .h = 50};
                    gs.layers[LAYER_IDX_CHARACTERS].push_back(player);
                    gs.playerIndex = gs.layers[LAYER_IDX_CHARACTERS].size() - 1;
                    break;
                }
                case 5: // grass
                {
                    GameObject o = createObject(r, c, res.texGrass, ObjectType::level);
                    gs.foregroundTiles.push_back(o);
                    break;
                }
                case 6: // brick
                {
                    GameObject o = createObject(r, c, res.texBrick, ObjectType::level);
                    gs.backgroundTiles.push_back(o);
                    break;
                }
                }
            }
        }
    };
    loadMap(map);
    loadMap(background);
    loadMap(foreground);
    assert(gs.playerIndex != -1);
}

void handleKeyInput(const SDLState &state, GameState &gs, GameObject &obj, SDL_Scancode key, bool keyDown)
{
    const float JUMP_FORCE = -400.0f;
    if (obj.type == ObjectType::player)
    {
        switch (obj.data.player.state)
        {
        case PlayerState::idle:
        {
            if (key == SDL_SCANCODE_W && keyDown)
            {
                obj.data.player.state = PlayerState::jumping;
                obj.velocity.y += JUMP_FORCE;
            }
            break;
        }
        case PlayerState::running:
        {
            if (key == SDL_SCANCODE_W && keyDown)
            {
                obj.data.player.state = PlayerState::jumping;
                obj.velocity.y += JUMP_FORCE;
            }
            break;
        }
        }
    }
}

void drawParalaxBackground(SDL_Renderer *renderer, SDL_Texture *texture, float xVelocity, float &scrollPos, float scrollFactor, float deltaTime)
{
    scrollPos -= xVelocity * scrollFactor * deltaTime;
    if (scrollPos <= -texture->w)
    {
        scrollPos = 0;
    }

    SDL_FRect dst{
        .x = scrollPos,
        .y = 5,
        .w = texture->w * 3.0f,
        .h = static_cast<float>(texture->h)};

    SDL_RenderTextureTiled(renderer, texture, nullptr, 1, &dst);
}