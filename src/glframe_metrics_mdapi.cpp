// Copyright (C) Intel Corp.  2020.  All Rights Reserved.

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

#include "glframe_metrics_mdapi.hpp"

#ifndef _WIN64
#include <dlfcn.h>
#endif
#include <GL/gl.h>
#include <GL/glext.h>

#include "metrics_discovery_api.h"

#include "glretrace.hpp"
#include "glframe_glhelper.hpp"

using glretrace::PerfMetricsMdapi;
using glretrace::OnFrameRetrace;
using glretrace::MetricId;
using glretrace::RenderId;
using glretrace::ExperimentId;
using glretrace::SelectionId;
using glretrace::GlFunctions;
using glretrace::ID_PREFIX_MASK;

using MetricsDiscovery::TTypedValue_1_0;
using MetricsDiscovery::OpenMetricsDevice_fn;
using MetricsDiscovery::IMetricsDevice_1_5;
using MetricsDiscovery::TMetricsDeviceParams_1_2;
using MetricsDiscovery::IConcurrentGroup_1_5;
using MetricsDiscovery::TConcurrentGroupParams_1_0;
using MetricsDiscovery::TMetricSetParams_1_4;
using MetricsDiscovery::IMetric_1_0;
using MetricsDiscovery::TMetricParams_1_0;
using MetricsDiscovery::API_TYPE_OGL4_X;
using MetricsDiscovery::API_TYPE_OGL;
using MetricsDiscovery::IMetricSet_1_5;
using MetricsDiscovery::TMetricSetParams_1_4;
using MetricsDiscovery::TCompletionCode;
using MetricsDiscovery::CC_OK;
using MetricsDiscovery::VALUE_TYPE_UINT32;
using MetricsDiscovery::VALUE_TYPE_UINT64;
using MetricsDiscovery::VALUE_TYPE_FLOAT;
using MetricsDiscovery::VALUE_TYPE_BOOL;
using MetricsDiscovery::VALUE_TYPE_CSTRING;

static IConcurrentGroup_1_5* mdapi_group = NULL;
static const unsigned int API_MASK = (API_TYPE_OGL4_X |
                                      API_TYPE_OGL);

static void init() {
  OpenMetricsDevice_fn OpenMetricsDevice;
#ifdef _WIN64
  HMODULE handle_to_proprietary_dll = NULL;
  CHAR loc[1024];
  DWORD loc_len = sizeof(loc);
  HKEY sub_key;
  RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Intel\\Display\\igfxcui", 0, KEY_READ, &sub_key);
  RegGetValue(sub_key, "", "InstPath", RRF_RT_ANY, NULL, loc, &loc_len);
  SetDllDirectory(loc);

  // SetDllDirectory("c:/Users/majanes/Downloads/Driver-Release-Internal-64-bit/");
  handle_to_proprietary_dll = LoadLibrary(TEXT("igdmd64.dll"));
  OpenMetricsDevice = (MetricsDiscovery::OpenMetricsDevice_fn) GetProcAddress(handle_to_proprietary_dll, "OpenMetricsDevice");
#else
  void *libmd = dlopen("libmd.so", RTLD_LAZY | RTLD_GLOBAL);
  OpenMetricsDevice = (OpenMetricsDevice_fn) dlsym(libmd,                                                                        "OpenMetricsDevice");
#endif
  
  IMetricsDevice_1_5* device = NULL;
  OpenMetricsDevice(&device);
  TMetricsDeviceParams_1_2* device_params = device->GetParams();
  const int group_count = device_params->ConcurrentGroupsCount;
  for( int i = 0; i < group_count; i++ ) {
    IConcurrentGroup_1_5* group = device->GetConcurrentGroup(i);
    TConcurrentGroupParams_1_0* group_params = group->GetParams();
    if( NULL == group_params )
    {
      //we are done, general error
      printf("CG params null");
      assert(false);
      return;
    }
    if (strncmp(group_params->SymbolName, "OA", 2) != 0)
      continue;

    mdapi_group = group;
    return;
  }
  assert(false);
}


struct MetricDescription {
  MetricId id;
  std::string name;
  std::string description;
  MetricDescription() {}
  MetricDescription(MetricId i,
                    const std::string &n,
                    const std::string &d)
      : id(i), name(n), description(d) {}
};

class MdapiMetric {
 public:
  MdapiMetric(int set_offset, int metric_offset);
  MetricId id() const { return m_id; }
  const std::string &name() const { return m_name; }
  const std::string &description() const { return m_description; }
  float getMetric(const std::vector<TTypedValue_1_0> &values) const;
 private:
  const int m_metric_offset;
  const MetricId m_id;
  std::string m_name, m_description;
};

MdapiMetric::MdapiMetric(int set_offset, int metric_offset)
    : m_metric_offset(metric_offset),
      m_id(set_offset, metric_offset)
{
  auto set = mdapi_group->GetMetricSet(set_offset);
  set->SetApiFiltering(API_MASK);
  auto metric = set->GetMetric(metric_offset);
  auto metric_param = metric->GetParams();
  m_name = metric_param->ShortName;
  m_description = metric_param->LongName;
}

