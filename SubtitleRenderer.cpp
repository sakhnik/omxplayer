#include <string>
#include <vector>

#include <cairo.h>

#include "SubtitleRenderer.h"
#include "DispmanxLayer.h"

using namespace std;

SubtitleRenderer::SubtitleRenderer(int display_num, int layer_num, float r_m_font_size,
	bool centered, bool box_opacity, unsigned int lines)
: m_centered(centered),
  m_ghost_box(box_opacity),
  m_max_lines(lines)
{
	printf("  display_num: %d\n", display_num);
	printf("    layer_num: %d\n", layer_num);
	printf("r_m_font_size: %f\n", r_m_font_size);
	puts(centered ? "     centered: yes" : "     centered: no");
	puts(box_opacity ? "  box_opacity: yes" : "  box_opacity: no");
	printf("        lines: %d\n", lines);

	int screen_width, screen_height;
	openDisplay(display_num, screen_width, screen_height);

	//Calculate font as thousands of screen height
	m_font_size = screen_height * r_m_font_size;

	// Calculate line spacing as 2/5 of the font size
	m_line_spacing = 0.5f * m_font_size;

	// And line_height combines the two
	int line_height = m_font_size + m_line_spacing;

	// Alignment
	int margin_left;
	m_screen_center = screen_width / 2;
	if(m_centered)
		margin_left = 0;
	else
		margin_left = (int)(screen_width - screen_height) / 2;

	// Calculate image height - must be evenly divisible by 16
	m_image_height = (m_max_lines * line_height) + 5;
	m_image_height = (m_image_height + 15) & ~15; // grow to fit

	m_image_width = screen_width - margin_left - 100;
	m_image_width = m_image_width & ~15; // shrink to fit

	// bottom margin (relative to top)
	int top_margin = screen_height - line_height - m_image_height;

	// Start a blank cairo drawing surface
	createBlankSurface();

	// Create image layer
	createImageLayer(layer_num, margin_left, top_margin, m_image_width, m_image_height);
}

void SubtitleRenderer::createBlankSurface()
{
	m_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, m_image_width, m_image_height);
	m_cr = cairo_create(m_surface);
	m_surface_is_blank = true;
}

void SubtitleRenderer::prepare(vector<string> &lines)
{
	if(!m_surface_is_blank) resetSurface();

	cairo_select_font_face(m_cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);

	cairo_set_font_size(m_cr, m_font_size);

	double cursor_y_position = m_image_height - m_line_spacing;

	int h_padding = 20;

	// Show ghost box?
	double ghost_box_transparency = m_ghost_box ? 0.5f : 0.0f;

	// Limit the number of line
	int no_of_lines = lines.size();
	if(no_of_lines > m_max_lines) no_of_lines = m_max_lines;
	for(int i = no_of_lines - 1; i > -1; i--) {
		cairo_text_extents_t extents;
		cairo_text_extents(m_cr, lines[i].c_str(), &extents);
		double box_width = extents.width + (h_padding * 2);

		// centered text
		double cursor_x_position;
		if(m_centered)
			cursor_x_position = m_screen_center - (box_width / 2);
		else
			cursor_x_position = 0.0f;

		// draw ghostBox
		cairo_set_source_rgba(m_cr, 0, 0, 0, ghost_box_transparency);
		cairo_rectangle(m_cr, cursor_x_position, cursor_y_position - m_font_size, box_width, m_font_size + m_line_spacing);
		cairo_fill(m_cr);

		// draw white text
		cairo_set_source_rgba(m_cr, 1, 1, 1, 1);
		cairo_move_to(m_cr, cursor_x_position + h_padding, cursor_y_position);
		cairo_text_path(m_cr, lines[i].c_str());
		cairo_fill_preserve(m_cr);

		// draw black text outline
		cairo_set_source_rgba(m_cr, 0, 0, 0, 1);
		cairo_set_line_width(m_cr, 2);
		cairo_stroke(m_cr);

		// next line
		cursor_y_position -= m_font_size + m_line_spacing;
	}

	// get data pointer
	int stride = cairo_image_surface_get_stride(m_surface);
	int height = cairo_image_surface_get_height(m_surface);
	image_size = stride * height;

	m_surface_is_blank = false;
}

void SubtitleRenderer::show_next()
{
	if(!m_surface_is_blank) {
		copySurfaceToScreen();
		resetSurface();
	}
}

void SubtitleRenderer::hide()
{
	copySurfaceToScreen();
	if(!m_surface_is_blank)
		resetSurface();
}

void SubtitleRenderer::copySurfaceToScreen()
{
	unsigned char* image_data = cairo_image_surface_get_data(m_surface);
	changeImageData(image_data);
}


void SubtitleRenderer::resetSurface()
{
	if(m_surface_is_blank) return;

	deleteSurface();
	createBlankSurface();
}

void SubtitleRenderer::deleteSurface()
{
	cairo_destroy(m_cr);
	cairo_surface_destroy(m_surface);
}

SubtitleRenderer::~SubtitleRenderer()
{
	deleteSurface();
	removeImageLayer();
}
