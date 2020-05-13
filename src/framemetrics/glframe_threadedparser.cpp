// Copyright (C) Intel Corp.  2019.  All Rights Reserved.

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

#include "glframe_threadedparser.hpp"
#include "glframe_thread.hpp"
#include "glframe_os.hpp"
#include "glframe_retrace_render.hpp"

using glretrace::ThreadParser;
using glretrace::Semaphore;
using trace::Call;
using trace::Parser;
using trace::ParseBookmark;

namespace glretrace {
class ThreadParser : public Thread {
 public:
  ThreadParser(const char *filename, int max_frame);
  void parse_range(const ThreadedParser::ParseRange &range);
  Call *parse_call(void);
  void clear();
  void Run();
  void stop();
  void wait_for_parse() { m_consumer_sem.wait(); }

  ~ThreadParser();
 private:
  const int m_max_frame;
  std::vector<Call *> m_parsed_calls;
  std::vector<Call *>::iterator m_current_call;
  Parser m_parser;
  ThreadedParser::ParseRange m_range;
  std::mutex m_mutex;
  Semaphore m_reader_sem, m_consumer_sem;
  bool m_running;
};
}

using glretrace::ThreadedParser;

ThreadedParser::ThreadedParser(int max_frame) : m_max_frame(max_frame) {}
ThreadedParser::~ThreadedParser() {
}

// delegates to the current thread
Call *
ThreadedParser::parse_call(void) {
  Call *c = (*m_current_thread)->parse_call();
  if (c)
    return c;

  // else current thread has provided all available calls

  if (m_frames.empty())
    // EOF, will drop a few frames at the end
    return NULL;

  // start the current thread on the proximal frame
  (*m_current_thread)->parse_range(m_frames.front());
  m_frames.pop();

  // switch to the next thread
  if (++m_current_thread == m_threads.end())
    m_current_thread = m_threads.begin();

  (*m_current_thread)->wait_for_parse();
  return (*m_current_thread)->parse_call();
}

void
ThreadedParser::getBookmark(trace::ParseBookmark &bookmark) {
  assert(false);
}

  // 
void
ThreadedParser::setBookmark(const trace::ParseBookmark &bookmark) {
  assert(false);
  // for (auto t : m_threads)
  //   t->clear();
  // auto frame = m_frames.find(ThreadedParser::ParseRange(bookmark, bookmark));
  // ThreadedParser::ParseRange r(bookmark, frame->first.end);
  // for (auto t : m_threads) {
  //   t->parse_range(r);
  //   // TODO handle EOF
  //   ++frame;
  //   r = frame->first;
  // }
}

// scans file for frame boundaries
bool
ThreadedParser::open(const char *filename) {
  if (!m_parser.open(filename))
    return false;
  ParseBookmark begin, end;
  m_parser.getBookmark(begin);
  std::cout << "scanning for frames\n";
  while (auto call = m_parser.scan_call()) {
    if (RetraceRender::endsFrame(*call)) {
      m_parser.getBookmark(end);
      m_frames.emplace(ParseRange(begin, end));
      if (0 == (m_frames.size() % 100))
        std::cout << "frame: " << m_frames.size() << std::endl;
      begin = end;
      if (m_frames.size() > m_max_frame)
        break;
    }
    delete call;
  }
  for (int i = 0; i < 4; ++i) {
    ThreadParser *p = new ThreadParser(filename, m_max_frame);
    m_threads.push_back(p);
    p->Start();
    p->parse_range(m_frames.front());
    m_frames.pop();
  }
  m_current_thread = m_threads.begin();
  (*m_current_thread)->wait_for_parse();
  return true;
}

void
ThreadedParser::close(void) {
    for (auto t : m_threads) {
    t->stop();
    t->Join();
    delete t;
  }
  m_threads.clear();
}

unsigned long long
ThreadedParser::getVersion(void) const { return m_parser.getVersion(); }

const trace::Properties &
ThreadedParser::getProperties(void) const { return m_parser.getProperties(); }

ThreadParser::ThreadParser(const char *filename, int max_frame)
    : Thread("threaded parser"),
      m_max_frame(max_frame),
      m_range(ParseBookmark(), ParseBookmark()),
      m_running(false) {
  m_parser.open(filename);
  m_current_call = m_parsed_calls.begin();
}

void
ThreadParser::parse_range(const ThreadedParser::ParseRange &range) {
  std::lock_guard<std::mutex> l(m_mutex);
  assert(m_current_call == m_parsed_calls.end());
  m_parsed_calls.clear();
  m_range = range;
  m_reader_sem.post();
}

Call *
ThreadParser::parse_call(void) {
  std::lock_guard<std::mutex> l(m_mutex);
  if (m_current_call == m_parsed_calls.end())
    return NULL;
  Call *c = *m_current_call;
  ++m_current_call;
  return c;
}

void
ThreadParser::clear() {
  assert(false);
}
void
ThreadParser::Run() {
  m_running = true;
  int frame = 0;
  while (auto call = m_parser.scan_call()) {
    if (RetraceRender::endsFrame(*call)) {
      if (frame++ > m_max_frame)
        break;
    }
    delete call;
  }

  while(m_running) {
    {
      m_reader_sem.wait();
      std::lock_guard<std::mutex> l(m_mutex);
      if (!m_running)
        return;
      m_parser.setBookmark(m_range.begin);
      int call_id = m_range.begin.next_call_no;
      const int next_frame_call_id = m_range.end.next_call_no;
      while(call_id != next_frame_call_id) {
        ++call_id;
        m_parsed_calls.push_back(m_parser.parse_call());
      }
      m_current_call = m_parsed_calls.begin();
      m_consumer_sem.post();
    }
  }
}
void
ThreadParser::stop() {
  m_running = false;
  m_reader_sem.post();
}

ThreadParser::~ThreadParser() {
}
