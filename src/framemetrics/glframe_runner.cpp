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

using glretrace::PerfMetricDescriptor;
using glretrace::FrameRunner;
using glretrace::NoCopy;
using glretrace::NoAssign;
using glretrace::GlFunctions;
using glretrace::ERR;

using retrace::parser;
using trace::Call;

extern retrace::Retracer retracer;

FrameRunner::FrameRunner(const std::string filepath,
                         const std::string out_path,
                         PerfMetricDescriptor metrics_desc,
                         int max_frame,
                         MetricInterval interval,
                         int event_interval)
    : m_of(), m_out(NULL),
      m_current_frame(0),
      m_current_event(0),
      m_group_id(-1),
      m_interval(interval),
      m_event_interval(event_interval),
      m_metrics_desc(metrics_desc),
      m_current_group(NULL),
      m_parser(max_frame) {
  if (out_path.size()) {
    m_of.open(out_path);
    m_out = new std::ostream(m_of.rdbuf());
  } else {
    m_out = new std::ostream(std::cout.rdbuf());
  }

  retrace::debug = 0;
  retracer.addCallbacks(glretrace::gl_callbacks);
  retracer.addCallbacks(glretrace::glx_callbacks);
  retracer.addCallbacks(glretrace::wgl_callbacks);
  retracer.addCallbacks(glretrace::cgl_callbacks);
  retracer.addCallbacks(glretrace::egl_callbacks);
  retrace::setUp();
  parser = &m_parser;
  parser->open(filepath.c_str());
}

/* return the string starting with *arg until the specified deliminator
 * (replacing the deliminator with NULL terminator) and advancing *arg
 * to point to the start of the remainder
 */
static char *
pop_str(char **arg, char delim)
{
  if (!*arg)
    return NULL;

  char *result = *arg;
  char *s = strchr(*arg, delim);
  if (!s) {
    /* reached the end: */
    *arg = NULL;
  } else {
    s[0] = '\0';
    /* advance to next token: */
    *arg = &s[1];
  }

  return result;
}

PerfMetricDescriptor::PerfMetricDescriptor(const char *desc)
{
  if (strstr(desc, ":")) {
    /* parse group name + counter names in the form:
     * group:counter1,counter2,...
     */
    char *arg = strdup(desc);
    m_metrics_group = pop_str(&arg, ':');
    char *metric;
    while ((metric = pop_str(&arg, ',')))
      m_metrics_names.push_back(metric);
  } else {
    m_metrics_group = desc;
  }
}

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

