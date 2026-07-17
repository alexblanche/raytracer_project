#include "screen/screen.hpp"

#include <string>
#include <array>

namespace rt {

	static const std::string DEFAULT_TITLE = "Raytracer_project";

	using enum sdl::window::flag;
	
#if APPLE_CLANG
	screen::screen(image& image, tone_mapping_parameters::mode mode)
		: 	window(DEFAULT_TITLE, sdl::rect(10, 10, image.width(), image.height()), { AllowHighDPI, Resizable }),
			renderer(window, image.width(), image.height(), { sdl::renderer::flag::Accelerated }, sdl::renderer::vsync_option::Disabled),
			srcrect(0, 0, image.width(), image.height()),
			dstrect(0, 0, image.width(), image.height()),
			texture(renderer, sdl::texture::PixelFormat::RGB24, sdl::texture::Access::Streaming, image.width(), image.height()),
			img(image), tone_mapping_mode(mode) {

		sdl::init({ sdl::Init::Video });
	}
#else
	screen::screen(image& image, tone_mapping_parameters::mode mode)
		: 	window(DEFAULT_TITLE, sdl::rect(10, 10, image.width(), image.height()), std::array<sdl::window::flag, 2>{ AllowHighDPI, Resizable }),
			renderer(window, image.width(), image.height(), std::array<sdl::renderer::flag, 1>{ sdl::renderer::flag::Accelerated }, sdl::renderer::vsync_option::Disabled),
			srcrect(0, 0, image.width(), image.height()),
			dstrect(0, 0, image.width(), image.height()),
			texture(renderer, sdl::texture::PixelFormat::RGB24, sdl::texture::Access::Streaming, image.width(), image.height()),
			img(image), tone_mapping_mode(mode) {

		sdl::init(std::array<sdl::Init, 1>{ sdl::Init::Video });
	}
#endif

	// screen::screen(image& image, tone_mapping_parameters::mode mode)
	// 	: screen(image.data, mode, image.gamma) {}

	screen::~screen() noexcept {
		sdl::quit();
	}

	void screen::set_pixel(int x, int y, Uint8 r, Uint8 g, Uint8 b) const {
		
		renderer.set_render_draw_color(r, g, b);
		renderer.draw_point(x, y);
	}

	void screen::set_pixel(int x, int y, const color& c) const {

		const auto [ r, g, b ] = c.to_uint8();
		set_pixel(x, y, r, g, b);
	}

	void screen::clear() const {

		renderer.set_render_draw_color(0, 0, 0);
		renderer.clear();
	}


	/* Flushes the buffer to the screen */
	void screen::update() const {
		renderer.render_present();
	}

	/* Same as update, but first copies the texture onto the renderer */
	void screen::update_from_texture() const {

		renderer.clear();
		texture.render_copy(renderer, srcrect, dstrect);
		renderer.render_present();
	}


	/****************************************************************************************************/
	/** Event processing **/

	using namespace sdl;
	using enum event::polling_type;

	/* Stop at the next quit event */
	template <event::polling_type type>
	static screen::quit_event next_quit_event() {

		using enum screen::quit_event;

		event event;
		while (event.next_event<type>()) {
			using enum event::type;
			switch (event.get_type()) {
				case Quit: 		return QuitEvent;
				case KeyDown:	return KeyBoardEvent;
				default:		break;
			}
		}
		return Error;
	}

	/* Wait indefinitely for the next quit event */
	screen::quit_event screen::wait_quit_event() const {
		return next_quit_event<Wait>();
	}

	/* Stop at the next quit event */
	screen::quit_event screen::is_quit_event() const {
		return next_quit_event<Poll>();
	}

	template <event::polling_type type>
	static screen::key wait_poll_keyboard_event() {
		
		event event;
		while (event.next_event<type>()) {

			using enum event::type;
			
			switch (event.get_type()) {
				
				case Quit:
					return screen::key::QuitEvent;
				
				case KeyDown:
					using enum event::key;
					switch(event.get_key()) {
						
						case Escape:
							return screen::key::QuitEvent;
						
						case Space:
						case Return:
						case KeyPad_Enter:
							return screen::key::SpaceEnter;

						case B:
							return screen::key::B;
						
						case R:
							return screen::key::R;
						
						default:
							break;
					}
					break;
				
				case MouseButtonDown:
					printf("\nX = %d, Y = %d", event.e.button.x, event.e.button.y);
					break;

				default:
					break;
			}
		}
		return screen::key::Other;
	}

	screen::key screen::wait_keyboard_event() const {
		return wait_poll_keyboard_event<Wait>();
	}

