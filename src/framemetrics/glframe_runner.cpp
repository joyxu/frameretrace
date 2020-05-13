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

using metrics::PerfMetricDescriptor;
using metrics::PerfMetricGroup;
using metrics::PerfMetrics;
using glretrace::FrameRunner;
using glretrace::GlFunctions;
using glretrace::ERR;

using retrace::parser;
using trace::Call;

extern retrace::Retracer retracer;

FrameRunner::FrameRunner(const std::string filepath,
                         const std::string out_path,
                         std::vector<metrics::PerfMetricDescriptor> metrics_descs,
                         unsigned max_frame,
                         MetricInterval interval,
                         unsigned event_interval)
    : m_of(), m_out(NULL),
      m_current_frame(0),
      m_current_event(0),
      m_interval(interval),
      m_event_interval(event_interval),
      m_metrics_descs(metrics_descs),
      m_current_metrics(NULL),
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

static PerfMetrics *
create_metric_group(std::vector<metrics::PerfMetricDescriptor> metrics_descs) {
  switch (get_metrics_type()) {
    case AMD_METRICS:
      return create_amd_metrics(metrics_descs);
    case INTEL_METRICS:
      return create_intel_metrics(metrics_descs);
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

  switch (get_metrics_type()) {
    case AMD_METRICS:
      dump_amd_metrics();
      break;
    case INTEL_METRICS:
      dump_intel_metrics();
      break;
    default:
      assert(false);
  }
}

void
FrameRunner::init() {
  m_current_metrics = create_metric_group(m_metrics_descs);

  if (m_current_metrics == NULL) {
    exit(-1);
  }

  // get current context
  Context *c = getCurrentContext();
  m_context_metrics[c] = m_current_metrics;

  std::vector<PerfMetricGroup *> groups;
  m_current_metrics->get_metric_groups(&groups);

  // write a header
  *m_out << "frame\tevent_number\tevent_type\tprogram";

  // add each metric column to the header
  for (auto group : groups) {
    std::vector<std::string> names;
    group->get_metric_names(&names);

    for (auto metric : names) {
      *m_out << "\t" << metric;
    }
  }
  *m_out << std::endl;
}

void
FrameRunner::advanceToFrame(unsigned f) {
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

int
FrameRunner::get_prog()
{
  if (m_interval != kPerRender)
    return 0;
  int prog;
  GlFunctions::GetIntegerv(GL_CURRENT_PROGRAM, &prog);
  assert(!GlFunctions::GetError());
  return prog;
}

void
FrameRunner::run(unsigned end_frame) {
  // retrace count frames and output frame time
  GlFunctions::Finish();

  m_current_metrics->begin(m_current_frame, ++m_current_event, get_prog());
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
          m_current_metrics->end(call->name());
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
        m_current_metrics->end(call->name());
        m_current_metrics->begin(m_current_frame, m_current_event, get_prog());
      }
    }

    if (ThreadContext::changesContext(*call)) {
      Context *c = getCurrentContext();
      if (m_context_calls.find(c) == m_context_calls.end()) {
        m_context_calls[c] = call;
        save_call = true;
      }

      if (m_context_metrics.find(c) == m_context_metrics.end())
        m_context_metrics[c] = create_metric_group(m_metrics_descs);

      m_current_metrics = m_context_metrics[c];
      m_current_metrics->begin(m_current_frame, ++m_current_event, get_prog());
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
            m_current_metrics->end(call->name());
            m_current_metrics->publish(m_out, false);
            m_current_metrics->begin(m_current_frame, m_current_event, get_prog());
          }
        }
        if (m_context_metrics.size() > 1) {
          glretrace::Context *original_context = NULL;
          for (auto g : m_context_metrics) {
            if (g.second == m_current_metrics) {
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

