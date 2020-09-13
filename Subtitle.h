#pragma once
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

#include <vector>
#include <string>

#include "utils/simple_geometry.h"

class Subtitle {
  public:
  Subtitle(bool is_image);

  Subtitle(int start, int stop, std::vector<std::string> &text_lines);

  Subtitle(const Subtitle &old);
  Subtitle& operator=(const Subtitle &old);

  Subtitle(Subtitle &&old) noexcept;
  Subtitle& operator=(Subtitle &&old) noexcept;

  ~Subtitle();

  int start;
  int stop;
  bool isImage = false;

  union {
    std::vector<std::string> text_lines;
    struct {
      std::basic_string<unsigned char> data;
      Rectangle rect;
    } image;
  };
};
