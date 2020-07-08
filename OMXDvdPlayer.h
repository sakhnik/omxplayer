#pragma once

#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>
#include <string>

class OMXDvdPlayer
{
  public:
	OMXDvdPlayer(std::string filename);
	~OMXDvdPlayer();

	void CloseTrack();
	bool OpenTrack(int ct);

	int Read(unsigned char *lpBuf, int64_t uiBufSize);
	int64_t Seek(int64_t iFilePosition, int iWhence);
	int64_t GetLength();
	int64_t getCurrentTrackLength();
	int TotalChapters();
	bool SeekChapter(int chapter);
	int GetChapter();
	int GetCurrentTrack() const { return current_track; }

  private:
	int dvdtime2msec(dvd_time_t *dt);
	void get_title_name();

	bool m_open = false;
	volatile int pos = 0;
	dvd_reader_t *dvd_device = NULL;
	dvd_file_t *dvd_track = NULL;

	int current_track;
	int total_blocks;

	struct dvd_info {
		std::string device;
		std::string disc_title;
		int title_count;
		struct title_info {
			int vts;
			float length;
			int chapter_count;
			int *chapters;
			int first_sector;
			int last_sector;
		} *titles;
	} dvd_info;

	double frames_per_s[4] = {-1.0, 25.00, -1.0, 29.97};

};
