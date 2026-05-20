#pragma once

#include <SDL2/SDL.h>

namespace sdl {

    void quit() {
        SDL_Quit();
    }

    enum class cursor_option {
        Disable = SDL_DISABLE,
        Enable  = SDL_ENABLE
    };

    void show_cursor(cursor_option c) {
        SDL_ShowCursor(static_cast<int>(c));
    }

    class rect {
        private:
            SDL_Rect r;
        
        public:
            rect() {}
            rect(int x, int y, int w, int h)
                : r({ x, y, w, h }) {}
    };

    class window {
        private:
            SDL_Window *win = nullptr;

        public:
            window() {}
            // window = SDL_CreateWindow("Raytracer_project", 10, 10, width, height, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);

            ~window() {
                if (win != nullptr)
                    SDL_DestroyWindow(win);
                win = nullptr;
            }

            SDL_Window * get_window_pt() const {
                return win;
            }

            // SDL_SetWindowFullscreen(param.scr.window, SDL_WINDOW_FULLSCREEN);
    };

    class renderer {

        enum class vsync_option {
            Disabled = 0, Enabled = 1
        };
        enum class flag {
            Accelerated = SDL_RENDERER_ACCELERATED
        };

        private:
            SDL_Renderer *ren = nullptr;

        public:
            renderer(const window& win, flag f) {
                constexpr int INDEX_FIRST_ONE = -1;
                ren = SDL_CreateRenderer(win.get_window_pt(), INDEX_FIRST_ONE, static_cast<int>(f));
            }

            ~renderer() {
                if (ren != nullptr)
                    SDL_DestroyRenderer(ren);
                ren = nullptr;
            }

            void set_vsync(vsync_option v) const {
                SDL_RenderSetVSync(ren, static_cast<int>(v));
            }

            void set_logical_size(int width, int height) const {
                SDL_RenderSetLogicalSize(ren, width, height);
            }

            void set_render_draw_color(Uint8 r, Uint8 g, Uint8 b) const {
                SDL_SetRenderDrawColor(ren, r, g, b, 255);
            }

            void render_present() const {
                SDL_RenderPresent(ren);
            }
            
            void clear() const {
                SDL_RenderClear(ren);
            }
    };

    class surface {
        private:
            SDL_Surface *sur;

        public:
            surface() {}
            // SDL_CreateRGBSurfaceFrom(static_cast<void*>(pixels.data()),
            //      dwidth, dheight, 8 * depth, // depth bytes per pixel (in bits)
            //      depth * dwidth, 0, 0, 0, 0);

            ~surface() {
                if (sur != nullptr)
                    SDL_FreeSurface(sur);
                sur = nullptr;
            }


    };

    class texture {
        private:
            SDL_Texture *txt;

        public:

            // SDL_CreateTexture(param.scr.renderer, SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STREAMING, width, height);

            ~texture() {
                if (txt != nullptr)
                    SDL_DestroyTexture(txt);
                txt = nullptr;
            }
            
            void unlock() const {
                SDL_UnlockTexture(txt);
            }
    };

    class event {
        
    };
}