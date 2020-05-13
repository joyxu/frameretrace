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

#ifndef _METRICS_HPP_
#define _METRICS_HPP_

#include <map>
#include <string>
#include <vector>

#include <fstream> // NOLINT

#include "glframe_traits.hpp"


namespace metrics {

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

class PerfValue {
 public:
  int handle;
  unsigned frame;
  unsigned number;
  int prog;         /* current program, for kPerRender mode */
  std::string event_type;
  PerfValue(int _handle, unsigned _frame,
      unsigned _number, int _prog)
      : handle(_handle), frame(_frame), number(_number), prog(_prog),
        event_type("") {}
};

class PerfMetricGroup {
 public:
  virtual ~PerfMetricGroup() {}
  virtual const std::string &name() const = 0;
  virtual void get_metric_names(std::vector<std::string> *out_names) = 0;
};

/**
 * Encapsulates one or more PerfMetricGroups
 */
class PerfMetrics {
 public:
  virtual ~PerfMetrics() {}
  virtual void get_metric_groups(std::vector<PerfMetricGroup *> *out_groups) = 0;
  virtual void begin(unsigned current_frame, unsigned event_number, int prog) = 0;
  virtual void end(const std::string &event_type) = 0;
  virtual void publish(std::ostream *outf, bool wait) = 0;
};

}  // namespace metrics

metrics::PerfMetrics *create_intel_metrics(std::vector<metrics::PerfMetricDescriptor> metrics_descs);
void dump_intel_metrics(void);

metrics::PerfMetrics *create_amd_metrics(std::vector<metrics::PerfMetricDescriptor> metrics_descs);
void dump_amd_metrics(void);

#endif

