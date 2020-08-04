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
		void set_font(int new_font_type);
		void set_color(int new_color);

		enum {
			NORMAL_FONT,
			ITALIC_FONT,
			BOLD_FONT,
		};

		enum {
			LEFT_ALIGN,
			CENTER_ALIGN,
			RIGHT_ALIGN,
		};

		bool m_prepared = false;

		// cairo stuff
		cairo_surface_t *m_surface;
		cairo_t *m_cr;

		cairo_font_face_t *m_italic_font;
		cairo_font_face_t *m_normal_font;
		cairo_scaled_font_t *m_normal_font_scaled;
		cairo_scaled_font_t *m_italic_font_scaled;

		cairo_pattern_t *m_ghost_box_transparency;
		cairo_pattern_t *m_default_font_color;
		cairo_pattern_t *m_black_font_outline;

		// positional elements
		int m_alignment;
		int m_screen_center;
		bool m_ghost_box;
		int m_max_lines;
		int m_image_width; // must be evenly divisible by 16
		int m_image_height;  // must be evenly divisible by 16

		// tag parser
		SubtitleTagParser *m_tag_parser;

		// font properties
		int m_padding;
		int m_font_size;
		int m_current_font;
		int m_color;
};
