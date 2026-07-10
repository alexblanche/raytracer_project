#pragma once

#include <SDL2/SDL.h>
#include <span>
#include <utility>
#include <string>

// Function that applies bitwise OR to a list of flags
template<class T, typename UInt = uint32_t>
requires std::is_enum_v<T>
    && std::is_convertible_v<std::underlying_type_t<T>, UInt>

UInt fold(const std::span<const T> elts) {
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

    inline void init(const std::span<const Init> flags) {
        SDL_Init(fold(flags));
    }

    inline void quit() {
        SDL_Quit();
    }

    enum class cursor_option : int {
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

            // Delete copy and move constructors

            rect(int x, int y, int w, int h)
                : r({ x, y, w, h }) {}

        friend class window;
        friend class texture;
    };



    // SDL_Window

    class window {

        private:
            SDL_Window *win = nullptr;

        public:
            enum class flag {
                AllowHighDPI = SDL_WINDOW_ALLOW_HIGHDPI,
                Resizable    = SDL_WINDOW_RESIZABLE,
                FullScreen   = SDL_WINDOW_FULLSCREEN,
                Borderless   = SDL_WINDOW_BORDERLESS,
                Windowed     = 0
            };

            window(const std::string& title, const rect& rect, const std::span<const flag> flags) {
                win = SDL_CreateWindow(title.c_str(), rect.r.x, rect.r.y, rect.r.w, rect.r.h, fold(flags));
            }

            window(window&&)                  = delete;
            window(const window&)             = delete;
            window& operator=(window&&)       = delete;
            window& operator=(const window&)  = delete;

            ~window() noexcept {
                if (win != nullptr)
                    SDL_DestroyWindow(win);
                win = nullptr;
            }

            void set_full_sreen(flag flag) const {
                SDL_SetWindowFullscreen(win, static_cast<uint32_t>(flag));
            }

            void warp_mouse(int x, int y) const {
                SDL_WarpMouseInWindow(win, x, y);
            }

        friend class renderer;
    };


    // SDL_Renderer

    class renderer {

        private:
            SDL_Renderer *ren = nullptr;

        public:
            enum class vsync_option {
                Disabled = 0, Enabled = 1
            };
            
            enum class flag {
                Accelerated = SDL_RENDERER_ACCELERATED
            };

            renderer(const window& win, int width, int height, const std::span<const flag> flags, vsync_option vsync) {
                constexpr int INDEX_FIRST_ONE = -1;
                const uint32_t flag = fold(flags);
                ren = SDL_CreateRenderer(win.win, INDEX_FIRST_ONE, flag);
                SDL_RenderSetVSync(ren, static_cast<int>(vsync));
		        SDL_RenderSetLogicalSize(ren, width, height);
            }

            renderer(renderer&&)                  = delete;
            renderer(const renderer&)             = delete;
            renderer& operator=(renderer&&)       = delete;
            renderer& operator=(const renderer&)  = delete;

            ~renderer() noexcept {
                if (ren != nullptr)
                    SDL_DestroyRenderer(ren);
                ren = nullptr;
            }

            void set_logical_size(int width, int height) const {
                SDL_RenderSetLogicalSize(ren, width, height);
            }

            void set_render_draw_color(Uint8 r, Uint8 g, Uint8 b) const {
                SDL_SetRenderDrawColor(ren, r, g, b, 255);
            }

            void draw_point(int x, int y) const {
                SDL_RenderDrawPoint(ren, x, y);
            }

            void render_present() const {
                SDL_RenderPresent(ren);
            }
            
            void clear() const {
                SDL_RenderClear(ren);
            }
        
        friend class texture;
    };


    // SDL_Surface

    class surface {
        private:
            SDL_Surface *sur = nullptr;

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

            surface(surface&&)                  = delete;
            surface(const surface&)             = delete;
            surface& operator=(surface&&)       = delete;
            surface& operator=(const surface&)  = delete;

            ~surface() noexcept {
                if (sur != nullptr)
                    SDL_FreeSurface(sur);
                sur = nullptr;
            }
    };


    // SDL_Texture

    class texture {

        public:
            enum class PixelFormat : uint32_t {
                RGB24  = SDL_PIXELFORMAT_RGB24,
                BGR24  = SDL_PIXELFORMAT_BGR24,
                RGBA32 = SDL_PIXELFORMAT_RGBA32,
                ARGB32 = SDL_PIXELFORMAT_ARGB32,
                BGRA32 = SDL_PIXELFORMAT_BGRA32,
                ABGR32 = SDL_PIXELFORMAT_ABGR32
            };
            static_assert(std::is_convertible_v<SDL_PixelFormatEnum, uint32_t>);

            enum class Access : int {
                Static    = SDL_TEXTUREACCESS_STATIC,
                Streaming = SDL_TEXTUREACCESS_STREAMING,
                Target    = SDL_TEXTUREACCESS_TARGET
            };
            static_assert(std::is_convertible_v<SDL_TextureAccess, int>);

        private:
            SDL_Texture *txt = nullptr;
            PixelFormat format;

            static uint32_t format_code(PixelFormat f) {
                using enum PixelFormat;
                switch (f) {
                    case RGB24:
                    case BGR24:
                    case RGBA32:
                    case ARGB32:
                    case BGRA32:
                    case ABGR32:
                        return std::to_underlying(f);
                    default:
                        return static_cast<uint32_t>(SDL_PIXELFORMAT_UNKNOWN);
                }
            }

            static int access_code(Access a) {
                return std::to_underlying(a);
            }

        public:

            texture(const renderer& ren, PixelFormat format, Access access, int width, int height) :
                txt(SDL_CreateTexture(ren.ren, format_code(format), access_code(access), width, height)),
                format(format) {}

            texture(texture&&)                  = delete;
            texture(const texture&)             = delete;
            texture& operator=(texture&&)       = delete;
            texture& operator=(const texture&)  = delete;

            ~texture() noexcept {
                if (txt != nullptr)
                    SDL_DestroyTexture(txt);
                txt = nullptr;
            }

            PixelFormat get_format() const {
                return format;
            }

            unsigned int bytes_per_pixel() const {
                using enum PixelFormat;
                switch (format) {
                    case RGB24:
                    case BGR24:
                        return 3;
                    default:
                        return 4;
                }
            }

            struct locked_info {
                char *  pixels;
                int     pitch;
            };

            locked_info lock_texture() const {
                char * pixels;
                int    pitch;
                SDL_LockTexture(txt, nullptr, reinterpret_cast<void**>(&pixels), &pitch);
                return { pixels, pitch };
            }

            locked_info lock_texture(const rect& rect) const {
                char * pixels;
                int    pitch;
                SDL_LockTexture(txt, &rect.r, reinterpret_cast<void**>(&pixels), &pitch);
                return { pixels, pitch };
            }
            
            void unlock_texture() const {
                SDL_UnlockTexture(txt);
            }

            // Lock class: automatically unlocks the texture upon destruction
            class lock {
                
                private:
                    const texture& txt;
                    
                public:
                    locked_info info;

                    lock(const texture& texture)
                        : txt(texture), info(txt.lock_texture()) {}

                    lock(lock&&)                  = delete;
                    lock(const lock&)             = delete;
                    lock& operator=(lock&&)       = delete;
                    lock& operator=(const lock&)  = delete;

                    ~lock() {
                        txt.unlock_texture();
                    }
            };

            lock get_lock() const {
                return lock(*this);
            }

            void render_copy(const renderer& ren, const rect& src_rect, const rect& dst_rect) const {
                SDL_RenderCopy(ren.ren, txt, &src_rect.r, &dst_rect.r);
            }
    };

    
    // SDL_Event

    class event {

        public:
            SDL_Event e;

            enum class type {
                KeyDown         = SDL_KEYDOWN,
                KeyUp           = SDL_KEYUP,
                MouseMotion     = SDL_MOUSEMOTION,
                MouseButtonDown = SDL_MOUSEBUTTONDOWN,
                MouseButtonUp   = SDL_MOUSEBUTTONUP,
                MouseWheel      = SDL_MOUSEWHEEL,
                Quit            = SDL_QUIT,
                Other           = SDL_FIRSTEVENT
            };

            event() {}

            event(event&&)                  = delete;
            event(const event&)             = delete;
            event& operator=(event&&)       = delete;
            event& operator=(const event&)  = delete;

            enum class polling_type {
                Wait, Poll
            };

            bool poll_event() {
                return SDL_PollEvent(&e);
            }

            bool wait_event() {
                return SDL_WaitEvent(&e);
            }

            template <polling_type type>
            bool next_event() {
                if constexpr (type == polling_type::Wait)
                    return wait_event();
                else
                    return poll_event();
            }

            bool wait_event_timeout(int timeout) {
                return SDL_WaitEventTimeout(&e, timeout);
            }

            type get_type() const {
                switch (e.type) {
                    case SDL_KEYDOWN:
                    case SDL_KEYUP:
                    case SDL_MOUSEMOTION:
                    case SDL_MOUSEBUTTONDOWN:
                    case SDL_MOUSEBUTTONUP:
                    case SDL_MOUSEWHEEL:
                    case SDL_QUIT:
                        return static_cast<type>(e.type);
                    default:
                        return type::Other;
                }
            }

            enum class key : int {
                Escape          = SDL_SCANCODE_ESCAPE,
                Space           = SDL_SCANCODE_SPACE,
                Return          = SDL_SCANCODE_RETURN,
                KeyPad_Enter    = SDL_SCANCODE_KP_ENTER,
                B               = SDL_SCANCODE_B,
                R               = SDL_SCANCODE_R,
                Other           = SDL_SCANCODE_UNKNOWN
            };

            key get_key() const noexcept {
                switch (const SDL_Scancode code = e.key.keysym.scancode) {
                    case SDL_SCANCODE_ESCAPE:
                    case SDL_SCANCODE_SPACE:
                    case SDL_SCANCODE_RETURN:
                    case SDL_SCANCODE_KP_ENTER:
                    case SDL_SCANCODE_B:
                    case SDL_SCANCODE_R:
                        return static_cast<key>(code);
                    default:
                        return key::Other;
                }
            }
    };
}
