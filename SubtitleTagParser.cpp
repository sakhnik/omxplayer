/*
 *      Copyright (C) 2020 Michael J. Walsh
 *
 *      Based on COMXSubtitleTagSami.cpp by Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "SubtitleTagParser.h"
#include "utils/RegExp.h"

#include <boost/algorithm/string.hpp>

using namespace std;

SubtitleTagParser::SubtitleTagParser()
{
	m_tags = new CRegExp(true);
	m_tags->RegComp("(<[^>]*>|\\{\\\\[^\\}]*\\})");

	m_font_color_html = new CRegExp(true);
	m_font_color_html->RegComp("color[ \\t]*=[ \\t\"']*#?([a-f0-9]{6})");

	m_font_color_curly = new CRegExp(true);
	m_font_color_curly->RegComp("^\\{\\\\c&h([a-f0-9]{2})([a-f0-9]{2})([a-f0-9]{2})&\\}$");
}


vector<vector<SubtitleText> > SubtitleTagParser::ParseLines(vector<string> &text_lines)
{
	vector<vector<SubtitleText> > formatted_lines(text_lines.size());

	bool bold = false, italic = false, color = false;

	for(uint i=0; i < text_lines.size(); i++) {
		boost::algorithm::trim(text_lines[i]);

		int pos = 0, old_pos = 0;

		int line_length = text_lines[i].length();
		while (pos < line_length) {
			pos = m_tags->RegFind(text_lines[i].c_str(), pos);

			//parse text
			if(pos != old_pos) {
				string t = text_lines[i].substr(old_pos, pos - old_pos);
				formatted_lines[i].emplace_back(move(t), bold, italic, color, m_color_code);
			}

			// No more tags found
			if(pos < 0) break;

			// Parse Tag
			string fullTag = m_tags->GetMatch(0);
			boost::algorithm::to_lower(fullTag);
			pos += fullTag.length();
			old_pos = pos;

			if (fullTag == "<b>" || fullTag == "{\\b1}") {
				bold = true;
			} else if ((fullTag == "</b>" || fullTag == "{\\b0}") && bold) {
				bold = false;
			} else if (fullTag == "<i>" || fullTag == "{\\i1}") {
				italic = true;
			} else if ((fullTag == "</i>" || fullTag == "{\\i0}") && italic) {
				italic = false;
			} else if ((fullTag == "</font>" || fullTag == "{\\c}") && color) {
				color = false;
			} else if (fullTag.substr(0,5) == "<font") {
				if(m_font_color_html->RegFind(fullTag.c_str(), 5) >= 0) {
					color = true;
					m_color_code = hex2int(m_font_color_html->GetMatch(1).c_str());
				} else {
					printf("ERROR 1: no match for '%s'\n", fullTag.c_str());
				}
			} else if(m_font_color_curly->RegFind(fullTag.c_str(), 0) >= 0) {
				color = true;
				string t = m_font_color_curly->GetMatch(3) + m_font_color_curly->GetMatch(2)
					+ m_font_color_curly->GetMatch(1);
				m_color_code = hex2int(t.c_str());
			} else {
				printf("ERROR 2: no match for '%s'\n", fullTag.c_str());
			}
		}
	}

	return formatted_lines;
}

// expects 6 lowercase, digit hex string
int SubtitleTagParser::hex2int(const char *hex)
{
	int r = 0;
	for(int i = 0, f = 20; i < 6; i++, f -= 4)
		if(hex[i] >= 'a')
			r += (hex[i] - 87) << f;
		else
			r += (hex[i] - 48) << f;

	return r;
}

SubtitleTagParser::~SubtitleTagParser()
{
	delete m_tags;
	delete m_font_color_html;
	delete m_font_color_curly;
}
