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

using namespace std;

SubtitleRenderer::SubtitleRenderer(int display_num, int layer_num, float r_font_size,
	bool centered, bool box_opacity, unsigned int lines)
: m_centered(centered),
  m_ghost_box(box_opacity),
  m_max_lines(lines)
{
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

void SubtitleRenderer::prepare(vector<string> &lines)
{
	if(m_prepared) unprepare();

	m_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, m_image_width, m_image_height);
	m_cr = cairo_create(m_surface);

	cairo_select_font_face(m_cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(m_cr, m_font_size);

	// cursor position
	int cursor_x_position = 0;
	int cursor_y_position = m_image_height - m_padding;

	// Show ghost box?
	float ghost_box_transparency = m_ghost_box ? 0.5f : 0.0f;

	// Limit the number of line
	int no_of_lines = lines.size();
	if(no_of_lines > m_max_lines) no_of_lines = m_max_lines;

	for(int i = no_of_lines - 1; i > -1; i--) {
		cairo_text_extents_t extents;
		cairo_text_extents(m_cr, lines[i].c_str(), &extents);
		int box_width = extents.x_advance + (m_padding * 2);

		// centered text
		if(m_centered)
			cursor_x_position = m_screen_center - (box_width / 2);

		// draw ghost box
		cairo_set_source_rgba(m_cr, 0, 0, 0, ghost_box_transparency);
		cairo_rectangle(m_cr, cursor_x_position, cursor_y_position - m_font_size, box_width, 
			m_font_size + m_padding);
		cairo_fill(m_cr);

		// draw (slightly off colour) white text
		cairo_set_source_rgba(m_cr, 0.866667, 0.866667, 0.866667, 1);
		cairo_move_to(m_cr, cursor_x_position + m_padding, cursor_y_position);
		cairo_text_path(m_cr, lines[i].c_str());
		cairo_fill_preserve(m_cr);

		// draw black text outline
		cairo_set_source_rgba(m_cr, 0, 0, 0, 1);
		cairo_set_line_width(m_cr, 2);
		cairo_stroke(m_cr);

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
}
