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

#include "glframe_runner.hpp"

#include <GL/gl.h>

#include <map>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

#include "glframe_glhelper.hpp"
#include "glframe_retrace_render.hpp"
#include "glretrace.hpp"
#include "retrace.hpp"
#include "glframe_traits.hpp"
#include "glframe_logger.hpp"
#include "glframe_os.hpp"

#include "glframe_glhelper.hpp"
#include "glframe_traits.hpp"
#include "glframe_logger.hpp"
#include "metrics.hpp"

using glretrace::NoCopy;
using glretrace::NoAssign;
using glretrace::GlFunctions;
using metrics::PerfValue;
using metrics::PerfMetricGroup;
using metrics::PerfMetrics;


static void
get_group_ids(std::vector<unsigned int> *ids) {
  int num_groups;
  GlFunctions::GetPerfMonitorGroupsAMD(&num_groups, 0, NULL);
  assert(!GlFunctions::GetError());
  assert(num_groups > 0);
  ids->resize(num_groups);
  GlFunctions::GetPerfMonitorGroupsAMD(&num_groups, num_groups,
      ids->data());
  assert(!GlFunctions::GetError());
  return;
}


class AMDPerfMetric : public NoCopy, NoAssign {
 public:
  AMDPerfMetric(int group_id, int counter_num);
  const std::string &name() const { return m_name; }
  void getMetric(const unsigned char *buf_ptr, int *bytes_read);
  void publish(std::ostream *out);
  int counter() const { return m_counter_num; }
 private:
  const int m_group_id, m_counter_num;
  std::string m_name, m_description;
  float m_current_val;
  bool m_parsed;

  enum CounterType {
    kInt64Counter = GL_UNSIGNED_INT64_AMD,
    kPercentCounter =  GL_PERCENTAGE_AMD,
    kUnsignedCounter = GL_UNSIGNED_INT,
    kFloatCounter = GL_FLOAT
  } m_counter_type;
};

AMDPerfMetric::AMDPerfMetric(int group_id, int counter_num)
    : m_group_id(group_id), m_counter_num(counter_num), m_current_val(0), m_parsed(false) {
  GLsizei length;
  GlFunctions::GetPerfMonitorCounterStringAMD(
      m_group_id, m_counter_num, 0, &length, NULL);
  assert(!GlFunctions::GetError());
  std::vector<char> name(length + 1);
  GlFunctions::GetPerfMonitorCounterStringAMD(
    m_group_id, m_counter_num, length + 1, &length,
    name.data());
  assert(!GlFunctions::GetError());
  m_name = name.data();

  GLuint counter_type;
  GlFunctions::GetPerfMonitorCounterInfoAMD(
      m_group_id, m_counter_num, GL_COUNTER_TYPE_AMD, &counter_type);
  m_counter_type = static_cast<AMDPerfMetric::CounterType>(counter_type);
}

void
AMDPerfMetric::getMetric(const unsigned char *p_value, int *bytes_read) {
  switch (m_counter_type) {
  case kInt64Counter: {
    uint64_t uval = *reinterpret_cast<const uint64_t *>(p_value);
    m_current_val = static_cast<float>(uval);
    *bytes_read = sizeof(uint64_t);
    break;
  }
  case kPercentCounter:
  case kFloatCounter: {
    m_current_val = *reinterpret_cast<const float *>(p_value);
    *bytes_read = sizeof(float);
    break;
  }
  case kUnsignedCounter: {
    uint32_t uval = *reinterpret_cast<const uint32_t *>(p_value);
    m_current_val = static_cast<float>(uval);
    *bytes_read = sizeof(uint32_t);
    break;
  }
  default:
    assert(false);
  }
  m_parsed = true;
}

void
AMDPerfMetric::publish(std::ostream *outf) {
  assert(m_parsed);
  m_parsed = false;
  *outf << "\t" << m_current_val;
}

