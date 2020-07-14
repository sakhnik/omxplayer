#pragma once

#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>
#include <string>

struct OMXStream;

class OMXDvdPlayer
{
  public:
	bool Open(std::string filename);
	~OMXDvdPlayer();

	void CloseTrack();
	bool ChangeTrack(int delta, int &t);
	bool OpenTrack(int ct);

	int Read(unsigned char *lpBuf, int64_t uiBufSize);
	int64_t Seek(int64_t iFilePosition, int iWhence);
	int64_t GetLength();
	int64_t getCurrentTrackLength();
	int TotalChapters();
	bool SeekChapter(int chapter);
	int GetChapter();
	int GetCurrentTrack() const { return current_track; }
	void GetStreamInfo(OMXStream *stream);
	bool MetaDataCheck(int audiostream_count, int subtitle_count);
	std::string GetDVDID() const { return dvd_info.disc_checksum; }
	void enableHeuristicTrackSelection();
	int findNextEnabledTrack(int i);
	int findPrevEnabledTrack(int i);

  private:
	int dvdtime2msec(dvd_time_t *dt);
	void get_title_name();
	void get_disc_id();

	bool m_open = false;
	bool m_allocated = false;
	volatile int pos = 0;
	dvd_reader_t *dvd_device = NULL;
	dvd_file_t *dvd_track = NULL;

	int current_track;
	int total_blocks;

	struct dvd_info {
		std::string device;
		std::string disc_title;
		std::string disc_checksum;
		int title_count;
		struct title_info {
		    bool enabled;
			int vts;
			float length;
			int chapter_count;
			int *chapters;
			int audiostream_count;
			int subtitle_count;
			struct stream_info {
				int index;
				int id;
				uint16_t lang;
			} *streams;
			int first_sector;
			int last_sector;
		} *titles;
	} dvd_info;

	double frames_per_s[4] = {-1.0, 25.00, -1.0, 29.97};

};
