#include <string>
#include <vector>

#include <cairo.h>

using namespace std;

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
		bool m_prepared = false;
		int m_image_size;
		unsigned char *m_blank_image;

		// cairo stuff
		cairo_surface_t *m_surface;
		cairo_t *m_cr;

		// positional elements
		int m_font_size;
		int m_line_spacing;
		bool m_centered;
		int m_screen_center;
		bool m_ghost_box;
		int m_max_lines;
		int m_image_width; // must be evenly divisible by 16
		int m_image_height;  // must be evenly divisible by 16
};