class PerfValue {
 public:
  int handle;
  int frame;
  int number;
  std::string event_type;
  PerfValue(int _handle, int _frame,
            int _number)
      : handle(_handle), frame(_frame), number(_number),
        event_type("") {}
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

class PerfMetricGroup {
 public:
  virtual ~PerfMetricGroup() {}
  virtual const std::string &name() const = 0;
  virtual void set_metric_names(std::vector<std::string> names) = 0;
  virtual void get_metric_names(std::vector<std::string> *out_names) = 0;
  virtual void begin(int current_frame, int event_number) = 0;
  virtual void end(const std::string &event_type) = 0;
  virtual void publish(std::ostream *outf, bool wait) = 0;
};

class IntelPerfMetricGroup : public PerfMetricGroup, NoCopy, NoAssign {
 public:
  explicit IntelPerfMetricGroup(int query_id);
  ~IntelPerfMetricGroup();
  const std::string &name() const { return m_query_name; }
  void set_metric_names(std::vector<std::string> names);
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
IntelPerfMetricGroup::set_metric_names(std::vector<std::string> names)
{
  GRLOG(glretrace::ERR, "unsupported");
  assert(false);
  exit(-1);
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

class AMDPerfMetricGroup : public PerfMetricGroup, NoCopy, NoAssign {
 public:
  explicit AMDPerfMetricGroup(int group_id);
  ~AMDPerfMetricGroup();
  const std::string &name() const { return m_group_name; }
  void set_metric_names(std::vector<std::string> names);
  void get_metric_names(std::vector<std::string> *out_names);
  void begin(int current_frame, int event_number);
  void end(const std::string &event_type);
  void publish(std::ostream *outf, bool wait);

 private:
  std::string m_group_name;
  const int m_group_id;
  std::vector<unsigned char> m_data_buf;

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
AMDPerfMetricGroup::set_metric_names(std::vector<std::string> names)
{
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

enum MetricsType {
  INTEL_METRICS,
  AMD_METRICS
};

MetricsType get_metrics_type() {
  std::string extensions;
  GlFunctions::GetGlExtensions(&extensions);
  if (extensions.find("GL_INTEL_performance_query") != std::string::npos)
    return INTEL_METRICS;
  if (extensions.find("GL_AMD_performance_monitor") != std::string::npos)
    return AMD_METRICS;
  assert(false);
}

void get_group_ids(std::vector<unsigned int> *ids) {
  switch (get_metrics_type()) {
    case INTEL_METRICS: {
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
    case AMD_METRICS: {
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
  }
}

static PerfMetricGroup *create_metric_group(int group_id) {
  switch (get_metrics_type()) {
    case AMD_METRICS:
      return new AMDPerfMetricGroup(group_id);
    case INTEL_METRICS:
      return new IntelPerfMetricGroup(group_id);
    default:
      assert(false);
      return NULL;
  }
}

void
FrameRunner::dumpGroupsAndCounters() {
  /* advance until we have a context: */
  trace::Call *call;
  while ((call = parser->parse_call())) {
    retracer.retrace(*call);
    std::string extensions;
    GlFunctions::GetGlExtensions(&extensions);
    if (extensions.size() > 0)
      break;
  }

  /* drain any errors from trace: */
  while (GlFunctions::GetError()) {}

  std::vector<unsigned int> ids;
  get_group_ids(&ids);

  for (auto group : ids) {
    m_current_group = create_metric_group(group);

    std::cout << m_current_group->name() << ":";

    std::vector<std::string> names;
    m_current_group->get_metric_names(&names);

    for (auto metric : names) {
      std::cout << " " << metric;
    }

    std::cout << std::endl;

    delete m_current_group;
    m_current_group = NULL;
  }
}

void
FrameRunner::init() {
  std::vector<unsigned int> ids;
  get_group_ids(&ids);

  for (auto group : ids) {
    m_current_group = create_metric_group(group);
    if (m_current_group->name() == m_metrics_desc.m_metrics_group) {
      m_group_id = group;
      break;
    }

    delete m_current_group;
    m_current_group = NULL;
  }

  if (m_current_group == NULL) {
    std::cout << "Group " << m_metrics_desc.m_metrics_group << " not found!" << std::endl;
    exit(-1);
  }

  if (m_metrics_desc.m_metrics_names.size() > 0) {
    m_current_group->set_metric_names(m_metrics_desc.m_metrics_names);
  }

  // get current context
  Context *c = getCurrentContext();
  m_context_metrics[c] = m_current_group;

  std::vector<std::string> names;
  m_current_group->get_metric_names(&names);

  // write a header
  *m_out << "frame\tevent_number\tevent_type";
  // add each metric column to the header
  for (auto metric : names) {
    *m_out << "\t" << metric;
  }
  *m_out << std::endl;
}

void
FrameRunner::advanceToFrame(int f) {
  trace::Call *call;
  while ((call = parser->parse_call()) && m_current_frame < f) {
    assert(!GlFunctions::GetError());
    retracer.retrace(*call);
    /* drain any errors from trace: */
    while (GlFunctions::GetError()) {}
    bool save_call = false;
    const bool frame_boundary = call->flags & trace::CALL_FLAG_END_FRAME;
    if (ThreadContext::changesContext(*call)) {
      Context *c = getCurrentContext();
      m_retraced_contexts[call->arg(2).toUIntPtr()] = c;
      if (m_context_calls.find(c) == m_context_calls.end()) {
        m_context_calls[c] = call;
        save_call = true;
      }
    }

    if (!save_call)
      delete call;
    if (frame_boundary) {
      ++m_current_frame;
      if (m_current_frame == f)
        break;
    }
  }
}

void
FrameRunner::run(int end_frame) {
  // retrace count frames and output frame time
  GlFunctions::Finish();

  m_current_group->begin(m_current_frame, ++m_current_event);
  while (Call *call = parser->parse_call()) {
    bool save_call = false;

    if (ThreadContext::changesContext(*call)) {
        Context *current = getCurrentContext();
        auto new_context = call->arg(2).toUIntPtr();
        if ((new_context == 0) ||
            (m_retraced_contexts[new_context] == current)) {
          // don't retrace useless context switches
          delete call;
          continue;
        } else {
          // call actually changes context
          m_current_group->end(call->name());
        }
      }

    assert(!GlFunctions::GetError());
    retracer.retrace(*call);
    /* drain any errors from trace: */
    while (GlFunctions::GetError()) {}

    if (RetraceRender::isRender(*call) && m_interval == kPerRender) {
      ++m_current_event;
      if (m_current_event % m_event_interval == 0) {
        // stop/start metrics to measure the render
        m_current_group->end(call->name());
        m_current_group->begin(m_current_frame, m_current_event);
      }
    }

    if (ThreadContext::changesContext(*call)) {
      Context *c = getCurrentContext();
      if (m_context_calls.find(c) == m_context_calls.end()) {
        m_context_calls[c] = call;
        save_call = true;
      }

      if (m_context_metrics.find(c) == m_context_metrics.end())
        m_context_metrics[c] = create_metric_group(m_group_id);

      m_current_group = m_context_metrics[c];
      m_current_group->begin(m_current_frame, ++m_current_event);
    }

    const bool frame_boundary = call->flags & trace::CALL_FLAG_END_FRAME;
    if (frame_boundary) {
      // do not count bogus frame terminators
      if (strncmp("glFrameTerminatorGREMEDY", call->sig->name,
                  strlen("glFrameTerminatorGREMEDY")) != 0)
      {
        ++m_current_event;
        ++m_current_frame;
        if (m_interval == kPerRender || m_interval == kPerFrame) {

          if ((m_interval == kPerRender) ||
              (m_interval == kPerFrame && m_current_frame % m_event_interval == 0)) {
            m_current_group->end(call->name());
            m_current_group->publish(m_out, false);
            m_current_group->begin(m_current_frame, m_current_event);
          }
        }
        if (m_context_metrics.size() > 1) {
          glretrace::Context *original_context = NULL;
          for (auto g : m_context_metrics) {
            if (g.second == m_current_group) {
              original_context = g.first;
              continue;
            }
            // make context current for group
            retracer.retrace(*m_context_calls[g.first]);
            g.second->publish(m_out, false);
          }
          retracer.retrace(*m_context_calls[original_context]);
        }
      }
    }

    if (!save_call)
      delete call;

    if (m_current_frame >= end_frame)
      break;
  }
  for (auto g : m_context_metrics) {
    // make context current for group
    retracer.retrace(*m_context_calls[g.first]);
    g.second->end("last_render");
    GlFunctions::Finish();
    g.second->publish(m_out, true);
  }
}

FrameRunner::~FrameRunner() {
  for (auto c : m_context_calls)
    delete c.second;
  m_context_calls.clear();
  for (auto g : m_context_metrics)
    delete g.second;
  m_context_metrics.clear();

  if (m_of.is_open())
    m_of.close();
  delete m_out;
}

