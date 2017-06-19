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

// this has to be first, yuck
#include <QQuickImageProvider>
#include <QtConcurrentRun>

#include <sstream>
#include <string>
#include <vector>

#include "glframe_retrace_model.hpp"


#include "glframe_retrace.hpp"
#include "glframe_retrace_images.hpp"

using glretrace::FrameRetraceModel;
using glretrace::FrameState;
using glretrace::QMetric;
using glretrace::QRenderBookmark;

FrameRetraceModel::FrameRetraceModel() : m_open_percent(0) {
  connect(this, &glretrace::FrameRetraceModel::updateMetricList,
          this, &glretrace::FrameRetraceModel::onUpdateMetricList);
}

FrameRetraceModel::~FrameRetraceModel() {
  std::cout << "~FrameRetraceModel\n";
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
                                    const std::string &fragment_simd16) {
  ScopedLock s(m_protect);
  m_vs_ir = vertex_ir.c_str();
  m_fs_ir = fragment_ir.c_str();
  m_vs_shader = vertex_shader.c_str();
  m_fs_shader = fragment_shader.c_str();
  m_vs_vec4 = vertex_vec4.c_str();
  m_fs_simd8 = fragment_simd8.c_str();
  m_fs_simd16 = fragment_simd16.c_str();
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
  ScopedLock s(m_protect);
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
FrameRetraceModel::onUpdateMetricList() {
  m_metrics_model.clear();
  ScopedLock s(m_protect);
  assert(t_ids.size() == t_names.size());
  for (int i = 0; i < t_ids.size(); ++i)
    m_metrics_model.append(new QMetric(t_ids[i], t_names[i]));
  emit onQMetricList();
}

void
FrameRetraceModel::setMetric(int index, int id) {
  if (index >= m_active_metrics.size())
    m_active_metrics.resize(index+1);
  m_active_metrics[index] = MetricId(id);
  m_retrace.retraceMetrics(m_active_metrics, ExperimentId(0),
                           this);
}
