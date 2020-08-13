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
#include <map>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "RecentDVDStore.h"

using namespace std;

RecentDVDStore::RecentDVDStore()
{
	// recent DVD file store
	recent_dvd_file = getenv("HOME");
	recent_dvd_file += "/.omxplayer_dvd_store";
}

void RecentDVDStore::readStore()
{
	m_init = true;

	ifstream s(recent_dvd_file);

	if(!s.is_open()) return;

	string line;
	int pos = 0;
	while(getline(s, line)) { 
	   DVDInfo d;
	   string key;
	   
	   istringstream is(line);
	   if(is >> key >> d.track >> d.time) {
		   d.pos = pos++;
		   store[key] = move(d);
	   }
	}
	s.close();
}

int RecentDVDStore::setCurrentDVD(string key, int &track)
{
	current_dvd = key;
	int time = 0;

	if(store.find(current_dvd) == store.end()) {
		return 0;
	}

	if(track == -1) {
		track = store[current_dvd].track;
		time = store[current_dvd].time;
	} else if(track == store[current_dvd].track) {
		time = store[current_dvd].time;
	}

	store.erase(current_dvd);
	return time;
}


void RecentDVDStore::remember(int track, int time)
{
	store[current_dvd] = {time, track, -1};
}

bool RecentDVDStore::DVDInfoCmp(pair<string, DVDInfo> const &a, pair<string, DVDInfo> const &b)
{
	return a.second.pos < b.second.pos;
}

void RecentDVDStore::saveStore()
{
	if(!m_init) return;

	// create a sorted vector
	vector<pair<string, DVDInfo> > vector_store;
	vector_store.assign(store.begin(), store.end());
	sort(vector_store.begin(), vector_store.end(), DVDInfoCmp);

	int size = vector_store.size();
	if(size > 20) size = 20; // to to twenty files
	
	ofstream s(recent_dvd_file);
	
	for(int i = 0; i < size; i++) {
		s << vector_store[i].first << '\t';
		s << vector_store[i].second.track << '\t';
		s << vector_store[i].second.time << "\n";
	}
	
	s.close();
}
