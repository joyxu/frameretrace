/**************************************************************************
 *
 * Copyright 2015 Intel Corporation
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
 **************************************************************************/

#include <qtextstream.h>
#include "glframe_retrace_model.hpp"

// this has to be first, yuck
#include <QQuickImageProvider>   // NOLINT
#include <QtConcurrentRun>   // NOLINT

#include <sstream>  // NOLINT
#include <string> // NOLINT
#include <vector> // NOLINT

#include "glframe_qbargraph.hpp"
#include "glframe_retrace.hpp"
#include "glframe_retrace_images.hpp"

using glretrace::FrameRetraceModel;
using glretrace::FrameState;
using glretrace::QMetric;
using glretrace::QRenderBookmark;
using glretrace::QBarGraphRenderer;
using glretrace::QSelection;

FrameRetraceModel::FrameRetraceModel() : m_selection(NULL),
                                         m_open_percent(0),
                                         m_max_metric(0) {
  m_metrics_model.push_back(new QMetric(MetricId(0), "No metric"));
  connect(this, &glretrace::FrameRetraceModel::updateMetricList,
          this, &glretrace::FrameRetraceModel::onUpdateMetricList);
}

FrameRetraceModel::~FrameRetraceModel() {
  ScopedLock s(m_protect);
  if (m_state) {
    delete m_state;
    m_state = NULL;
  }
}

FrameState *frame_state_off_thread(std::string filename,
                                   int framenumber) {
  return new FrameState(filename, framenumber);
}

static QFuture<FrameState *> future;

void
FrameRetraceModel::setFrame(const QString &filename, int framenumber) {
  // m_retrace = new FrameRetrace(filename.toStdString(), framenumber);
  future = QtConcurrent::run(frame_state_off_thread,
                             filename.toStdString(), framenumber);
  m_retrace.openFile(filename.toStdString(), framenumber, this);
}

QQmlListProperty<QRenderBookmark>
FrameRetraceModel::renders() {
  return QQmlListProperty<QRenderBookmark>(this, m_renders_model);
}

QQmlListProperty<QMetric>
FrameRetraceModel::metricList() {
  return QQmlListProperty<QMetric>(this, m_metrics_model);
}


void
FrameRetraceModel::onShaderAssembly(RenderId renderId,
                                    const std::string &vertex_shader,
                                    const std::string &vertex_ir,
                                    const std::string &vertex_vec4,
                                    const std::string &fragment_shader,
                                    const std::string &fragment_ir,
                                    const std::string &fragment_simd8,
                                    const std::string &fragment_simd16,
                                    const std::string &fragment_nir_ssa,
                                    const std::string &fragment_nir_final) {
  ScopedLock s(m_protect);
  m_vs_ir = vertex_ir.c_str();
  m_fs_ir = fragment_ir.c_str();
  m_vs_shader = vertex_shader.c_str();
  m_fs_shader = fragment_shader.c_str();
  m_vs_vec4 = vertex_vec4.c_str();
  m_fs_simd8 = fragment_simd8.c_str();
  m_fs_simd16 = fragment_simd16.c_str();
  m_fs_ssa = fragment_nir_ssa.c_str();
  m_fs_nir = fragment_nir_final.c_str();
  emit onShaders();
}


void
FrameRetraceModel::onRenderTarget(RenderId renderId,
                                  RenderTargetType type,
                                  const std::vector<unsigned char> &data) {
  ScopedLock s(m_protect);
  glretrace::FrameImages::instance()->SetImage(data);
  emit onRenderTarget();
}

void
FrameRetraceModel::onShaderCompile(RenderId renderId,
                                   int status,
                                   std::string errorString) {}

void
FrameRetraceModel::retrace(int start) {
  m_retrace.retraceShaderAssembly(RenderId(start), this);
  m_retrace.retraceRenderTarget(RenderId(start), 0, glretrace::NORMAL_RENDER,
                                glretrace::STOP_AT_RENDER, this);
}


