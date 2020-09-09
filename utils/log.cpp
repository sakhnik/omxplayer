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

#include <time.h>
#include <sys/time.h>
#include <cstring>

#include "log.h"
#include "utils/StdString.h"

static FILE*       m_stream         = NULL;
static bool        m_file_is_open   = false;
static int         m_repeatCount    = 0;
static int         m_repeatLogLevel = -1;
static std::string m_repeatLine;
static int         m_logLevel       = LOGNONE;

static pthread_mutex_t   m_log_mutex;

static char levelNames[][8] =
{"NONE", "FATAL", "SEVERE", "ERROR", "WARNING", "NOTICE", "INFO", "DEBUG"};


CLog::CLog()
{}

CLog::~CLog()
{}

void CLog::Close()
{
  if (m_file_is_open)
  {
    fclose(m_stream);
    m_file_is_open = false;
  }
  m_stream = NULL;
  m_repeatLine.clear();
  pthread_mutex_destroy(&m_log_mutex);
}

void CLog::Log(int loglevel, const char *format, ... )
{
  pthread_mutex_lock(&m_log_mutex);

  if (loglevel <= m_logLevel)
  {
    static const char* prefixFormat = "%02.2d:%02.2d:%02.2d T:%llu %7s: ";

    if (m_stream == NULL)
    {
      pthread_mutex_unlock(&m_log_mutex);
      return;
    }

    struct timeval now;
    gettimeofday(&now, NULL);
    struct tm *time = localtime( &now.tv_sec );
    uint64_t stamp = now.tv_usec + now.tv_sec * 1000000;
    CStdString strPrefix, strData;

    va_list va;
    va_start(va, format);
    strData.FormatV(format,va);
    va_end(va);

    if (m_repeatLogLevel == loglevel && m_repeatLine == strData)
    {
      m_repeatCount++;
      pthread_mutex_unlock(&m_log_mutex);
      return;
    }
    else if (m_repeatCount)
    {
      CStdString strData2;
      strPrefix.Format(prefixFormat, time->tm_hour, time->tm_min, time->tm_sec, stamp, levelNames[m_repeatLogLevel]);
      strData2.Format("Previous line repeats %d times.\n", m_repeatCount);

      fputs(strPrefix.c_str(), m_stream);
      fputs(strData2.c_str(), m_stream);

      m_repeatCount = 0;
    }
    
    m_repeatLine      = strData;
    m_repeatLogLevel  = loglevel;

    unsigned int length = 0;
    while ( length != strData.length() )
    {
      length = strData.length();
      strData.TrimRight(" ");
      strData.TrimRight('\n');
      strData.TrimRight("\r");
    }

    if (!length)
    {
      pthread_mutex_unlock(&m_log_mutex);
      return;
    }

    /* fixup newline alignment, number of spaces should equal prefix length */
    strData.Replace("\n", "\n                                            ");
    strData += "\n";

    strPrefix.Format(prefixFormat, time->tm_hour, time->tm_min, time->tm_sec, stamp, levelNames[loglevel]);

    fputs(strPrefix.c_str(), m_stream);
    fputs(strData.c_str(), m_stream);
    fflush(m_stream);
  }

  pthread_mutex_unlock(&m_log_mutex);
}

bool CLog::Init(int level, const char* path)
{
  pthread_mutex_init(&m_log_mutex, NULL);

  m_logLevel = level;

  if(m_logLevel == LOGNONE) return false;

  if(strcasecmp(path, "stdout") == 0)
  {
    m_file_is_open = false;
    m_stream = stdout;
  }
  else if(strcasecmp(path, "stderr") == 0)
  {
    m_file_is_open = false;
    m_stream = stderr;
  }
  else
  {
    m_stream = fopen(path, "w");
    m_file_is_open = m_stream != NULL;
  }

  return m_stream != NULL;
}

void CLog::MemDump(char *pData, int length)
{
  if (m_logLevel != LOGDEBUG || m_stream == NULL) return;

  Log(LOGDEBUG, "MEM_DUMP: Dumping from %p", pData);
  for (int i = 0; i < length; i+=16)
  {
    CStdString strLine;
    strLine.Format("MEM_DUMP: %04x ", i);
    char *alpha = pData;
    for (int k=0; k < 4 && i + 4*k < length; k++)
    {
      for (int j=0; j < 4 && i + 4*k + j < length; j++)
      {
        CStdString strFormat;
        strFormat.Format(" %02x", *pData++);
        strLine += strFormat;
      }
      strLine += " ";
    }
    // pad with spaces
    while (strLine.size() < 13*4 + 16)
      strLine += " ";
    for (int j=0; j < 16 && i + j < length; j++)
    {
      if (*alpha > 31)
        strLine += *alpha;
      else
        strLine += '.';
      alpha++;
    }
    Log(LOGDEBUG, strLine.c_str());
  }
}
