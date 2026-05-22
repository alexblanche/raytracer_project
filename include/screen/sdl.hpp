#pragma once

#include <SDL2/SDL.h>
#include <vector>

template<class T, typename UInt = uint32_t>
requires std::is_enum_v<T>
    && std::is_convertible_v<std::underlying_type_t<T>, UInt>

UInt fold(const std::vector<T>& elts) {
    UInt acc = 0;
    for (T e : elts)
        acc |= static_cast<UInt>(e);
    return acc;
}


namespace sdl {

    // General

    enum class Init {
        Video       = SDL_INIT_VIDEO,
        Events      = SDL_INIT_EVENTS,
        Everything  = SDL_INIT_EVERYTHING
    };

    inline void init(const std::vector<Init>& flags) {
        SDL_Init(fold(flags));
    }

    inline void quit() {
        SDL_Quit();
    }

    enum class cursor_option {
        Disable = SDL_DISABLE,
        Enable  = SDL_ENABLE
    };

    inline void show_cursor(cursor_option c) {
        SDL_ShowCursor(static_cast<int>(c));
    }


    // SDL_Rect

    class rect {

        private:
            SDL_Rect r;
        
        public:
            rect() {}
            rect(int x, int y, int w, int h)
                : r({ x, y, w, h }) {}

            const SDL_Rect * get() const {
                return &r;
            }
    };




    // SDL_Window

    class window {

        enum class flag {
            AllowHighDPI = SDL_WINDOW_ALLOW_HIGHDPI,
            Resizable    = SDL_WINDOW_RESIZABLE,
            FullScreen   = SDL_WINDOW_FULLSCREEN,
            Borderless   = SDL_WINDOW_BORDERLESS,
            Windowed     = 0
        };

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

            void setFullScreen(flag flag) const {
                SDL_SetWindowFullscreen(win, static_cast<uint32_t>(flag));
            }

            void warp_mouse(int x, int y) const {
                SDL_WarpMouseInWindow(win, x, y);
            }
    };


    // SDL_Renderer

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
            renderer(const window& win, std::vector<flag>& flags) {
                constexpr int INDEX_FIRST_ONE = -1;
                const uint32_t flag = fold(flags);
                ren = SDL_CreateRenderer(win.get_window_pt(), INDEX_FIRST_ONE, flag);
            }

            ~renderer() {
                if (ren != nullptr)
                    SDL_DestroyRenderer(ren);
                ren = nullptr;
            }

            SDL_Renderer* get() const {
                return ren;
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


    // SDL_Surface

    class surface {
        private:
            SDL_Surface *sur;

        public:
            surface(int width, int height, int depth, char * pixels, int pitch) {
                sur = SDL_CreateRGBSurfaceFrom(
                    static_cast<void*>(pixels),
                    width, height,
                    8 * depth, // depth bytes per pixel (in bits)
                    pitch,
                    0, 0, 0, 0 // Masks for red, green, blue, alpha
                );
            }

            ~surface() {
                if (sur != nullptr)
                    SDL_FreeSurface(sur);
                sur = nullptr;
            }


    };


    // SDL_Texture

    class texture {

        enum class PixelFormat {
            RGB24 = SDL_PIXELFORMAT_RGB24,
            BGR24 = SDL_PIXELFORMAT_BGR24
        };

        enum class Access {
            Streaming = SDL_TEXTUREACCESS_STREAMING
        };

        private:
            SDL_Texture *txt;

        public:

            texture(const renderer& ren, PixelFormat format, Access access, int w, int h) {
                txt = SDL_CreateTexture(ren.get(), static_cast<uint32_t>(format), static_cast<int>(access), w, h);
            }

            ~texture() {
                if (txt != nullptr)
                    SDL_DestroyTexture(txt);
                txt = nullptr;
            }

            struct locked_info {
                char *  pixels;
                int     pitch;
            };

            locked_info lock() const {
                char * pixels;
                int    pitch;
                SDL_LockTexture(txt, nullptr, reinterpret_cast<void**>(&pixels), &pitch);
                return { pixels, pitch };
            }

            locked_info lock(const rect& rect) const {
                char * pixels;
                int    pitch;
                SDL_LockTexture(txt, rect.get(), reinterpret_cast<void**>(&pixels), &pitch);
                return { pixels, pitch };
            }
            
            void unlock() const {
                SDL_UnlockTexture(txt);
            }

            void render_copy(const renderer& ren, const rect& src_rect, const rect& dst_rect) const {
                SDL_RenderCopy(ren.get(), txt, src_rect.get(), dst_rect.get());
            }
    };

    
    // SDL_Event

    class event {

        enum class type {
            MouseMotion     = SDL_MOUSEMOTION,
            MouseButtonDown = SDL_MOUSEBUTTONDOWN,
            MouseButtonUp   = SDL_MOUSEBUTTONUP,
            KeyDown         = SDL_KEYDOWN,
            KeyUp           = SDL_KEYUP,
            Quit            = SDL_QUIT
        };

        public:
            SDL_Event e;

            bool poll_event() {
                return SDL_PollEvent(&e);
            }

            type get_type() const {
                return static_cast<type>(e.type);
            }
    };
}
