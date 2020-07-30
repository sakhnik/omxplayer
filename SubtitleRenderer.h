#pragma once
/*
 *
 *      Copyright (C) 2020 Michael J. Walsh
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <string>
#include <vector>

#include <cairo.h>

using namespace std;

class SubtitleText;
class SubtitleTagParser;

class SubtitleRenderer {
	public:
		SubtitleRenderer(const SubtitleRenderer&) = delete;
		SubtitleRenderer& operator=(const SubtitleRenderer&) = delete;
		SubtitleRenderer(int display, int layer,
						float r_font_size,
						bool centered,
						bool box_opacity,
						unsigned int lines);

		~SubtitleRenderer();

		void prepare(vector<string> &lines);
		void show_next();
		void hide();
		void unprepare();

	private:
		void change_font_italic(SubtitleText &st, bool setAnyway = false);
		void change_font_color(SubtitleText &st, bool setAnyway = false);

		bool m_prepared = false;

		// cairo stuff
		cairo_surface_t *m_surface;
		cairo_t *m_cr;

		// positional elements
		int m_font_size;
		int m_padding;
		bool m_centered;
		int m_screen_center;
		bool m_ghost_box;
		int m_max_lines;
		int m_image_width; // must be evenly divisible by 16
		int m_image_height;  // must be evenly divisible by 16

		// tag parser
		SubtitleTagParser *m_tag_parser;

		// font properties
		bool m_italic = false;
		//bool m_bold = false;
		bool m_font_color = false;
		int m_font_color_code = 0;
};