QString
FrameRetraceModel::renderTargetImage() const {
  ScopedLock s(m_protect);
  static int i = 0;
  std::stringstream ss;
  ss << "image://myimageprovider/image" << ++i << ".png";
  return ss.str().c_str();
}

void
FrameRetraceModel::onFileOpening(bool finished,
                                 uint32_t percent_complete) {
  ScopedLock s(m_protect);
  if (finished) {
    m_state = future.result();
    const int rcount = m_state->getRenderCount();
    for (int i = 0; i < rcount; ++i) {
      m_renders_model.append(new QRenderBookmark(i));
    }
    emit onRenders();

    m_open_percent = 101;
    setMetric(0, 0);
    setMetric(1, 0);
  }
  if (m_open_percent == percent_complete)
    return;
  m_open_percent = percent_complete;
  emit onOpenPercent();
}

void
FrameRetraceModel::onMetricList(const std::vector<MetricId> &ids,
                                const std::vector<std::string> &names) {
  ScopedLock s(m_protect);
  t_ids = ids;
  t_names = names;
  emit updateMetricList();
}

void
FrameRetraceModel::onMetrics(const MetricSeries &metricData,
                             ExperimentId experimentCount) {
  ScopedLock s(m_protect);
  bool vertical_metric = false;
  if (metricData.metric == m_active_metrics[0])
    vertical_metric = true;
  if (vertical_metric)
    m_max_metric = 0.0;
  assert(m_metrics.size() <= metricData.data.size());
  while (m_metrics.size() < metricData.data.size())
    m_metrics.append(BarMetrics());
  auto bar = m_metrics.begin();
  for (auto i : metricData.data) {
    vertical_metric ? bar->metric1 = i : bar->metric2 = i;
    if (vertical_metric && i > m_max_metric)
      m_max_metric = i;
    ++bar;
  }
  emit onMaxMetric();
  emit onQMetricData(m_metrics);
}

void
FrameRetraceModel::onUpdateMetricList() {
  ScopedLock s(m_protect);
  for (auto i : m_metrics_model)
    delete i;
  m_metrics_model.clear();
  m_metrics_model.push_back(new QMetric(MetricId(0), "No metric"));
  assert(t_ids.size() == t_names.size());
  for (int i = 0; i < t_ids.size(); ++i)
    m_metrics_model.append(new QMetric(t_ids[i], t_names[i]));
  emit onQMetricList();
}

void
FrameRetraceModel::setMetric(int index, int id) {
  if (m_open_percent < 100)
    // file not open yet
    return;

  if (index >= m_active_metrics.size())
    m_active_metrics.resize(index+1);

  if (m_active_metrics[index] == MetricId(id))
    return;
  m_active_metrics[index] = MetricId(id);

  // clear bar data, so stale data will not be displayed.  It may be
  // than one of the metrics we are requesting is "No metric"
  for (auto &bar : m_metrics) {
    bar.metric1 = 0;
    bar.metric2 = 0;
  }

  std::vector<MetricId> t_metrics = m_active_metrics;
  if (t_metrics[1] == MetricId(0))
    // don't replay a null metric for the widths, it makes the bars contiguous.
    t_metrics.resize(1);

  m_retrace.retraceMetrics(t_metrics, ExperimentId(0),
                           this);
}

void
FrameRetraceModel::subscribe(QBarGraphRenderer *graph) {
  ScopedLock s(m_protect);
  connect(this, &FrameRetraceModel::onQMetricData,
          graph, &QBarGraphRenderer::onMetrics);
}

QSelection *
FrameRetraceModel::selection() {
  ScopedLock s(m_protect);
  return m_selection;
}

void
FrameRetraceModel::setSelection(QSelection *s) {
  ScopedLock st(m_protect);
  m_selection = s;
  connect(s, &QSelection::onSelect,
          this, &FrameRetraceModel::onSelect);
}

void
FrameRetraceModel::onSelect(QList<int> selection) {
  ScopedLock s(m_protect);
  retrace(selection.back());
}
