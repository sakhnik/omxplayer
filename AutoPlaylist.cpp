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
#include <algorithm>
#include <dirent.h>

#include "utils/RegExp.h"
#include "AutoPlaylist.h"

using namespace std;

void AutoPlaylist::readPlaylist(string &filename)
{
	// reset object
	playlist_pos = -1;
	playlist.clear();
	dirname = "";

    int pos = filename.find_last_of('/');
    dirname = filename.substr(0, pos+1); // including trailing slash
    string basename = filename.substr(pos+1);

	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir(dirname.c_str())) == NULL) {
		puts("Failed to open directory for reading");
		return;
	}

	// re for filename match
	CRegExp *fnameext_match;
	fnameext_match = new CRegExp(true);
	fnameext_match->RegComp("\\.(3g2|3gp|amv|asf|avi|drc|f4a|f4b|f4p|f4v|flv|"
		"m2ts|m2v|m4p|m4v|mkv|mov|mp2|mp4|mpe|mpeg|mpg|mpv|mts|mxf|nsv|ogg|"
		"ogv|qt|rm|rmvb|roq|svi|ts|vob|webm|wmv|yuv|iso|dmg)$");

	while ((ent = readdir(dir)) != NULL) {
		if(ent->d_type != 4 && ent->d_name[0] != '.' &&
				fnameext_match->RegFind(ent->d_name, 0) > -1) {
			playlist.push_back(ent->d_name);
		}
	}

	sort(playlist.begin(), playlist.end());

	for(uint i = 0; i < playlist.size(); i++) {
		if(playlist[i] == basename) {
			playlist_pos = i;
			return;
		}
	}

	// strange error
	puts("Weird error");
	playlist.clear();
}

bool AutoPlaylist::getNextFile(string &filename)
{
	int last = playlist.size() - 1;
	if(playlist_pos == last) return false;
	playlist_pos++;
	filename = dirname + playlist[playlist_pos];
	return true;
}

bool AutoPlaylist::getPrevFile(string &filename)
{
	if(playlist_pos <= 0) return false;
	playlist_pos--;
	filename = dirname + playlist[playlist_pos];
	return true;
}

// c++ -std=c++0x -lpcre -o AutoPlaylist AutoPlaylist.cpp
