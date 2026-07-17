#include <SDL3/SDL_timer.h>
#include <cstdio>
#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_keycode.h>

#include "types.h"
#include "renderer.cc"

static double framerate = 60.0;
static u32 ticks_per_frame = 1000.0 / framerate;

u8 palette[256 * 4]; // RGBU format

u8 texture[409920];

void load_texture(std::string texture_filename) {
    FILE* file = std::fopen(texture_filename.c_str(), "rb");
    std::fseek(file, 0, SEEK_END);
    u32 size = std::ftell(file);
    std::fseek(file, 0, SEEK_SET);
    std::fread(&texture, 1, size, file);
    std::fclose(file);
}

u16 angle(i16 x) {
    i16 y = x % 360;
    return (y < 0) ? y + 360 : y;
}

u16 angle_sum(u16 x, i16 increment) {
    if (increment >= 0) return (x+increment) % 360;
    u16 abs = std::abs(increment);
    return angle(x-abs);
}

class Player {
    public:

    u16 alpha;
    u16 beta;
    i16 v_alpha;
    i16 v_beta;

    Player(void) : alpha(0), beta(0), v_alpha(0), v_beta(0) {}

    void update(void) {
        alpha = angle_sum(alpha, v_alpha);
        beta = angle_sum(beta, v_beta);
    }
};

void init_sdl(void) {
	int r = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	if (r < 0) {
        std::cout << "Failed to init SDL: " << SDL_GetError() << std::endl;
		throw 2;
    }
    atexit(SDL_Quit);
}

void tavola_colori (const u8 *nuova_tavolozza,
            unsigned int colore_di_partenza, unsigned int nr_colori,
            i8 filtro_rosso, i8 filtro_verde, i8 filtro_blu)
{
    constexpr unsigned int K_FILTER = 63; // original is 63
    unsigned int c, cc = 0;
    nr_colori *= 4; // using new padding in palette table
    colore_di_partenza *= 4; // new padding

    int pad = 3;
    c = colore_di_partenza;
    while (c < nr_colori-colore_di_partenza) {
        palette[c] = nuova_tavolozza[cc];
        cc++;
        c++;
        if (--pad == 0) {
            palette[c] = 0;
            c++;
            pad = 3;
        }
    }

    c = colore_di_partenza;
    while (c<nr_colori+colore_di_partenza) {
        u16 temp = palette[c];
        temp *= (u8)filtro_rosso;
        temp /= K_FILTER;
        palette[c] = temp;
        c++;
        temp = palette[c];
        temp *= (u8)filtro_verde;
        temp /= K_FILTER;
        palette[c] = temp;
        c++;
        temp = palette[c];
        temp *= (u8)filtro_blu;
        temp /= K_FILTER;
        palette[c] = temp;
        c+=2;
    }
}

// we only need to call tinte(0),
// any higher parameter value is only used close to Sunny
void tinte (unsigned char satu)
{
    u8 buffer[1057];

    constexpr unsigned char K = 255;
    constexpr unsigned int GRAD_COUNT_1 = 16 * 3;
    constexpr unsigned int GRAD_COUNT_2 = 16 * 3;

    constexpr float F1 = 256.f / GRAD_COUNT_1;
    constexpr float F2 = 256.f / GRAD_COUNT_2;

    unsigned int i;
    for (i=0; i < 768; i++) {
        buffer[i] = K;
    }
    for (i=0; i < GRAD_COUNT_1; i += 3) {
        buffer[i] = buffer[i + 1] = satu;
        buffer[i + 2] = static_cast<float>(i) * F1;
    }
    for (i=0; i < GRAD_COUNT_2; i += 3) {
        unsigned int v = static_cast<float>(i) * F2 + satu;
        if (v > K) v = K;
        buffer[i + GRAD_COUNT_1] = v;
        buffer[i + GRAD_COUNT_1 + 1] = v;
    }
    tavola_colori (buffer, 0, 256, 63, 63, 63);
}

bool poll_events(Player* p) {
    bool quit = false;
    SDL_Event sdlevent;
    while (SDL_PollEvent(&sdlevent)) {
        switch (sdlevent.type) {
            case SDL_EVENT_WINDOW_RESIZED:
                break;
            case SDL_EVENT_QUIT:
                quit = true;
                break;
            case SDL_EVENT_KEY_UP:
                if (!sdlevent.key.down) {
                    int key = sdlevent.key.key;
                    switch (key) {
                        case SDLK_ESCAPE:
                            quit = true;
                            break;
                        case SDLK_UP:
                        case SDLK_DOWN:
                            p->v_alpha = 0;
                            break;
                        case SDLK_RIGHT:
                        case SDLK_LEFT:
                            p->v_beta = 0;
                            break;
                        default:
                            break;
                    }
                }
                break;
            case SDL_EVENT_KEY_DOWN:
                if (sdlevent.key.down) {
                    int key = sdlevent.key.key;
                    switch (key) {
                        case SDLK_UP:
                            p->v_alpha = -1;
                            break;
                        case SDLK_DOWN:
                            p->v_alpha = 1;
                            break;
                        case SDLK_RIGHT:
                            p->v_beta = -1;
                            break;
                        case SDLK_LEFT:
                            p->v_beta = 1;
                            break;
                        default:
                            break;
                    }
                }
                break;
        }
    }
    return quit;
}

Renderer* renderer_small;
Renderer* renderer_large;
Player player;

inline void draw(Renderer* renderer) {
    renderer->draw_atmosphere(player.alpha, player.beta);
    renderer->render(palette);
    renderer->pclear(0);
}

bool main_loop(void) {
    player.update();

    draw(renderer_small);
    draw(renderer_large);

    return poll_events(&player);
}

int main (int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Command line argument expected: please give the path to an atmosphere file, e.g. \"NEBULA.ATM\"" << std::endl;
        return -1;
    }

    std::string filename(argv[1]);

    init_sdl();
    load_texture(filename);
    tinte(0);

    renderer_small = new Renderer((filename + " (320x200)").c_str(), 320, 200, 2.0, 2.0);
    renderer_large = new Renderer((filename + " (640x400)").c_str(), 640, 400);

    renderer_small->init_atmosphere(texture, palette);
    renderer_large->init_atmosphere(texture, palette);

    bool quit = false;
	while (!quit) {
        u64 tick_start = SDL_GetTicks();
        u64 next_frame = tick_start + ticks_per_frame;

        quit = main_loop();

        u32 tick_now = SDL_GetTicks();
        while(tick_now < next_frame) {
            SDL_Delay(3);
            tick_now = SDL_GetTicks();
        }
    }
}