class AMDPerfMetricGroup : public PerfMetrics, PerfMetricGroup, NoCopy, NoAssign {
 public:
  explicit AMDPerfMetricGroup(int group_id);
  ~AMDPerfMetricGroup();
  const std::string &name() const { return m_group_name; }
  void get_metric_groups(std::vector<PerfMetricGroup *> *out_groups);
  void set_metric_names(std::vector<std::string> names);
  void get_metric_names(std::vector<std::string> *out_names);
  void begin(int current_frame, int event_number);
  void end(const std::string &event_type);
  void publish(std::ostream *outf, bool wait);

 private:
  std::string m_group_name;
  const int m_group_id;

  // key is the counter
  std::map<int, AMDPerfMetric *> m_metrics;

  // represent monitors that have not produced results
  std::queue<PerfValue> m_extant_monitors;

  // represent monitors that can be reused
  std::vector<unsigned int> m_free_monitors;
};

AMDPerfMetricGroup::AMDPerfMetricGroup(int group_id)
    : m_group_id(group_id) {
  GLint max_name_len = 0;
  assert(!GlFunctions::GetError());
  GlFunctions::GetPerfMonitorGroupStringAMD(m_group_id, 0,
                                            &max_name_len, NULL);
  assert(!GlFunctions::GetError());
  std::vector<GLchar> group_name(max_name_len + 1);
  GLsizei name_len;
  GlFunctions::GetPerfMonitorGroupStringAMD(m_group_id, max_name_len + 1,
                                            &name_len, group_name.data());
  m_group_name = group_name.data();

  int num_counters;
  int max_active_counters;
  GlFunctions::GetPerfMonitorCountersAMD(m_group_id,
                                         &num_counters,
                                         &max_active_counters,
                                         0, NULL);
  assert(!GlFunctions::GetError());
  std::vector<unsigned int> counters(num_counters);
  GlFunctions::GetPerfMonitorCountersAMD(m_group_id,
                                         &num_counters,
                                         &max_active_counters,
                                         num_counters, counters.data());
  assert(!GlFunctions::GetError());
  // assert(max_active_counters == num_counters);
  for (auto counter : counters) {
    AMDPerfMetric *p = new AMDPerfMetric(m_group_id, counter);
    m_metrics[counter] = p;
  }
}

AMDPerfMetricGroup::~AMDPerfMetricGroup() {
  while (!m_extant_monitors.empty()) {
    m_free_monitors.push_back(m_extant_monitors.front().handle);
    m_extant_monitors.pop();
  }
  GlFunctions::DeletePerfMonitorsAMD(m_free_monitors.size(),
                                     m_free_monitors.data());
  assert(!GlFunctions::GetError());
  m_free_monitors.clear();
  for (auto i : m_metrics)
    delete i.second;
  m_metrics.clear();
}

void
AMDPerfMetricGroup::get_metric_groups(std::vector<PerfMetricGroup *> *out_groups) {
  out_groups->push_back(this);
}

void
AMDPerfMetricGroup::set_metric_names(std::vector<std::string> names) {
  assert(names.size() > 0);

  std::map<int, AMDPerfMetric *> filtered_metrics;

  std::vector<std::string> found_names;
  for (auto i : m_metrics) {
    auto it = std::find(names.begin(), names.end(), i.second->name());
    if (it != names.end()) {
      filtered_metrics[i.first] = i.second;
      found_names.push_back(*it);
      *it = names.back();
      names.pop_back();
    } else {
      delete i.second;
    }
  }
  for (const auto n : names)
    GRLOGF(glretrace::ERR, "Could not enable metric: %s", n.c_str());

  assert(!found_names.empty());

  m_metrics.clear();
  m_metrics = filtered_metrics;
}

void
AMDPerfMetricGroup::get_metric_names(std::vector<std::string> *out_names) {
  for (auto i : m_metrics)
    out_names->push_back(i.second->name());
}