float
MdapiMetric::getMetric(const std::vector<TTypedValue_1_0> &values) const {
  auto &v = values[m_metric_offset];
  switch (v.ValueType) {
    case VALUE_TYPE_UINT32:
      return v.ValueUInt32;
    case VALUE_TYPE_UINT64:
      return v.ValueUInt64;
    case VALUE_TYPE_FLOAT:
      return v.ValueFloat;
    case VALUE_TYPE_BOOL:
      return v.ValueBool;
    case VALUE_TYPE_CSTRING:
      return atof(v.ValueCString);
    default:
      assert(false);
      break;
  };
}

class MdapiGroup {
 public:
  explicit MdapiGroup(int index);
  ~MdapiGroup();
  // const std::string &name() const;
  void metrics(std::vector<const MdapiMetric*> *m) const;
  void activate() { m_metric_set->Activate(); }
  void begin(RenderId render);
  void end(RenderId render);
  void publish(MetricId metric, PerfMetricsMdapi::MetricMap *m);
 private:
  IMetricSet_1_5* m_metric_set;
  std::vector<unsigned char> m_buf;
  std::vector<TTypedValue_1_0> m_values;
  std::map<MetricId, MdapiMetric *> m_metrics;
  unsigned int m_query_id;

  // represent queries that have not produced results
  std::map<RenderId, int> m_extant_query_handles;
  // represent query handles that can be reused
  std::vector<unsigned int> m_free_query_handles;
};

MdapiGroup::MdapiGroup(int index) : m_metric_set(NULL) {
  assert(mdapi_group);
  m_metric_set = mdapi_group->GetMetricSet(index);
  m_metric_set->SetApiFiltering(API_MASK);
  TMetricSetParams_1_4* set_params = m_metric_set->GetParams();
  for (unsigned int m = 0; m < set_params->MetricsCount; ++m ) {
    MdapiMetric *metric = new MdapiMetric(index, m);
    m_metrics[metric->id()] = metric;
  }
  m_values.resize(set_params->MetricsCount + set_params->InformationCount);
  m_buf.resize(set_params->QueryReportSize);
  char * query_name = (char*)"Intel_Raw_Hardware_Counters_Set_0_Query";
  GlFunctions::GetPerfQueryIdByNameINTEL (query_name,
                                          &m_query_id);

}

MdapiGroup::~MdapiGroup() {
}

void
MdapiGroup::metrics(std::vector<const MdapiMetric *> *m) const {
  for (auto metric : m_metrics)
    m->push_back(metric.second);
}

void
MdapiGroup::begin(RenderId render) {
  if (m_free_query_handles.empty()) {
    GLuint query_handle;
    GlFunctions::CreatePerfQueryINTEL(m_query_id, &query_handle);
    m_free_query_handles.push_back(query_handle);
  }
  GLuint query_handle = m_free_query_handles.back();
  m_free_query_handles.pop_back();
  GlFunctions::BeginPerfQueryINTEL(query_handle);
  m_extant_query_handles[render] = query_handle;
}

void
MdapiGroup::end(RenderId render) {
  if (m_extant_query_handles.find(render) != m_extant_query_handles.end())
    GlFunctions::EndPerfQueryINTEL(m_extant_query_handles[render]);
}

static const MetricId ALL_METRICS_IN_GROUP = MetricId(~ID_PREFIX_MASK);

void
MdapiGroup::publish(MetricId metric, PerfMetricsMdapi::MetricMap *out_metrics) {
  const bool publish_all = (metric == ALL_METRICS_IN_GROUP);
  for (auto extant_query : m_extant_query_handles) {
    memset(m_buf.data(), 0, m_buf.size());
    GLuint bytes_written = 0;
    GlFunctions::GetPerfQueryDataINTEL(extant_query.second,
                                       GL_PERFQUERY_WAIT_INTEL,
                                       m_buf.size(), m_buf.data(),
                                       &bytes_written);
    assert(bytes_written == m_buf.size());
    uint32_t out_report_count;
    const size_t s = m_values.size();
    m_values.clear();
    m_values.resize(s);
    TCompletionCode result = m_metric_set->CalculateMetrics(m_buf.data(),
                                                            m_buf.size(),
                                                            m_values.data(),
                                                            m_values.size() * sizeof(TTypedValue_1_0),
                                                            &out_report_count,
                                                            NULL, 0);
    assert(result == CC_OK);

    if (publish_all) {
      for (auto desired_metric : m_metrics) {
        MetricId met_id = desired_metric.first;
        (*out_metrics)[met_id][extant_query.first] =
            desired_metric.second->getMetric(m_values);
      }
    } else {
        (*out_metrics)[metric][extant_query.first] =
            m_metrics[metric]->getMetric(m_values);
    }
    m_free_query_handles.push_back(extant_query.second);
  }
  m_extant_query_handles.clear();
}

