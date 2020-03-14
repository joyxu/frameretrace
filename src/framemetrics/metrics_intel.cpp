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
#include "glframe_traits.hpp"
#include "glframe_logger.hpp"
#include "metrics.hpp"

using glretrace::NoCopy;
using glretrace::NoAssign;
using glretrace::GlFunctions;
using metrics::PerfValue;
using metrics::PerfMetricGroup;
using metrics::PerfMetrics;

class IntelPerfMetric : public NoCopy, NoAssign {
 public:
  IntelPerfMetric(int query_id, int counter_num);
  const std::string &name() const { return m_name; }
  void publish(const std::vector<unsigned char> &data,
               std::ostream *outf);

 private:
  const int m_query_id, m_counter_num;
  GLuint m_offset, m_data_size, m_type,
    m_data_type;
  std::string m_name, m_description;
};

IntelPerfMetric::IntelPerfMetric(int query_id,
                       int counter_num) : m_query_id(query_id),
                                          m_counter_num(counter_num) {
  static GLint max_name_len = 0, max_desc_len = 0;
  if (max_name_len == 0)
    GlFunctions::GetIntegerv(GL_PERFQUERY_COUNTER_NAME_LENGTH_MAX_INTEL,
                             &max_name_len);
  if (max_desc_len == 0)
    GlFunctions::GetIntegerv(GL_PERFQUERY_COUNTER_DESC_LENGTH_MAX_INTEL,
                             &max_desc_len);
  std::vector<GLchar> counter_name(max_name_len);
  std::vector<GLchar> counter_description(max_desc_len);
  GLuint64 max_value;
  GlFunctions::GetPerfCounterInfoINTEL(m_query_id, m_counter_num,
                                       counter_name.size(), counter_name.data(),
                                       counter_description.size(),
                                       counter_description.data(),
                                       &m_offset, &m_data_size, &m_type,
                                       &m_data_type, &max_value);
  m_name = counter_name.data();
  m_description = counter_description.data();
}

void
IntelPerfMetric::publish(const std::vector<unsigned char> &data,
                    std::ostream *outf) {
  const unsigned char *p_value = data.data() + m_offset;
  float fval;
  switch (m_data_type) {
    case GL_PERFQUERY_COUNTER_DATA_UINT32_INTEL: {
      assert(m_data_size == 4);
      const uint32_t val = *reinterpret_cast<const uint32_t *>(p_value);
      fval = static_cast<float>(val);
      break;
    }
    case GL_PERFQUERY_COUNTER_DATA_UINT64_INTEL: {
      assert(m_data_size == 8);
      const uint64_t val = *reinterpret_cast<const uint64_t *>(p_value);
      fval = static_cast<float>(val);
      break;
    }
    case GL_PERFQUERY_COUNTER_DATA_FLOAT_INTEL: {
      assert(m_data_size == 4);
      fval = *reinterpret_cast<const float *>(p_value);
      break;
    }
    case GL_PERFQUERY_COUNTER_DATA_DOUBLE_INTEL: {
      assert(m_data_size == 8);
      const double val = *reinterpret_cast<const double *>(p_value);
      fval = static_cast<float>(val);
      break;
    }
    case GL_PERFQUERY_COUNTER_DATA_BOOL32_INTEL: {
      assert(m_data_size == 4);
      const bool val = *reinterpret_cast<const bool*>(p_value);
      fval = val ? 1.0 : 0.0;
      break;
    }
    default:
      assert(false);
  }
  *outf << "\t" << fval;
}

class IntelPerfMetricGroup : public PerfMetrics, PerfMetricGroup, NoCopy, NoAssign {
 public:
  explicit IntelPerfMetricGroup(int query_id);
  ~IntelPerfMetricGroup();
  const std::string &name() const { return m_query_name; }
  void get_metric_groups(std::vector<PerfMetricGroup *> *out_groups);
  void get_metric_names(std::vector<std::string> *out_names);
  void begin(int current_frame, int event_number);
  void end(const std::string &event_type);
  void publish(std::ostream *outf, bool wait);

 private:
  std::string m_query_name;
  const int m_query_id;
  unsigned int m_data_size;
  std::vector<unsigned char> m_data_buf;

  std::vector<IntelPerfMetric *> m_metrics;

  // represent queries that have not produced results
  std::queue<PerfValue> m_extant_query_handles;

  // represent query handles that can be reused
  std::vector<unsigned int> m_free_query_handles;
};

IntelPerfMetricGroup::IntelPerfMetricGroup(int query_id)
    : m_query_id(query_id) {
  static GLint max_name_len = 0;
  if (max_name_len == 0)
    GlFunctions::GetIntegerv(GL_PERFQUERY_QUERY_NAME_LENGTH_MAX_INTEL,
                             &max_name_len);

  std::vector<GLchar> query_name(max_name_len);
  unsigned int number_instances, capabilities_mask, number_counters;
  GlFunctions::GetPerfQueryInfoINTEL(m_query_id,
                                     query_name.size(), query_name.data(),
                                     &m_data_size, &number_counters,
                                     &number_instances, &capabilities_mask);
  m_data_buf.resize(m_data_size);
  m_query_name = query_name.data();
  for (unsigned int counter_num = 1; counter_num <= number_counters;
       ++counter_num) {
    m_metrics.push_back(new IntelPerfMetric(m_query_id, counter_num));
  }
}