void AMDPerfMetricGroup::begin(int current_frame, int event_number) {
  if (m_free_monitors.empty()) {
    m_free_monitors.resize(10000);
    GlFunctions::GenPerfMonitorsAMD(m_free_monitors.size(),
                                    m_free_monitors.data());
    assert(!GlFunctions::GetError());

    // enable all the metrics in the group
    std::vector<unsigned int> counters;
    for (auto metric : m_metrics)
      counters.push_back(metric.first);
    for (auto i : m_free_monitors) {
      GlFunctions::SelectPerfMonitorCountersAMD(
          i, true,
          m_group_id, counters.size(),
          counters.data());
    }
    assert(!GlFunctions::GetError());
  }
  GlFunctions::BeginPerfMonitorAMD(m_free_monitors.back());
  m_extant_monitors.emplace(PerfValue(m_free_monitors.back(), current_frame,
                                      event_number));
  m_free_monitors.pop_back();
}

void
AMDPerfMetricGroup::end(const std::string &event_type) {
  m_extant_monitors.back().event_type = event_type;
  GlFunctions::EndPerfMonitorAMD(m_extant_monitors.back().handle);
}

void
AMDPerfMetricGroup::publish(std::ostream *outf, bool wait) {
  while (true) {
    if (m_extant_monitors.empty())
      break;
    const PerfValue &val = m_extant_monitors.front();
    int extant_monitor = val.handle;
    GLuint ready_for_read = 0, data_size = 0;
    GLsizei bytes_written = 0;
    while (!ready_for_read) {
      GlFunctions::GetPerfMonitorCounterDataAMD(
          extant_monitor, GL_PERFMON_RESULT_AVAILABLE_AMD,
          sizeof(GLuint), &ready_for_read, &bytes_written);
      assert(bytes_written == sizeof(GLuint));
      assert(!GlFunctions::GetError());
      if (!wait)
        break;
      if (!ready_for_read)
        GlFunctions::Finish();
    }
    if (!ready_for_read)
      return;
    *outf << val.frame << "\t" << val.number
          << "\t" << val.event_type;
    m_extant_monitors.pop();
    GlFunctions::GetPerfMonitorCounterDataAMD(extant_monitor,
                                              GL_PERFMON_RESULT_SIZE_AMD,
                                              sizeof(GLuint), &data_size,
                                              &bytes_written);
    assert(!GlFunctions::GetError());
    assert(bytes_written == sizeof(GLuint));
    std::vector<unsigned char> buf(data_size);
    GlFunctions::GetPerfMonitorCounterDataAMD(
        extant_monitor, GL_PERFMON_RESULT_AMD, data_size,
        reinterpret_cast<unsigned int *>(buf.data()), &bytes_written);
    const unsigned char *buf_ptr = buf.data();
    const unsigned char *buf_end = buf_ptr + bytes_written;
    while (buf_ptr < buf_end) {
      const GLuint *group = reinterpret_cast<const GLuint *>(buf_ptr);
      const GLuint *counter = group + 1;
      buf_ptr += 2*sizeof(GLuint);
      assert(*group == (unsigned int)m_group_id);
      int bytes_read = 0;
      m_metrics[*counter]->getMetric(buf_ptr, &bytes_read);
      buf_ptr += bytes_read;
    }
    for (auto m : m_metrics)
      m.second->publish(outf);
    m_free_monitors.push_back(extant_monitor);
    *outf << std::endl;
  }
}

PerfMetrics *
create_amd_metrics(metrics::PerfMetricDescriptor metrics_desc) {
  std::vector<unsigned int> ids;
  get_group_ids(&ids);

  for (auto group_id : ids) {
    AMDPerfMetricGroup *group = new AMDPerfMetricGroup(group_id);

    if (group->name() == metrics_desc.m_metrics_group) {
      if (metrics_desc.m_metrics_names.size() > 0) {
        group->set_metric_names(metrics_desc.m_metrics_names);
      }

      return group;
    }

    delete group;
    group = NULL;
  }

  return NULL;
}

void
dump_amd_metrics(void) {
  std::vector<unsigned int> ids;
  get_group_ids(&ids);

  for (auto group_id : ids) {
    AMDPerfMetricGroup *group = new AMDPerfMetricGroup(group_id);

    std::cout << group->name() << ":";

    std::vector<std::string> names;
    group->get_metric_names(&names);

    for (auto metric : names) {
      std::cout << " " << metric;
    }

    std::cout << std::endl;

    delete group;
  }
}
