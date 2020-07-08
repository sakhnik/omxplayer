/*
 * Copyright (C) 2020 by Michael J. Walsh
 *
 * Much of this file is a slimmed down version of lsdvd by Chris Phillips
 * and Henk Vergonet.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with libdvdnav; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#include "OMXDvdPlayer.h"
#include <string.h>
#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>


OMXDvdPlayer::OMXDvdPlayer(std::string filename)
{
	// Open DVD device or file
	dvd_device = DVDOpen(filename.c_str());
	if(!dvd_device) {
		puts("Error on DVDOpen");
		return;
	}

	ifo_handle_t *ifo_zero, **ifo;

	// open dvd meta data header
	ifo_zero = ifoOpen(dvd_device, 0);
	if ( !ifo_zero ) {
		fprintf( stderr, "Can't open main ifo!\n");
		return;
	}

	ifo = (ifo_handle_t **)malloc((ifo_zero->vts_atrt->nr_of_vtss + 1) * sizeof(ifo_handle_t *));

	for (int i=1; i <= ifo_zero->vts_atrt->nr_of_vtss; i++) {
		ifo[i] = ifoOpen(dvd_device, i);
		if ( !ifo[i] && i == 0 ) {
			fprintf( stderr, "Can't open ifo %d!\n", i);
			return;
		}
	}

	int titles = ifo_zero->tt_srpt->nr_of_srpts;

	dvd_info.device = filename;
	get_title_name();

	dvd_info.title_count = titles;
	dvd_info.titles = (dvd_info::title_info *)calloc(titles, sizeof(*dvd_info.titles));

	for (int j=0, h=0; j < titles; j++, h++) {
		int title_set_nr = ifo_zero->tt_srpt->title[j].title_set_nr;
		int vts_ttn = ifo_zero->tt_srpt->title[j].vts_ttn;

		if (!ifo[title_set_nr]->vtsi_mat) {
			h--;
			dvd_info.title_count--;
			continue;
		}

		pgcit_t *vts_pgcit;
		pgc_t *pgc;
		vts_pgcit  = ifo[title_set_nr]->vts_pgcit;
		pgc = vts_pgcit->pgci_srp[ifo[title_set_nr]->vts_ptt_srpt->title[vts_ttn - 1].ptt[0].pgcn - 1].pgc;

		if(pgc->cell_playback == NULL || pgc->program_map == NULL) {
			h--;
			dvd_info.title_count--;
			continue;
		}

		dvd_info.titles[h].vts = title_set_nr;
		dvd_info.titles[h].length = dvdtime2msec(&pgc->playback_time)/1000.0;
		dvd_info.titles[h].chapter_count = pgc->nr_of_programs;
		int cell_count = pgc->nr_of_cells;

		/* CELLS */
		// ignore non-contiguous ends of tracks
		dvd_info.titles[h].first_sector = pgc->cell_playback[0].first_sector;
		int last_sector = -1;

		for (int i = 0; i < cell_count - 1; i++) {
			int end = pgc->cell_playback[i].last_sector;
			int next = pgc->cell_playback[i+1].first_sector - 1;
			if(end != next) {
				last_sector = end;
				float missing_time = 0;
				for (i++; i < cell_count; i++){
					missing_time += dvdtime2msec(&pgc->cell_playback[i].playback_time)/1000.0;
				}
				dvd_info.titles[h].length -= missing_time;
				break;
			}
		}
		if(last_sector == -1) {
			int last = cell_count - 1;
			last_sector = pgc->cell_playback[last].last_sector;
		}
		dvd_info.titles[h].last_sector = last_sector;

		/* CHAPTERS */
		dvd_info.titles[h].chapters = (int *)calloc(dvd_info.titles[h].chapter_count, sizeof(*dvd_info.titles[h].chapters));

		for (int i=0; i<dvd_info.titles[h].chapter_count; i++) {
			int idx = pgc->program_map[i] - 1;
			int p = pgc->cell_playback[idx].first_sector;
			if(p > last_sector) {
				dvd_info.titles[h].chapter_count = i;
				break;
			}
			dvd_info.titles[h].chapters[i] = p - dvd_info.titles[h].first_sector;
		}
	}

	// close dvd meta data
	for (int i=1; i <= ifo_zero->vts_atrt->nr_of_vtss; i++) ifoClose(ifo[i]);
	free(ifo);
	ifoClose(ifo_zero);
}

