/*
 * Copyright (C) 2020 by Michael J. Walsh
 *
 * Much of this file is a slimmed down version of lsdvd by Chris Phillips
 * and Henk Vergonet, and Martin Thierer's fork.
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

#include <string.h>
#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>

#include "OMXReader.h"
#include "OMXDvdPlayer.h"

bool OMXDvdPlayer::Open(const std::string &filename)
{
	static int audio_id[7] = {0x80, 0, 0xC0, 0xC0, 0xA0, 0, 0x88};
	device_path = filename;

	// Open DVD device or file
	dvd_device = DVDOpen(device_path.c_str());
	if(!dvd_device) {
		puts("Error on DVDOpen");
		return false;
	}

	// Get device name and checksum
	get_title_name();
	get_disc_checksum();

	// Open dvd meta data header
	ifo_handle_t *ifo_zero, **ifo;
	ifo_zero = ifoOpen(dvd_device, 0);
	if ( !ifo_zero ) {
		fprintf( stderr, "Can't open main ifo!\n");
		return false;
	}

	ifo = (ifo_handle_t **)malloc((ifo_zero->vts_atrt->nr_of_vtss + 1) * sizeof(ifo_handle_t *));
	for (int i=1; i <= ifo_zero->vts_atrt->nr_of_vtss; i++) {
		ifo[i] = ifoOpen(dvd_device, i);
		if ( !ifo[i] && i == 0 ) {
			fprintf( stderr, "Can't open ifo %d!\n", i);
			return false;
		}
	}

	// loop through title sets
	title_count = ifo_zero->tt_srpt->nr_of_srpts;
	titles = (title_info *)calloc(title_count, sizeof(*titles));
	for (int j=0, h=0; j < title_count; j++, h++) {
		int title_set_nr = ifo_zero->tt_srpt->title[j].title_set_nr;
		int vts_ttn = ifo_zero->tt_srpt->title[j].vts_ttn;

		if (!ifo[title_set_nr]->vtsi_mat) {
			h--;
			title_count--;
			continue;
		}

		pgcit_t    *vts_pgcit  = ifo[title_set_nr]->vts_pgcit;
		vtsi_mat_t *vtsi_mat   = ifo[title_set_nr]->vtsi_mat;
		pgc_t      *pgc        = vts_pgcit->pgci_srp[ifo[title_set_nr]->vts_ptt_srpt->title[vts_ttn - 1].ptt[0].pgcn - 1].pgc;

		if(pgc->cell_playback == NULL || pgc->program_map == NULL) {
			h--;
			title_count--;
			continue;
		}

		titles[h].enabled = true;
		titles[h].vts = title_set_nr;
		titles[h].length = dvdtime2msec(&pgc->playback_time)/1000.0;
		titles[h].chapter_count = pgc->nr_of_programs;
		int cell_count = pgc->nr_of_cells;

		// Tracks
		// ignore non-contiguous ends of tracks
		titles[h].first_sector = pgc->cell_playback[0].first_sector;
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
				titles[h].length -= missing_time;
				break;
			}
		}
		if(last_sector == -1) {
			int last = cell_count - 1;
			last_sector = pgc->cell_playback[last].last_sector;
		}
		titles[h].last_sector = last_sector;

		// Chapters
		titles[h].chapters = (int *)calloc(titles[h].chapter_count, sizeof(*titles[h].chapters));

		for (int i=0; i<titles[h].chapter_count; i++) {
			int idx = pgc->program_map[i] - 1;
			int p = pgc->cell_playback[idx].first_sector;
			if(p > last_sector) {
				titles[h].chapter_count = i;
				break;
			}
			titles[h].chapters[i] = p - titles[h].first_sector;
		}

		// Streams
		titles[h].audiostream_count = 0;
		titles[h].subtitle_count = 0;
		for (int k = 0; k < 8; k++)
			if (pgc->audio_control[k] & 0x8000)
				titles[h].audiostream_count++;

		for (int k = 0; k < 32; k++)
			if (pgc->subp_control[k] & 0x80000000)
				titles[h].subtitle_count++;

		titles[h].streams = (title_info::stream_info *)calloc(
				titles[h].audiostream_count + titles[h].subtitle_count,
				sizeof(*titles[h].streams));
		int stream_index = 0;

		// Audio streams
		for (int i=0, k=0; i<8; i++) {
			if ((pgc->audio_control[i] & 0x8000) == 0) continue;

			audio_attr_t *audio_attr = &vtsi_mat->vts_audio_attr[i];
			titles[h].streams[stream_index++] = {
				k++,
				audio_id[audio_attr->audio_format] + (pgc->audio_control[i] >> 8 & 7),
				audio_attr->lang_code
			};
		}

		// Subtitles
		int x = vtsi_mat->vts_video_attr.display_aspect_ratio == 0 ? 24 : 8;
		for (int i=0, k=0; i<32; i++) {
			if ((pgc->subp_control[i] & 0x80000000) == 0) continue;

			subp_attr_t *subp_attr = &vtsi_mat->vts_subp_attr[i];
			titles[h].streams[stream_index++] = {
				k++,
				(int)((pgc->subp_control[i] >> x) & 0x1f) + 0x20,
				subp_attr->lang_code,
			};
		}
	}

	// close dvd meta data filehandles
	for (int i=1; i <= ifo_zero->vts_atrt->nr_of_vtss; i++) ifoClose(ifo[i]);
	free(ifo);
	ifoClose(ifo_zero);
	m_allocated = true;
	puts("Finished parsing DVD meta data");
	return true;
}

bool OMXDvdPlayer::ChangeTrack(int delta, int &t)
{
	int ct;
	if(delta == -1)
		ct = findPrevEnabledTrack(t);
	else
		ct = findNextEnabledTrack(t);

	if(ct == -1)
		return false;

	bool r = OpenTrack(ct);
	t = current_track;
	return r;
}

bool OMXDvdPlayer::OpenTrack(int ct)
{
	if(m_open == true)
		CloseTrack();

	if(ct < 0 || ct > title_count - 1)
		return false;

	// select track
	current_track = ct;

	// seek to beginning to track
	pos = 0;

	// blocks for this track
	total_blocks = titles[current_track].last_sector - titles[current_track].first_sector + 1;

	// open dvd track
	dvd_track = DVDOpenFile(dvd_device, titles[current_track].vts, DVD_READ_TITLE_VOBS );

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

	int read_blocks = DVDReadBlocks(dvd_track, titles[current_track].first_sector + pos, blocks_to_read, lpBuf);
	pos += read_blocks;
	return read_blocks * 2048;
}

int64_t OMXDvdPlayer::getCurrentTrackLength()
{
	return (int64_t)(titles[current_track].length * 1000000);
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
	return (int64_t)total_blocks * 2048;
}

int OMXDvdPlayer::TotalChapters()
{
	return titles[current_track].chapter_count;
}

bool OMXDvdPlayer::SeekChapter(int chapter)
{
	// seeking next chapter from last causes eof
	if(chapter > titles[current_track].chapter_count) {
		pos = total_blocks;
		return false;
	} else {
		pos = titles[current_track].chapters[chapter-1];
		return true;
	}
}

int OMXDvdPlayer::GetChapter()
{
	int cpos = pos;
	int i;
	for (i=0; i<titles[current_track].chapter_count-1; i++) {
		if(cpos >= titles[current_track].chapters[i] &&
				cpos < titles[current_track].chapters[i+1])
			return i + 1;
	}
	return i + 1;
}

void OMXDvdPlayer::CloseTrack()
{
	DVDCloseFile(dvd_track);
	m_open = false;
}

// A sanity check. Before copying over the meta data, make sure the number of streams found
// in the vob file correspond to streams listed in the DVD meta data
bool OMXDvdPlayer::MetaDataCheck(int audiostream_count, int subtitle_count)
{
	return titles[current_track].audiostream_count == audiostream_count &&
		titles[current_track].subtitle_count == subtitle_count;
}

void OMXDvdPlayer::GetStreamInfo(OMXStream *stream)
{
	int len = titles[current_track].audiostream_count + titles[current_track].subtitle_count;

	for (int i=0; i < len; i++) {
		if(titles[current_track].streams[i].id == stream->stream->id) {
			stream->index = titles[current_track].streams[i].index;

			uint16_t lc = titles[current_track].streams[i].lang;
			sprintf(stream->language, "%c%c", lc >> 8, lc & 0xff);
			return;
		}
	}

	// fail
	stream->index = -1;
}

OMXDvdPlayer::~OMXDvdPlayer()
{
	if(m_open == true)
		CloseTrack();

	if(m_allocated) {
		for (int i=0; i < title_count; i++)
			free(titles[i].chapters);
		free(titles);
	}

	if(dvd_device)
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

	if (! (filehandle = fopen(device_path.c_str(), "r"))) {
		disc_title = "unknown";
		return;
	}

	if ( fseek(filehandle, 32808, SEEK_SET ) || 32 != (i = fread(title, 1, 32, filehandle)) ) {
		fclose(filehandle);
		disc_title = "unknown";
		return;
	}

	fclose (filehandle);

	title[32] = '\0';
	while(i-- > 2)
		if(title[i] == ' ') title[i] = '\0';

	disc_title = title;
}

void OMXDvdPlayer::get_disc_checksum()
{
	unsigned char buf[16];
	if (DVDDiscID(dvd_device, buf) == -1) return;

	char hex[33];
	for (int i = 0; i < 16; i++)
		sprintf(hex + 2 * i, "%02x", buf[i]);

	disc_checksum = hex;
}

//enable heuristic track skip
void OMXDvdPlayer::enableHeuristicTrackSelection()
{
	// Disable tracks which are shorter than two minutes
	for(int i = 0; i < title_count; i++) {
		if(titles[i].length < 120) {
			titles[i].enabled = false;
		}
	}

	// Search for and disable composite tracks
	for(int i = 0; i < title_count - 1; i++) {
		for(int j = i + 1; j < title_count; j++) {
			if(titles[i].vts == titles[j].vts
					&& titles[i].first_sector == titles[j].first_sector
					&& titles[i].enabled && titles[j].enabled) {

				if(titles[i].length > titles[j].length)
					titles[i].enabled = false;
				else
					titles[j].enabled = false;
			}
		}
	}
}

int OMXDvdPlayer::findNextEnabledTrack(int i)
{
	for(i++; i < title_count; i++) {
		if(titles[i].enabled)
			return i;
		else printf("Skipping Track %d\n", i+1);
	}
	return -1;
}

int OMXDvdPlayer::findPrevEnabledTrack(int i)
{
	for(i--; i > -1; i--) {
		if(titles[i].enabled)
			return i;
		else printf("Skipping Track %d\n", i+1);
	}
	return -1;
}
