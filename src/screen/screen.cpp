#include "screen/screen.hpp"

#include <cmath>
#include <vector>
#include <string>

namespace rt {

	static constexpr std::string DEFAULT_TITLE = "Raytracer_project";

	using enum sdl::window::flag;
	
	screen::screen(matrix& matrix, tone_mapping_parameters::mode mode, float gamma)
		: 	window(DEFAULT_TITLE, { 10, 10, matrix.width, matrix.height }, { AllowHighDPI, Resizable }),
			renderer(window, matrix.width, matrix.height, { sdl::renderer::flag::Accelerated }, sdl::renderer::vsync_option::Disabled),
			srcrect(0, 0, matrix.width, matrix.height),
			dstrect(0, 0, matrix.width, matrix.height),
			texture(renderer, sdl::texture::PixelFormat::RGB24, sdl::texture::Access::Streaming, matrix.width, matrix.height),
			mat(matrix), width(matrix.width), height(matrix.height), tone_mapping_mode(mode), gamma(gamma) {

		sdl::init({ sdl::Init::Video });
	}

	screen::screen(image& image, tone_mapping_parameters::mode mode)
		: screen(image.data, mode, image.gamma) {}

	screen::~screen() noexcept {
		sdl::quit();
	}

	void screen::set_pixel(int x, int y, Uint8 r, Uint8 g, Uint8 b) const {
		
		renderer.set_render_draw_color(r, g, b);
		renderer.draw_point(x, y);
	}

	void screen::set_pixel(int x, int y, const color& c) const {
		Uint8 r = c.get_red();
		Uint8 g = c.get_green();
		Uint8 b = c.get_blue();
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
		
		const unsigned int padding = texture_pitch % 3;
		const unsigned int shift = 3 * width + padding;

		for (int i = 0; std::vector<color>& line : mat.data) {

            for (int index = 3 * i; const color& pixel_col : line) {

				color avg = pixel_col * invN;
				avg.in_place_max_out();
				texture_pixels[index] 	  = static_cast<Uint8>(avg.get_red());
				texture_pixels[index + 1] = static_cast<Uint8>(avg.get_green());
				texture_pixels[index + 2] = static_cast<Uint8>(avg.get_blue());
				index += shift;
            }
			i++;
        }
	}

	/* Copy matrix to the screen with gamma correction */
	void screen::fast_copy_gamma(const unsigned int number_of_rays) const {

		const real invN = 1.0_r / static_cast<real>(number_of_rays);
		constexpr real inv255 = 1.0_r / 255.0_r;
		const real inv = inv255 * invN;

		const texture::lock lock = texture.get_lock();
		const auto [ texture_pixels, texture_pitch ] = lock.info;

		const unsigned int padding = texture_pitch % 3;
		const unsigned int shift = 3 * width + padding;
		
		for (int i = 0; const std::vector<color>& line : mat.data) {

            for (int index = 3 * i; const color& pixel_col : line) {

				color corrected = pixel_col * inv;
				corrected ^= gamma;
				corrected *= 255.0_r;
				corrected.in_place_max_out();
				texture_pixels[index] 	  = static_cast<Uint8>(corrected.get_red());
				texture_pixels[index + 1] = static_cast<Uint8>(corrected.get_green());
				texture_pixels[index + 2] = static_cast<Uint8>(corrected.get_blue());
				index += shift;
            }
			i++;
        }
	}

	/* Copy matrix to the screen with gamma correction and extended Reinhardt local tone mapping */
	void screen::fast_copy_reinhardt(const unsigned int number_of_rays) const {

		const real invN = 1.0_r / static_cast<real>(number_of_rays);

		// Computation of the maximum luminance
		real max_luminance = 0.0_r;
		for (const std::vector<color>& line : mat.data) {
			for (const rt::color& col : line) {
				const real luminance = (0.2126_r * col.get_red() + 0.7152_r * col.get_green() + 0.0722_r * col.get_blue()) * invN;
				if (luminance > max_luminance)
					max_luminance = luminance;
			}
		}
		const real lwhitecorr = 1.0_r / (max_luminance * max_luminance);

		const texture::lock lock = texture.get_lock();
		const auto [ texture_pixels, texture_pitch ] = lock.info;

		const unsigned int padding = texture_pitch % 3;
        
		//unsigned int index = 0;
		constexpr real inv255 = 1.0_r / 255.0_r;
		const real inv = inv255 * invN;

		const unsigned int shift = 3 * width + padding;
		for (int i = 0; const std::vector<color>& line : mat.data) {
			
            for (int index = 3 * i; const color& col : line) {

				const real lr = col.get_red();
				const real lg = col.get_green();
				const real lb = col.get_blue();
				
				const real lin = (0.2126_r * lr + 0.7152_r * lg + 0.0722_r * lb) * inv;
				// const real lin = (lr + lg + lb) * inv * 0.333;
				const real lcorr = (1.0_r + lin * lwhitecorr) / (1.0_r + lin);

				color corrected = col * (lcorr * inv);
				corrected ^= gamma;
				corrected *= 255.0_r;
				corrected.in_place_max_out();

				texture_pixels[index] 	  = static_cast<Uint8>(corrected.get_red());
				texture_pixels[index + 1] = static_cast<Uint8>(corrected.get_green());
				texture_pixels[index + 2] = static_cast<Uint8>(corrected.get_blue());
				index += shift;
            }
			i++;
        }
	}
}