using glretrace::MdapiContext;
class glretrace::MdapiContext {
 public:
  MdapiContext(OnFrameRetrace *cb=NULL);
  ~MdapiContext();
  int groupCount() const;
  void selectMetric(MetricId metric);
  void selectGroup(int index);
  void begin(RenderId render);
  void end();
  void publish(PerfMetricsMdapi::MetricMap *metrics);
 private:
  std::vector<MdapiGroup *> m_groups;
  // indicates offset in groups of PerfMetricGroup reporting MetricId
  std::map<MetricId, int> m_metric_map;
  // indicates the group that will handle subsequent begin/end calls
  MdapiGroup *m_current_group;
  MetricId m_current_metric;
  RenderId m_current_render;
};

MdapiContext::MdapiContext(OnFrameRetrace *cb)
    : m_current_group(NULL) {
  TConcurrentGroupParams_1_0* params = mdapi_group->GetParams();
  std::vector<MetricId> ids;
  std::vector<std::string> names;
  std::vector<std::string> descriptions;

  std::map<std::string, bool> known_metrics;
  std::vector<const MdapiMetric*> metrics;
  for (unsigned int index = 0; index < params->MetricSetsCount; ++index) {
    m_groups.push_back(new MdapiGroup(index));
    metrics.clear();
    m_groups.back()->metrics(&metrics);
    for (auto m : metrics) {
      const std::string &name = m->name();
      if (known_metrics.find(name) != known_metrics.end())
        continue;
      known_metrics[name] = true;
      m_metric_map[m->id()] = index;

      if(cb) {
        ids.push_back(m->id());
        names.push_back(m->name());
        descriptions.push_back(m->description());
      }
    }
  }
  if (cb)
    cb->onMetricList(ids, names, descriptions);
}

MdapiContext::~MdapiContext() {
  for (auto g : m_groups)
    delete g;
  m_groups.clear();
  m_metric_map.clear();
}

int
MdapiContext::groupCount() const {
  return m_groups.size();
}
void
MdapiContext::selectMetric(MetricId metric) {
  assert(m_metric_map.find(metric) != m_metric_map.end());
  m_current_metric = metric;
  m_current_group = m_groups[m_metric_map[metric]];
  m_current_group->activate();
}

void
MdapiContext::selectGroup(int index) {
  if (m_groups.empty())
    return;
  m_current_group = m_groups[index];
  m_current_metric = ALL_METRICS_IN_GROUP;
}

void
MdapiContext::begin(RenderId render) {
  m_current_group->begin(render);
  m_current_render = render;
}

void
MdapiContext::end() {
  m_current_group->end(m_current_render);
}

void
MdapiContext::publish(PerfMetricsMdapi::MetricMap *metrics) {
  m_current_group->publish(m_current_metric, metrics);
}

PerfMetricsMdapi::PerfMetricsMdapi(OnFrameRetrace *cb)
    : m_current_context(NULL), m_current_group(0) {
  if (mdapi_group == NULL)
    init();
  m_current_context = new MdapiContext(cb);
}

PerfMetricsMdapi::~PerfMetricsMdapi() {
  for (auto i : m_contexts) {
    delete i.second;
  }
  m_contexts.clear();
}

int
PerfMetricsMdapi::groupCount() const { return m_contexts.begin()->second->groupCount(); }

void
PerfMetricsMdapi::selectMetric(MetricId metric) {
  m_data.clear();
  m_current_metric = metric;
  for (auto i : m_contexts)
    i.second->selectMetric(metric);
}

void
PerfMetricsMdapi::selectGroup(int index) {
  m_current_group = index;
  m_current_metric = ALL_METRICS_IN_GROUP;
  for (auto i : m_contexts)
    i.second->selectGroup(index);
}

void
PerfMetricsMdapi::begin(RenderId render) {
  if (!m_current_context) {
    beginContext();
  }
  m_current_context->begin(render);
}

void
PerfMetricsMdapi::end() {
  if (m_current_context)
    m_current_context->end();
}

void
PerfMetricsMdapi::publish(ExperimentId experimentCount,
                          SelectionId selectionCount,
                          OnFrameRetrace *callback) {
  for (auto i : m_contexts)
    i.second->publish(&m_data);

  for (auto i : m_data) {
    MetricSeries s;
    s.metric = i.first;
    s.data.resize(i.second.rbegin()->first.index() + 1);
    for (auto datapoint : i.second)
      s.data[datapoint.first.index()] = datapoint.second;
    callback->onMetrics(s, experimentCount, selectionCount);
  }
  m_data.clear();
}

void
PerfMetricsMdapi::endContext() {
  if (m_current_context) {
    m_current_context->end();
    m_current_context->publish(&m_data);
  }
  m_current_context = NULL;
}

void
PerfMetricsMdapi::beginContext() {
  Context *c = getCurrentContext();
  auto entry = m_contexts.find(c);
  if (entry != m_contexts.end()) {
    m_current_context = entry->second;
  } else {
    m_current_context = new MdapiContext();
    m_contexts[c] = m_current_context;
  }
  m_current_context->selectGroup(m_current_group);
  if (m_current_metric() &&
      (m_current_metric != ALL_METRICS_IN_GROUP))
    m_current_context->selectMetric(m_current_metric);
}

