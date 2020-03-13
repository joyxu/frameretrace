/**************************************************************************
 *
 * Copyright 2016 Intel Corporation
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * Authors:
 *   Mark Janes <mark.a.janes@intel.com>
 **************************************************************************/

#ifndef _GLFRAME_RUNNER_HPP_
#define _GLFRAME_RUNNER_HPP_

#include <map>
#include <string>
#include <vector>

#include <fstream> // NOLINT

#include "glframe_thread_context.hpp"
#include "glframe_threadedparser.hpp"

namespace trace {
class Call;
}

class PerfMetricGroup;

namespace glretrace {

class Context;

/* Describes a requested PerfMetricGroup, with an optional list of
 * requested metrics within the group.  If no individual metrics
 * are requested, all metrics within the group will be used.  This
 * is not necessarily supported on all hw, in partcular with
 * GL_AMD_performance_monitor.
 */
class PerfMetricDescriptor {
public:
  std::string m_metrics_group;
  std::vector<std::string> m_metrics_names;

  /* Takes a PerfMetricGroup descriptor string, which can either
   * be just a group name, or "group:counter1,counter2,counter3"
   */
  PerfMetricDescriptor(const char *desc);
};

class FrameRunner {
 public:
  enum MetricInterval {
    kPerRender,
    kPerFrame,
    kPerTrace
  };

  FrameRunner(const std::string filepath,
              const std::string out_path,
              PerfMetricDescriptor metrics_desc,
              int max_frame,
              MetricInterval interval = kPerFrame,
              int event_interval = 1);
  ~FrameRunner();
  void advanceToFrame(int f);
  void dumpGroupsAndCounters();
  void init();
  void run(int end_frame);

 private:
  std::ofstream m_of;
  std::ostream *m_out;
  int m_current_frame, m_current_event, m_group_id;
  const MetricInterval m_interval;
  const int m_event_interval;
  PerfMetricDescriptor m_metrics_desc;
  PerfMetricGroup *m_current_group;
  std::map<Context *, PerfMetricGroup *> m_context_metrics;
  std::map<Context *, trace::Call*> m_context_calls;
  std::map<unsigned long long, Context *> m_retraced_contexts;
  ThreadedParser m_parser;
};

}  // namespace glretrace

#endif

