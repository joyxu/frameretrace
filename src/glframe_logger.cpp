// Copyright (C) Intel Corp.  2015.  All Rights Reserved.

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice (including the
// next paragraph) shall be included in all copies or substantial
// portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

//  **********************************************************************/
//  * Authors:
//  *   Mark Janes <mark.a.janes@intel.com>
//  **********************************************************************/

#include "glframe_logger.hpp"

#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#include <sstream>
#include <string>
#include <iostream>

#include "glframe_os.hpp"

using glretrace::Logger;
using glretrace::ScopedLock;
using glretrace::Thread;
using glretrace::Severity;

Logger *Logger::m_instance = NULL;

void
Logger::Create() {
  assert(!m_instance);
#ifdef WIN32
  char dir[MAX_PATH] = "";
  GetTempPath(MAX_PATH, dir);
#else
    const char *dir = "/tmp";
#endif
  std::stringstream ss;
  ss << dir << "/" << "frameretrace.log";
  m_instance = new Logger(ss.str());
}

void
Logger::Begin() {
  m_instance->Start();
}

void
Logger::Destroy() {
  m_instance->Stop();
  m_instance->Join();
  delete m_instance;
  m_instance = NULL;
}

Logger::Logger(const std::string &out_path) : Thread("logger"),
                                              m_severity(WARN),
                                              m_running(true),
                                              m_stderr(false) {
  std::stringstream ss;
  m_fh = fopen(out_path.c_str(), "a");
  assert(m_fh != NULL);
  m_read_fh = fopen(out_path.c_str(), "r");
  assert(m_read_fh != NULL);
}

Logger::~Logger() {
  fclose(m_fh);
  m_fh = NULL;
  fclose(m_read_fh);
  m_read_fh = NULL;
}

const char *
format_severity(glretrace::Severity s) {
  switch (s) {
    case glretrace::DEBUG:
      return "DEBUG";
    case glretrace::INFO:
      return "INFO";
    case glretrace::WARN:
      return "WARN";
    case glretrace::ERR:
      return "ERROR";
    default:
      return "UNKNOWN";
  }
}

void
Logger::EnableStderr() {
  m_instance->m_stderr = true;
}

void
Logger::Log(Severity s, const std::string &file, int line,
            const std::string &message) {
  assert(m_instance);
  if (s < m_instance->m_severity)
    return;
  time_t t = time(NULL);
  char ts[255];
  struct tm result;
  glretrace_localtime(&t, &result);
  strftime(ts, 255, "%Y %b %d %H:%M:%S", &result);
  std::stringstream ss;
  ss << "[" << ts << "] "
     << file << " " << line
     << " (" << format_severity(s) << "): "
     << message << "\n";
  ScopedLock sl(m_instance->m_protect);
  m_instance->m_q.push(ss.str());
  m_instance->m_sem.post();
  if (m_instance->m_stderr)
    std::cerr << ss.str();
}

void
Logger::SetSeverity(Severity s) {
  ScopedLock sl(m_instance->m_protect);
  m_instance->m_severity = s;
}

void
Logger::Flush() {
  assert(m_instance);
  while (true) {
    std::string front;
    {
      ScopedLock sl(m_instance->m_protect);
      if (m_instance->m_q.empty())
        break;
      front = m_instance->m_q.front();
      m_instance->m_q.pop();
    }
    fwrite(front.c_str(), 1, front.length(), m_instance->m_fh);
  }
  ScopedLock sl(m_instance->m_protect);
  fflush(m_instance->m_fh);
}

void
Logger::Run() {
  while (true) {
    m_sem.wait();
    Flush();
    if (!m_running)
      break;
  }
}

void
Logger::Stop() {
  ScopedLock sl(m_protect);
  m_running = false;
  m_sem.post();
}

void
Logger::GetLog(std::string *out) {
  ScopedLock sl(m_instance->m_protect);
  char buf[1024];
  size_t bytes = 1;
  while (bytes > 0) {
    bytes = fread(buf, 1, 1023, m_instance->m_read_fh);
    buf[bytes] = '\0';
    out->append(buf);
  }
}