bool OMXDvdPlayer::OpenTrack(int ct)
{
	if(m_open == true)
		CloseTrack();

	if(ct > dvd_info.title_count - 1)
		return false;

	// select track
	current_track = ct;

	// blocks for this track
	total_blocks = dvd_info.titles[current_track].last_sector - dvd_info.titles[current_track].first_sector + 1;

	// open dvd track
	dvd_track = DVDOpenFile(dvd_device, dvd_info.titles[current_track].vts, DVD_READ_TITLE_VOBS );

	if(!dvd_track) {
		puts("Error on DVDOpenFile");
		return false;
	}

	m_open = true;
	return true;
}

int OMXDvdPlayer::Read(unsigned char *lpBuf, int64_t uiBufSize)
{
	if(!m_open)
		return 0;

	// read in block in whole numbers
	int blocks_to_read = uiBufSize / 2048;

	if(pos + blocks_to_read > total_blocks) {
		blocks_to_read = total_blocks - pos;

		if(blocks_to_read < 1)
			return 0;
	}

	int read_blocks = DVDReadBlocks(dvd_track, dvd_info.titles[current_track].first_sector + pos, blocks_to_read, lpBuf);
	pos += read_blocks;
	return read_blocks * 2048;
}

int64_t OMXDvdPlayer::getCurrentTrackLength()
{
	return (int64_t)(dvd_info.titles[current_track].length * 1000000);
}

int64_t OMXDvdPlayer::Seek(int64_t iFilePosition, int iWhence)
{
	if (!m_open)
		return -1;

	// seek in blocks
	int seek_size = iFilePosition / 2048;

	switch (iWhence) {
		case SEEK_SET:
			pos = seek_size;
			break;
		case SEEK_CUR:
			pos += seek_size;
			break;
		case SEEK_END:
			pos = total_blocks - seek_size;
			break;
		default:
			return -1;
	}

	return 0;
}

int64_t OMXDvdPlayer::GetLength()
{
	int64_t len = dvd_info.titles[current_track].last_sector - dvd_info.titles[current_track].first_sector + 1;
	len *= 2048;
	return len;
}

int OMXDvdPlayer::TotalChapters()
{
	return dvd_info.titles[current_track].chapter_count;
}

bool OMXDvdPlayer::SeekChapter(int chapter)
{
	// seeking next chapter from last causes eof
	if(chapter > dvd_info.titles[current_track].chapter_count) {
		pos = total_blocks;
		return false;
	} else {
		pos = dvd_info.titles[current_track].chapters[chapter-1];
		return true;
	}
}

int OMXDvdPlayer::GetChapter()
{
	int cpos = pos;
	int i;
	for (i=0; i<dvd_info.titles[current_track].chapter_count-1; i++) {
		if(cpos >= dvd_info.titles[current_track].chapters[i] &&
				cpos < dvd_info.titles[current_track].chapters[i+1])
			return i + 1;
	}
	return i + 1;
}

void OMXDvdPlayer::CloseTrack()
{
	DVDCloseFile(dvd_track);
	m_open = false;
}

OMXDvdPlayer::~OMXDvdPlayer()
{
	if(m_open == true)
		CloseTrack();

	for (int i=0; i < dvd_info.title_count; i++)
		free(dvd_info.titles[i].chapters);
	free(dvd_info.titles);

	DVDClose(dvd_device);
}

int OMXDvdPlayer::dvdtime2msec(dvd_time_t *dt)
{
	double fps = frames_per_s[(dt->frame_u & 0xc0) >> 6];
	long   ms;
	ms  = (((dt->hour &   0xf0) >> 3) * 5 + (dt->hour   & 0x0f)) * 3600000;
	ms += (((dt->minute & 0xf0) >> 3) * 5 + (dt->minute & 0x0f)) * 60000;
	ms += (((dt->second & 0xf0) >> 3) * 5 + (dt->second & 0x0f)) * 1000;

	if(fps > 0)
	ms += (((dt->frame_u & 0x30) >> 3) * 5 + (dt->frame_u & 0x0f)) * 1000.0 / fps;

	return ms;
}

/*
 *  The following method is based on code from vobcopy, by Robos, with thanks.
 */
void OMXDvdPlayer::get_title_name()
{
	FILE *filehandle = 0;
	char title[33];
	int  i;

	if (! (filehandle = fopen(dvd_info.device.c_str(), "r"))) {
		dvd_info.disc_title = "unknown";
		return;
	}

	if ( fseek(filehandle, 32808, SEEK_SET ) || 32 != (i = fread(title, 1, 32, filehandle)) ) {
		fclose(filehandle);
		dvd_info.disc_title = "unknown";
		return;
	}

	fclose (filehandle);

	title[32] = '\0';
	while(i-- > 2)
		if(title[i] == ' ') title[i] = '\0';

	dvd_info.disc_title = title;
}