void
IntelPerfMetricGroup::get_metric_groups(std::vector<PerfMetricGroup *> *out_groups) {
  out_groups->push_back(this);
}

void
IntelPerfMetricGroup::get_metric_names(std::vector<std::string> *out_names) {
  for (auto m : m_metrics) {
    out_names->push_back(m->name());
  }
}

void
IntelPerfMetricGroup::begin(int current_frame, int event_number) {
  if (m_free_query_handles.empty()) {
    for (int i = 0; i < 10000; ++i) {
      GLuint query_handle;
      GlFunctions::CreatePerfQueryINTEL(m_query_id, &query_handle);
      m_free_query_handles.push_back(query_handle);
    }
  }
  GLuint query_handle = m_free_query_handles.back();
  m_free_query_handles.pop_back();

  // When more than one process requests metrics concurrently,
  // BeginPerfQueryINTEL fails.
  int retry = 0;
  GlFunctions::GetError();
  while (true) {
    GlFunctions::BeginPerfQueryINTEL(query_handle);
    if (GL_NO_ERROR == GlFunctions::GetError())
      break;
    assert(false);
    if (++retry > 20) {
      GRLOG(glretrace::ERR, "failed to begin metrics query, aborting");
      assert(false);
      exit(-1);
    }
    GRLOG(glretrace::WARN, "failed to begin metrics query");
    assert(false);
  }
  m_extant_query_handles.emplace(query_handle, current_frame, event_number);
}

void
IntelPerfMetricGroup::publish(std::ostream *outf, bool wait) {
  while (true) {
    if (m_extant_query_handles.empty())
      break;

    const auto &extant_query = m_extant_query_handles.front();
    memset(m_data_buf.data(), 0, m_data_buf.size());
    GLuint bytes_written = 0;
    GLuint flags = wait ? GL_PERFQUERY_WAIT_INTEL
                   : GL_PERFQUERY_DONOT_FLUSH_INTEL;
    GlFunctions::GetPerfQueryDataINTEL(extant_query.handle,
                                       flags,
                                       m_data_size, m_data_buf.data(),
                                       &bytes_written);
    if (!bytes_written)
      break;

    assert(bytes_written == m_data_size);
    *outf << extant_query.frame << "\t" << extant_query.number
          << "\t" << extant_query.event_type;
    for (auto desired_metric : m_metrics) {
      desired_metric->publish(m_data_buf, outf);
    }
    m_free_query_handles.push_back(extant_query.handle);
    m_extant_query_handles.pop();
    *outf << std::endl;
  }
}

void
IntelPerfMetricGroup::end(const std::string &event_type) {
  m_extant_query_handles.back().event_type = event_type;
  GlFunctions::EndPerfQueryINTEL(m_extant_query_handles.back().handle);
}

IntelPerfMetricGroup::~IntelPerfMetricGroup() {
  for (auto free_query : m_free_query_handles) {
    GlFunctions::DeletePerfQueryINTEL(free_query);
  }
  m_free_query_handles.clear();
  assert(m_extant_query_handles.empty());
  for (auto m : m_metrics)
    delete m;
  m_metrics.clear();
}


static void
get_query_ids(std::vector<unsigned int> *ids) {
  GLuint query_id;
  // list the metrics in the group
  GlFunctions::GetFirstPerfQueryIdINTEL(&query_id);
  if (query_id == GLuint(-1)) {
    assert(false);
    return;
  }

  if (query_id == 0)
    return;
  ids->push_back(query_id);

  while (true) {
    GlFunctions::GetNextPerfQueryIdINTEL(query_id, &query_id);
    if (!query_id)
      break;
    ids->push_back(query_id);
  }
  return;
}

PerfMetrics *
create_intel_metrics(std::vector<metrics::PerfMetricDescriptor> metrics_descs) {
  if (metrics_descs.size() != 1) {
    std::cout << "More than one metrics group is not supported "
        "by intel metrics!" << std::endl;
    return NULL;
  }

  metrics::PerfMetricDescriptor metrics_desc = metrics_descs.front();

  std::vector<unsigned int> ids;
  get_query_ids(&ids);

  if (metrics_desc.m_metrics_names.size() > 0) {
    std::cout << "Specifying individual metrics within group "
        "is not supported by intel metrics!" << std::endl;
    return NULL;
  }

  for (auto query_id : ids) {
    IntelPerfMetricGroup *group = new IntelPerfMetricGroup(query_id);
    if (group->name() == metrics_desc.m_metrics_group)
      return group;

    delete group;
  }

  return NULL;
}

void
dump_intel_metrics(void) {
  std::vector<unsigned int> ids;
  get_query_ids(&ids);

  for (auto query_id : ids) {
    IntelPerfMetricGroup *group = new IntelPerfMetricGroup(query_id);

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
