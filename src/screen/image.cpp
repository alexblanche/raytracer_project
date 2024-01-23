#include "headers/image.hpp"

namespace rt {

	image::image()
	{
	}

	image::image(int width, int height)
	{
		data = SDL_CreateRGBSurface(SDL_SWSURFACE,width,height,32,0,0,0,0);
	}

	image::image(const image& img)
	{
		data = img.data;
		data->refcount++;
	}

	image::~image()
	{
		if(data != NULL)
			SDL_FreeSurface(data);
	}

	image image::copy() const
	{
		image cpy(width(),height());
		blit(cpy,0,0);
		return cpy;
	}

	color image::get_pixel(int x, int y) const
	{
		if(x < 0 || y < 0 || x >= width() || y >= height()) return color(0,0,0,0);
		char* pixel = ((char*)data->pixels) + y*data->pitch + x*4;
		Uint8 r,g,b,a;
		SDL_GetRGBA(*((Uint32*)pixel), data->format, &r, &g, &b, &a);
		return color(r,g,b,a);
	}

	void image::set_pixel(int x, int y, const color& c)
	{
		if(x < 0 || y < 0 || x >= width() || y >= height()) return;
		uint32_t col = SDL_MapRGBA(data->format, c.get_red(), c.get_green(), c.get_blue(), c.get_alpha());
		char* pixel = ((char*)data->pixels) + y*data->pitch + x*4;
		*(uint32_t*)pixel = col;
	}

	void image::blit(image& dst, const rect& srcrect, int dstx, int dsty) const
	{
		SDL_Rect dstrect;
		dstrect.x = dstx;
		dstrect.y = dsty;
		SDL_BlitSurface(data, (SDL_Rect*)(&srcrect), dst.data, &dstrect);
	}

	void image::blit(image& dst, int dstx, int dsty) const
	{
		rect srcrect;
		srcrect.x = 0;
		srcrect.y = 0;
		srcrect.w = dst.data->w;
		srcrect.h = dst.data->h;
		blit(dst,srcrect,dstx,dsty);
	}

	void image::draw_line(int x1, int y1, int x2, int y2, const color& c)
	{
		int x,y;
		int dx,dy;
		int xincr,yincr;
		int erreur;
		int i;

		set_pixel(x1,y1,c);

		dx = abs(x2-x1);
		dy = abs(y2-y1);
		if(x1<x2)
			xincr = 1;
		else
			xincr = -1;
		if(y1<y2)
			yincr = 1;
		else
			yincr = -1;

		x = x1;
		y = y1;
		if(dx>dy)
		{
			erreur = dx/2;
			for(i=0; i<dx; i++)
			{
				x += xincr;
				erreur += dy;
				if(erreur > dx)
				{
					erreur -= dx;
					y += yincr;
				}
				set_pixel(x,y,c);
			}
		}
		else
		{
			erreur = dy/2;
			for(i=0; i<dy; i++)
			{
				y += yincr;
				erreur += dx;

				if(erreur>dy)
				{
					erreur -= dy;
					x += xincr;
				}
				set_pixel(x,y,c);
			}
		}
	}

	void image::draw_rect(int x1, int y1, int x2, int y2, const color& c)
	{
		draw_line(x1,y1,x1,y2,c);
		draw_line(x1,y1,x2,y1,c);
		draw_line(x2,y2,x1,y2,c);
		draw_line(x2,y2,x2,y1,c);
	}

	void image::fill_rect(int x1, int y1, int x2, int y2, const color& c)
	{
		SDL_Rect r = {(Sint16)x1, (Sint16)y1, (Uint16)(x2-x1+1), (Uint16)(y2-y1+1)};
		SDL_FillRect(data, &r,
				SDL_MapRGBA(data->format, c.get_red(), c.get_green(), c.get_blue(), c.get_alpha()));
	}

}
