/*
 *
 *		Copyright (C) 2020 Michael J. Walsh
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include <string>
#include <vector>

#include <cairo.h>

#include "SubtitleRenderer.h"
#include "DispmanxLayer.h"
#include "SubtitleTagParser.h"

using namespace std;

SubtitleRenderer::SubtitleRenderer(int display_num, int layer_num, float r_font_size,
	bool centered, bool box_opacity, unsigned int lines)
: m_centered(centered),
  m_ghost_box(box_opacity),
  m_max_lines(lines)
{
	m_tag_parser = new SubtitleTagParser();

	int screen_width, screen_height;
	openDisplay(display_num, screen_width, screen_height);

	//Calculate font as thousands of screen height
	m_font_size = screen_height * r_font_size;

	// Calculate padding as 1/4 of the font size
	m_padding = m_font_size / 4;

	// And line_height combines the two
	int line_height = m_font_size + m_padding;

	// Alignment: a fairly unscientific survey showed that with a font size of 59px
	// subtitles lines were rarely longer than 1300px.
	int margin_left;
	int assumed_longest_subtitle_line_in_pixels = m_font_size * 22.5;
	m_screen_center = screen_width / 2;
	if(m_centered)
		margin_left = 0;
	else if(screen_width > assumed_longest_subtitle_line_in_pixels)
		margin_left = (int)(screen_width - assumed_longest_subtitle_line_in_pixels) / 2;
	else if(screen_width > screen_height)
		margin_left = (int)(screen_width - screen_height) / 2;
	else
		margin_left = 0;

	// Calculate image height - must be evenly divisible by 16
	m_image_height = (m_max_lines * line_height) + 5;
	m_image_height = (m_image_height + 15) & ~15; // grow to fit

	m_image_width = screen_width - margin_left - 100;
	m_image_width = m_image_width & ~15; // shrink to fit

	// bottom margin (relative to top)
	int top_margin = screen_height - m_image_height - (line_height / 2);

	// Create image layer
	createImageLayer(layer_num, margin_left, top_margin, m_image_width, m_image_height);
}


void SubtitleRenderer::change_font_italic(SubtitleText &st, bool setAnyway)
{
	if(setAnyway || m_italic != st.italic) {
		cairo_select_font_face(m_cr, st.italic ? "FreeSansOblique" : "FreeSans",
			CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
		m_italic = st.italic;
	}
}

void SubtitleRenderer::change_font_color(SubtitleText &st, bool setAnyway)
{
	if(setAnyway || m_font_color != st.color || (m_font_color && st.color &&
			(m_font_color_code[0] != st.color_code[0]
			|| m_font_color_code[1] != st.color_code[1]
			|| m_font_color_code[2] != st.color_code[2]))) {

		float r, g, b;
		if(st.color) {
			r = (float)st.color_code[0] / 255;
			g = (float)st.color_code[1] / 255;
			b = (float)st.color_code[2] / 255;
		} else {
			r = g = b = 0.866667;
		}

		cairo_set_source_rgba(m_cr, r, g, b, 1);
		memcpy(m_font_color_code, st.color_code, 3);
	}
}


void SubtitleRenderer::prepare(vector<string> &lines)
{
	if(m_prepared) unprepare();

	vector<vector<SubtitleText> > parsed_lines = m_tag_parser->ParseLines(lines);

	m_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, m_image_width, m_image_height);
	m_cr = cairo_create(m_surface);

	cairo_set_font_size(m_cr, m_font_size);

	// cursor position
	int cursor_y_position = m_image_height - m_padding;

	// Show ghost box?
	float ghost_box_transparency = m_ghost_box ? 0.5f : 0.0f;

	// Limit the number of line
	int no_of_lines = parsed_lines.size();
	if(no_of_lines > m_max_lines) no_of_lines = m_max_lines;

	bool firstBox = true;
	for(int i = no_of_lines - 1; i > -1; i--) {
		vector<int> extent_widths(parsed_lines[i].size());
		int box_width = (m_padding * 2);

		for(uint h = 0; h < parsed_lines[i].size(); h++) {
			change_font_italic(parsed_lines[i][h], firstBox);
			firstBox = false;

			cairo_text_extents_t extents;
			cairo_text_extents(m_cr, parsed_lines[i][h].text.c_str(), &extents);

			extent_widths[h] = extents.x_advance;
			box_width += extents.x_advance;
		}

		// centered text
		int cursor_x_position;
		if(m_centered)
			cursor_x_position = m_screen_center - (box_width / 2);
		else
			cursor_x_position = 0;

		// draw ghost box
		cairo_set_source_rgba(m_cr, 0, 0, 0, ghost_box_transparency);
		cairo_rectangle(m_cr, cursor_x_position, cursor_y_position - m_font_size, box_width, 
			m_font_size + m_padding);
		cairo_fill(m_cr);

		bool firstBox = true;
		for(uint h = 0; h < parsed_lines[i].size(); h++) {
			change_font_italic(parsed_lines[i][h]);
			change_font_color(parsed_lines[i][h], firstBox);
			firstBox = false;

			cairo_move_to(m_cr, cursor_x_position + m_padding, cursor_y_position - (m_padding / 2));
			cairo_text_path(m_cr, parsed_lines[i][h].text.c_str());
			cairo_fill_preserve(m_cr);

			// draw black text outline
			cairo_set_source_rgba(m_cr, 0, 0, 0, 1);
			cairo_set_line_width(m_cr, 2);
			cairo_stroke(m_cr);

			// move cursor across
			cursor_x_position += extent_widths[h];
		}

		// next line
		cursor_y_position -= m_font_size + m_padding;
	}

	m_prepared = true;
}

void SubtitleRenderer::show_next()
{
	if(m_prepared) {
		unsigned char *image_data = cairo_image_surface_get_data(m_surface);
		setImageData(image_data);
		unprepare();
	}
}

void SubtitleRenderer::hide()
{
	hideElement();
}

void SubtitleRenderer::unprepare()
{
	if(!m_prepared) return;

	cairo_destroy(m_cr);
	cairo_surface_destroy(m_surface);
	m_prepared = false;
}

SubtitleRenderer::~SubtitleRenderer()
{
	unprepare();
	removeImageLayer();
	delete m_tag_parser;
}
