#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include <stdio.h>
#include <string>

#define LOGNONE    0
#define LOGFATAL   1
#define LOGSEVERE  2
#define LOGERROR   3
#define LOGWARNING 4
#define LOGNOTICE  5
#define LOGINFO    6
#define LOGDEBUG   7


#ifdef __GNUC__
#define ATTRIB_LOG_FORMAT __attribute__((format(printf,2,3)))
#else
#define ATTRIB_LOG_FORMAT
#endif

class CLog
{
public:
  CLog();
  virtual ~CLog(void);
  static void Close();
  static void Log(int loglevel, const char *format, ... ) ATTRIB_LOG_FORMAT;
  static void MemDump(char *pData, int length);
  static bool Init(int level, const char* path);
  static void SetLogLevel(int level);
};