	/* Same as wait_keyboard_event, with poll events */
	screen::key screen::poll_keyboard_event() const {
		return wait_poll_keyboard_event<Poll>();
	}

	/****************************************************************************************************/

	/**
	 * Copies the rt::color matrix onto the screen, by averaging the number_of_rays colors per pixel
	 */
	void screen::fast_copy(const unsigned int number_of_rays) const {

		const real invN = 1.0_r / static_cast<real>(number_of_rays);

		const texture::lock lock = texture.get_lock();
		const auto [ texture_pixels, texture_pitch ] = lock.info;
		
		const unsigned int bytes_per_pixel = texture.bytes_per_pixel();

		const unsigned int padding = texture_pitch % bytes_per_pixel;
		const unsigned int row_size = bytes_per_pixel * img.width();
		const unsigned int shift = 2 * row_size + padding;
		
		int index = (img.height() - 1) * (row_size + padding);
		for (const matrix::const_row row : img.data) {
            for (const color& pixel_col : row) {
				color avg = pixel_col * invN;
				avg.cap();
				const auto [ r, g, b ] = avg.to_uint8();
				texture_pixels[index] 	  = r;
				texture_pixels[index + 1] = g;
				texture_pixels[index + 2] = b;
				index += bytes_per_pixel;
            }
			index -= shift;
        }
	}

	/* Copy matrix to the screen with gamma correction */
	void screen::fast_copy_gamma(const unsigned int number_of_rays) const {

		const real invN = 1.0_r / static_cast<real>(number_of_rays);
		constexpr real inv255 = 1.0_r / 255.0_r;
		const real inv = inv255 * invN;

		const texture::lock lock = texture.get_lock();
		const auto [ texture_pixels, texture_pitch ] = lock.info;
		
		const unsigned int bytes_per_pixel = texture.bytes_per_pixel();
		const unsigned int padding = texture_pitch % bytes_per_pixel;
		const unsigned int row_size = bytes_per_pixel * img.width();
		const unsigned int shift = 2 * row_size + padding;
		
		int index = (img.height() - 1) * (row_size + padding);
		for (const matrix::const_row row : img.data) {
            for (const color& pixel_col : row) {

				color corrected = pixel_col * inv;
				corrected ^= img.gamma.value();
				corrected *= 255.0_r;
				corrected.cap();
				const auto [ cr, cg, cb ] = corrected.to_uint8();
				texture_pixels[index] 	  = cr;
				texture_pixels[index + 1] = cg;
				texture_pixels[index + 2] = cb;
				index += bytes_per_pixel;
            }
			index -= shift;
        }
	}

	/* Copy matrix to the screen with gamma correction and extended Reinhardt local tone mapping */
	void screen::fast_copy_reinhardt(const unsigned int number_of_rays) const {

		const real invN = 1.0_r / static_cast<real>(number_of_rays);

		// Computation of the maximum luminance
		real max_luminance = 0.0_r;
		for (const matrix::const_row row : img.data) {
			for (const rt::color& col : row) {
				const auto [ r, g, b ] = col;
				const real luminance = (0.2126_r * r + 0.7152_r * g + 0.0722_r * b) * invN;
				if (luminance > max_luminance)
					max_luminance = luminance;
			}
		}
		const real lwhitecorr = 1.0_r / (max_luminance * max_luminance);

		const texture::lock lock = texture.get_lock();
		const auto [ texture_pixels, texture_pitch ] = lock.info;

		const unsigned int bytes_per_pixel = texture.bytes_per_pixel();
		const unsigned int padding = texture_pitch % bytes_per_pixel;
        
		//unsigned int index = 0;
		constexpr real inv255 = 1.0_r / 255.0_r;
		const real inv = inv255 * invN;

		const unsigned int row_size = bytes_per_pixel * img.width();
		const unsigned int shift = 2 * row_size + padding;
		
		int index = (img.height() - 1) * (row_size + padding);

		for (const matrix::row row : img.data) {
            for (const color& col : row) {

				const auto [ lr, lg, lb ] = col;
				
				const real lin = (0.2126_r * lr + 0.7152_r * lg + 0.0722_r * lb) * inv;
				// const real lin = (lr + lg + lb) * inv * 0.333;
				const real lcorr = (1.0_r + lin * lwhitecorr) / (1.0_r + lin);

				color corrected = col * (lcorr * inv);
				corrected ^= img.gamma.value();
				corrected *= 255.0_r;
				corrected.cap();

				const auto [ cr, cg, cb ] = corrected.to_uint8();
				texture_pixels[index] 	  = cr;
				texture_pixels[index + 1] = cg;
				texture_pixels[index + 2] = cb;
				index += bytes_per_pixel;
            }
			index -= shift;
        }
	}
}
