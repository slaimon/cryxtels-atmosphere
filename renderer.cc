#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>

#include "sdl_exception.h"
#include "types.h"

SDL_Color color(u8* rgb) {
    return {rgb[0], rgb[1], rgb[2], 255};
}

// the given palette is assumed to be 256 entries long, each in format RGBU (U = unused) 
SDL_Palette* convert_palette(u8* palette) {
    SDL_Palette* sdl_palette = SDL_CreatePalette(256);
    SDL_Color* colors = new SDL_Color[256];
    for (int i = 0; i < 256; i++) {
        u8* rgb = &palette[4*i];
        colors[i] = color(rgb);
    }
    SDL_SetPaletteColors(sdl_palette, colors, 0, 256);
    return sdl_palette;
}

class Renderer {
    public:

    u32 width;
    u32 height;
    u32 window_width;
    u32 window_height;
    u32 framebuffer_size;
    
    SDL_Window* p_window = nullptr;
    SDL_Surface* p_surface_32 = nullptr;
    SDL_Surface* p_surface_scaled = nullptr;
    SDL_Renderer* p_renderer = nullptr;
    SDL_Texture* p_texture = nullptr;

    SDL_Surface* atmosphere = nullptr;
    SDL_Surface* atmosphere_scaled = nullptr;
    double atm_scale_x;
    double atm_scale_y;
    
    u8* video_buffer;

    Renderer(u16 width_, u16 height_, double scale_x = 1.0, double scale_y = 1.0) {
        width = width_;
        height = height_;
        window_width = scale_x * width;
        window_height = scale_y * height;
        framebuffer_size = width * height;
        atm_scale_x = static_cast<double>(width)/320;
        atm_scale_y = static_cast<double>(height)/200;
    }

    void init(void) {
        // create software video buffer
        video_buffer = new u8[framebuffer_size];
        memset(&video_buffer[0], 0, framebuffer_size);

        p_surface_32 = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBX32);
        if (p_surface_32 == nullptr) throw sdl_exception();

        p_window = SDL_CreateWindow("Atmosphere texture test",
                window_width, window_height, SDL_WINDOW_RESIZABLE);
        if (p_window == nullptr) throw sdl_exception();

        p_renderer = SDL_CreateRenderer(p_window, nullptr);

        p_texture = SDL_CreateTexture(p_renderer, SDL_PIXELFORMAT_RGBX32,
                                    SDL_TEXTUREACCESS_STREAMING, width,height);

        SDL_SetWindowMinimumSize(p_window, width, height);
    }

    // save the texture as an SDL surface and scale it up if needed
    void init_texture_to_surface(u8* texture, u8* palette) {
        // create an SDL surface from the texture, giving it arbitrary width and height
        // (only condition is that atm_w * atm_h == texture size in bytes)
        int atm_w = 640;
        int atm_h = 639;
        atmosphere = SDL_CreateSurfaceFrom(atm_w, atm_h, SDL_PIXELFORMAT_INDEX8, texture, atm_w);

        // very important: set palette! (segfault otherwise)
        SDL_Palette* sdl_palette = convert_palette(palette);
        SDL_SetSurfacePalette(atmosphere, sdl_palette);

        // create the scaled surface
        atmosphere_scaled = SDL_ScaleSurface(atmosphere, atm_scale_x*atm_w, atm_scale_y*atm_h, SDL_SCALEMODE_LINEAR);
    }

    // draw the atmosphere surface onto the video buffer
    void draw_surface(u16 alpha, u16 beta) {
        u32 a1 = (((360 - beta) * 32) / 36) * 4 * atm_scale_x;
        u32 a2 = 3 * width * (alpha % 360) * atm_scale_y;
        u32 dx = a1 % width + a2;

        SDL_LockSurface(atmosphere_scaled);
        u8* pixels = static_cast<u8*>(atmosphere_scaled->pixels);
        memcpy(&video_buffer[0], &pixels[dx], width*height);
        SDL_UnlockSurface(atmosphere_scaled);
    }

    /// Clear a graphical page with a pattern
    void pclear (u8 pattern) {
        memset(&video_buffer[0], pattern, framebuffer_size*sizeof(video_buffer[0]));
    }

    /// Will access texture up to element number:
    /// `width-1 + 3*359*width + width*height`.
    /// In the usual case where width = 320 and height = 200,
    /// addressed space is [0, 408'959].
    /// Note that NEBULA.ATM has size 409'920.
    void draw_texture(u8* texture, u16 alpha, u16 beta) {
        u32 a1 = (((360 - beta) * 32) / 36) * 4;
        u32 a2 = 3 * width * (alpha % 360);
        u32 dx = a1 % width + a2;

        memcpy(&video_buffer[0], &texture[dx], width*height);
    }

    void render (u8* palette) {
        // convert indexed 8-bit to RGBA 32-bit colors
        SDL_LockTexture(p_texture, nullptr, &p_surface_32->pixels, &p_surface_32->pitch);
        // paint into surface pixels
        unsigned int rpos = 0;
        unsigned int rpos_32 = 0;
        unsigned char* p_orig = &video_buffer[0];
        unsigned int* p_dest = static_cast<unsigned int*>(p_surface_32->pixels);
        for (unsigned int y = 0 ; y < height ; y++) {
            for (unsigned int x = 0 ; x < width ; x++) {
                auto pixel = p_orig[rpos + x];
                unsigned int c = palette[pixel * 4]
                    | (palette[pixel * 4 + 1] << 8)
                    | (palette[pixel * 4 + 2] << 16)
                    | (palette[pixel * 4 + 3] << 24);
                p_dest[rpos_32 + x] = c;
            }
            rpos += width;
            rpos_32 += (p_surface_32->pitch >> 2);
        }
        SDL_UnlockTexture(p_texture);
        SDL_RenderTexture(p_renderer, p_texture, nullptr, nullptr);
        SDL_RenderPresent(p_renderer);
    }
};