#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>

#include "sdl_exception.h"
#include "types.h"

/// Convert a u8* color to SDL_Color format.
/// The given color is assumed to be at least 3 bytes long and in RGB format (any additional bytes are ignored)
SDL_Color color(u8* rgb) {
    return {rgb[0], rgb[1], rgb[2], 255};
}

/// Convert a u8* palette to SDL_Palette format.
/// The given palette is assumed to be 256 entries long, each entry in format RGBU (U = unused) 
SDL_Palette* convert_palette_to_SDL(u8* palette) {
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
    
    private:

    u32 width;
    u32 height;
    u32 window_width;
    u32 window_height;
    u32 framebuffer_size;

    u8* video_buffer;
    
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    // TODO: are surface_32 and texture both needed?
    SDL_Surface* surface_32 = nullptr;
    SDL_Texture* texture = nullptr;

    bool atmosphere_initialized = false;
    SDL_Surface* atmosphere = nullptr;
    SDL_Surface* atmosphere_scaled = nullptr;
    double atm_scale_x, atm_scale_y;
    

    /// Compute the starting index for the atmosphere texture lookup
    u32 get_atmosphere_start(u16 alpha, u16 beta) {
        u32 a1 = (((360 - beta) * 32) / 36) * 4;
        u32 a2 = 3 * width * (alpha % 360);
        return a1 % width + a2;
    }


    public:

    /// Create a Renderer object with the given resolution and pixel scaling.
    Renderer(char* window_text, u16 width_, u16 height_, double scale_x = 1.0, double scale_y = 1.0) {
        width = width_;
        height = height_;
        window_width = scale_x * width;
        window_height = scale_y * height;
        framebuffer_size = width * height;
        atm_scale_x = static_cast<double>(width)/320;
        atm_scale_y = static_cast<double>(height)/200;

        // create software video buffer
        video_buffer = new u8[framebuffer_size];
        memset(&video_buffer[0], 0, framebuffer_size);

        surface_32 = SDL_CreateSurface(
            width,
            height,
            SDL_PIXELFORMAT_RGBX32
        );
        if (surface_32 == nullptr) throw sdl_exception();

        window = SDL_CreateWindow(
            window_text,
            window_width,
            window_height,
            SDL_WINDOW_RESIZABLE
        );
        if (window == nullptr) throw sdl_exception();

        renderer = SDL_CreateRenderer(window, nullptr);
        texture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_RGBX32,
            SDL_TEXTUREACCESS_STREAMING,
            width,
            height
        );

        SDL_SetWindowMinimumSize(window, width, height);
    }

    /// Convert the atmosphere texture to an SDL surface and scale it up if needed.
    /// Must call this method before draw_atmosphere!
    void init_atmosphere(u8* texture, u8* palette) {
        atmosphere_initialized = true;
        // create an SDL surface from the texture, giving it arbitrary width and height
        // (only condition is that atm_w * atm_h == texture size in bytes)
        int atm_w = 640;
        int atm_h = 639;
        atmosphere = SDL_CreateSurfaceFrom(atm_w, atm_h, SDL_PIXELFORMAT_INDEX8, texture, atm_w);

        // very important: set palette! (segfault otherwise)
        SDL_SetSurfacePalette(atmosphere, convert_palette_to_SDL(palette));

        // create the scaled surface
        atmosphere_scaled = SDL_ScaleSurface(atmosphere, atm_scale_x*atm_w, atm_scale_y*atm_h, SDL_SCALEMODE_LINEAR);
    }

    /// Copy the (scaled) atmosphere onto the video buffer
    void draw_atmosphere(u16 alpha, u16 beta) {
        if (!atmosphere_initialized)
            return;

        u32 start_idx = get_atmosphere_start(alpha, beta);

        SDL_LockSurface(atmosphere_scaled);
        u8* pixels = static_cast<u8*>(atmosphere_scaled->pixels);
        memcpy(&video_buffer[0], &pixels[start_idx], width*height);
        SDL_UnlockSurface(atmosphere_scaled);
    }

    /// Clear a graphical page with a pattern
    void pclear (u8 pattern) {
        memset(&video_buffer[0], pattern, framebuffer_size*sizeof(video_buffer[0]));
    }

    /// Draw the video buffer to screen
    void render (u8* palette) {
        // convert indexed 8-bit to RGBA 32-bit colors
        SDL_LockTexture(texture, nullptr, &surface_32->pixels, &surface_32->pitch);
        // paint into surface pixels
        unsigned int rpos = 0;
        unsigned int rpos_32 = 0;
        unsigned char* p_orig = &video_buffer[0];
        unsigned int* p_dest = static_cast<unsigned int*>(surface_32->pixels);
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
            rpos_32 += (surface_32->pitch >> 2);
        }
        SDL_UnlockTexture(texture);
        SDL_RenderTexture(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }
};