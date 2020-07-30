#pragma once
/*
 *      Copyright (C) 2020 Michael J. Walsh
 *
 *      Based on COMXSubtitleTagSami.h by Team XBMC
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

#include <string>
#include <vector>
#include <cstdio>

#include <string.h>

#include "utils/RegExp.h"

using namespace std;

class SubtitleText
{
	public:
		string text;
		bool bold = false;
		bool italic = false;
		bool color = false;
		char color_code[4];

		SubtitleText(string t, bool b, bool i, bool c, const char *cc)
		: text(t), bold(b), italic(i), color(c)
		{
			if(color) strcpy(color_code, cc);
			else memcpy(color_code, "\0\0\0\0", 4);
		};
};

class SubtitleTagParser
{
	public:
		SubtitleTagParser(const SubtitleTagParser&) = delete;
		SubtitleTagParser& operator=(const SubtitleTagParser&) = delete;

		SubtitleTagParser();
		~SubtitleTagParser();

		vector<vector<SubtitleText> > ParseLines(vector<string> &text_lines);

	private:
		void process_color_code(const char *hex, bool inverted);

		CRegExp *m_tags;
		CRegExp *m_font_color_html;
		CRegExp *m_font_color_curly;
		char m_color_code[3];
};
