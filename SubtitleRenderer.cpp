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
: m_alignment(centered ? CENTER_ALIGN : LEFT_ALIGN),
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
	if(m_alignment == CENTER_ALIGN)
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

	// font faces
	cairo_font_face_t *m_normal_font = cairo_toy_font_face_create("FreeSans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_font_face_t *m_italic_font = cairo_toy_font_face_create("FreeSans", CAIRO_FONT_SLANT_ITALIC, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_font_face_t *m_bold_font = cairo_toy_font_face_create("FreeSans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);

	// prepare scaled fonts
    cairo_matrix_t sizeMatrix, ctm;
    cairo_matrix_init_identity(&ctm);
    cairo_matrix_init_scale(&sizeMatrix, m_font_size, m_font_size);
    cairo_font_options_t *options = cairo_font_options_create();

	m_normal_font_scaled = cairo_scaled_font_create(m_normal_font, &sizeMatrix, &ctm, options);
	m_italic_font_scaled = cairo_scaled_font_create(m_italic_font, &sizeMatrix, &ctm, options);
	m_bold_font_scaled = cairo_scaled_font_create(m_bold_font, &sizeMatrix, &ctm, options);

	// font colours
	m_ghost_box_transparency = cairo_pattern_create_rgba(0, 0, 0, 0.5f);
	m_default_font_color = cairo_pattern_create_rgba(0.866667, 0.866667, 0.866667, 1);
	m_black_font_outline = cairo_pattern_create_rgba(0, 0, 0, 1);

	// cleanup
	cairo_font_options_destroy(options);
	cairo_font_face_destroy(m_normal_font);
	cairo_font_face_destroy(m_italic_font);
	cairo_font_face_destroy(m_bold_font);
}

void SubtitleRenderer::set_font(int new_font_type)
{
	if(new_font_type == m_current_font) return;

	switch(new_font_type) {
		case NORMAL_FONT:
			cairo_set_scaled_font(m_cr, m_normal_font_scaled);
			break;
		case BOLD_FONT:
			cairo_set_scaled_font(m_cr, m_bold_font_scaled);
			break;
		case ITALIC_FONT:
			cairo_set_scaled_font(m_cr, m_italic_font_scaled);
			break;
	}

	m_current_font = new_font_type;
}

void SubtitleRenderer::set_color(int new_color)
{
	if(new_color == m_color) return;

	if(new_color == -1)
		cairo_set_source(m_cr, m_default_font_color);
	else if(new_color == -2)
		cairo_set_source(m_cr, m_ghost_box_transparency);
	else if(new_color == 0)
		cairo_set_source(m_cr, m_black_font_outline);
	else
	{
		int x = new_color;

		int red = x >> 16;
		x -= (red << 16);

		int green = x >> 8;
		x -= (green << 8);

		float r = red / 255.0f;
		float g = green / 255.0f;
		float b = x / 255.0f;

		cairo_set_source_rgba(m_cr, r, g, b, 1);
	}

	m_color = new_color;
}


void SubtitleRenderer::prepare(vector<string> &lines)
{
	if(m_prepared) unprepare();

	vector<vector<SubtitleText> > parsed_lines = m_tag_parser->ParseLines(lines);

	// create surface
	m_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, m_image_width, m_image_height);
	m_cr = cairo_create(m_surface);

	// Reset font control vars as no font or drawing dolour has been set
	m_current_font = -500;
	m_color = -500;

	// cursor y position
	int cursor_y_position = m_image_height - m_padding;

	// Limit the number of line
	int no_of_lines = parsed_lines.size();
	if(no_of_lines > m_max_lines) no_of_lines = m_max_lines;

	for(int i = no_of_lines - 1; i > -1; i--) {
		int box_width = (m_padding * 2);
		int text_parts = parsed_lines[i].size();

		// cursor x position
		int cursor_x_position = 0;

		for(int j = 0; j < text_parts; j++) {
			set_font(parsed_lines[i][j].font);

			// prepare font glyphs
			cairo_status_t status = cairo_scaled_font_text_to_glyphs(
					cairo_get_scaled_font(m_cr),
					cursor_x_position + m_padding,
					cursor_y_position - (m_padding / 2),
					parsed_lines[i][j].text.c_str(), -1,
					&parsed_lines[i][j].glyphs,
					&parsed_lines[i][j].num_glyphs,
					NULL, NULL, NULL);

			if (status != CAIRO_STATUS_SUCCESS) {
				puts("cairo_scaled_font_text_to_glyphs: failed");
				return;
			}

			// calculate font extents
			cairo_text_extents_t extents;
			cairo_glyph_extents (m_cr,
				parsed_lines[i][j].glyphs,
				parsed_lines[i][j].num_glyphs,
				&extents);

			cursor_x_position += extents.x_advance;
		}
		box_width += cursor_x_position;

		// aligned text
		if(m_alignment == CENTER_ALIGN || m_alignment == RIGHT_ALIGN) {
			if(m_alignment == RIGHT_ALIGN)
				cursor_x_position = m_image_width - box_width;
			else
				cursor_x_position = m_screen_center - (box_width / 2);

			for(int j = 0; j < text_parts; j++) {
				cairo_glyph_t *p = parsed_lines[i][j].glyphs;
				for(int h = 0; h < parsed_lines[i][j].num_glyphs; h++, p++) {
					p->x +=cursor_x_position;
				}
			}
		} else {
			cursor_x_position = 0;
		}

		// draw ghost box
		if(m_ghost_box) {
			set_color(-2);
			cairo_rectangle(m_cr, cursor_x_position, cursor_y_position - m_font_size, box_width,
				m_font_size + m_padding);
			cairo_fill(m_cr);
		}

		for(int j = 0; j < text_parts; j++) {
			set_font(parsed_lines[i][j].font);
			set_color(parsed_lines[i][j].color);

			// draw text
			cairo_glyph_path(m_cr, parsed_lines[i][j].glyphs, parsed_lines[i][j].num_glyphs);

			// free glyph array
			cairo_glyph_free(parsed_lines[i][j].glyphs);
		}

		// draw black text outline
		cairo_fill_preserve(m_cr);
		set_color(0);
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

	cairo_scaled_font_destroy(m_normal_font_scaled);
	cairo_scaled_font_destroy(m_italic_font_scaled);
	cairo_scaled_font_destroy(m_bold_font_scaled);

	delete m_tag_parser;
}
