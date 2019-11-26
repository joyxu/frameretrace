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

#ifndef TRACE_THREADPARSER_H_
#define TRACE_THREADPARSER_H_

#include <queue>
#include "trace_parser.hpp"

namespace glretrace {

class ThreadParser;
class ThreadedParser : public trace::AbstractParser {
 public:
  struct ParseRange {
    ParseRange(const trace::ParseBookmark &b,
               const trace::ParseBookmark &e) : begin(b), end(e) {}
    trace::ParseBookmark begin;
    trace::ParseBookmark end;
    bool operator<(const ParseRange &o);
    ParseRange(const ParseRange &o) : begin(o.begin), end(o.end) {}
  };
  ThreadedParser(int max_frame);
  ~ThreadedParser();

  // delegates to the current thread
  trace::Call *parse_call(void);

  // discards current threads, re-initializes
  void getBookmark(trace::ParseBookmark &bookmark);

  // 
  void setBookmark(const trace::ParseBookmark &bookmark);

  // scans file for frame boundaries
  bool open(const char *filename);
  void close(void);
  unsigned long long getVersion(void) const;
  const trace::Properties & getProperties(void) const;

 private:
  const int m_max_frame;
  trace::Parser m_parser;
  std::vector<ThreadParser*> m_threads;
  std::vector<ThreadParser*>::iterator m_current_thread;
  // might need to be a queue if we disallow bookmarks
  std::queue<ParseRange> m_frames;
};

}  // namespace retrace

#endif  // TRACE_THREADPARSER_H_
