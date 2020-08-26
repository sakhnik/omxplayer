#pragma once
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
#include <map>

using namespace std;

class RecentDVDStore
{
public:
	RecentDVDStore();
	void readStore();
	int setCurrentDVD(const string &key, int &track);
	void remember(int track, int time);
	void saveStore();

private:
	struct DVDInfo {
		int time;
		int track;
		int pos;
	};

	static bool DVDInfoCmp(pair<string, DVDInfo> const &a, pair<string, DVDInfo> const &b);

	map<string, DVDInfo> store;
	string recent_dvd_file;
	string current_dvd;
	bool m_init = false;
};
