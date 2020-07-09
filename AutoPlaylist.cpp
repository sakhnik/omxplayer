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
	vector<pair<string,string>> filelist;

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
	CRegExp fnameext_match = CRegExp(true);
	fnameext_match.RegComp("\\.(3g2|3gp|amv|asf|avi|drc|f4a|f4b|f4p|f4v|flv|"
		"m2ts|m2v|m4p|m4v|mkv|mov|mp2|mp4|mpe|mpeg|mpg|mpv|mts|mxf|nsv|ogg|"
		"ogv|qt|rm|rmvb|roq|svi|ts|vob|webm|wmv|yuv|iso|dmg)$");

	while ((ent = readdir(dir)) != NULL) {
		if(ent->d_type != 4 && ent->d_name[0] != '.' &&
				fnameext_match.RegFind(ent->d_name, 0) > -1) {
			string a = ent->d_name;
			string b = a;

			// lowercase
			transform(b.begin(), b.end(), b.begin(),
				[](unsigned char c){ return tolower(c); });

			filelist.emplace_back(move(b), move(a));
		}
	}

	// sort by lower case
	sort(filelist.begin(), filelist.end());

	// shift second part of pair to playlist
	playlist.resize(filelist.size());
	int len = playlist.size();
	for(int i = 0; i < len; i++) {
		playlist[i] = move(filelist[i].second);
	}

	// search for file we started with
	for(int i = 0; i < len; i++) {
		if(playlist[i] == basename) {
			playlist_pos = i;
			return;
		}
	}

	// strange error
	puts("Weird error");
	playlist.clear();
}

bool AutoPlaylist::ChangeFile(int delta, string &filename)
{
	int npos = playlist_pos + delta;
	int last_index = playlist.size() - 1;

	if(npos < 0 || npos > last_index)
		return false;

	playlist_pos = npos;

	filename = dirname + playlist[playlist_pos];
	return true;
}
