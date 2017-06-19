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

#ifndef _GLFRAME_RETRACE_INTERFACE_HPP_
#define _GLFRAME_RETRACE_INTERFACE_HPP_

#include <assert.h>

#include <map>
#include <string>
#include <vector>

namespace glretrace {

enum RenderTargetType {
  HIGHLIGHT_RENDER,
  NORMAL_RENDER,
  DEPTH_RENDER,
  GEOMETRY_RENDER,
  OVERDRAW_RENDER,
  // etc
};

enum RenderOptions {
  DEFAULT_RENDER = 0x0,
  STOP_AT_RENDER = 0x1,
  CLEAR_BEFORE_RENDER = 0x2,
};

// masked onto common integer Ids, to prevent any confusion between
// integer identifiers.
enum IdPrefix {
  CALL_ID_PREFIX = 0x1 << 28,
  RENDER_ID_PREFIX = 0x2  << 28,
  RENDER_TARGET_ID_PREFIX = 0x3  << 28,
  METRIC_ID_PREFIX = 0x4  << 28,
  SELECTION_ID_PREFIX = 0x5  << 28,
  EXPERIMENT_ID_PREFIX = 0x6  << 28,
  ID_PREFIX_MASK = 0xf << 28
};

// Decorates Render numbers with a mask, so they can never be confused
// with any other Id
class RenderId {
 public:
  RenderId() : value(0) {}
  explicit RenderId(uint32_t renderNumber) {
    if (renderNumber == (uint32_t)-1)
      // -1 renderId means "no render".  This id addresses, for
      // -example, tracked state up to the point where the frame
      // -begins.
      renderNumber ^= ID_PREFIX_MASK;
    assert(((renderNumber & ID_PREFIX_MASK) == 0) ||
           ((renderNumber & ID_PREFIX_MASK) == RENDER_ID_PREFIX));
    value = RENDER_ID_PREFIX | renderNumber;
  }

  uint32_t operator()() const { return value; }
  uint32_t index() const { return value & (~ID_PREFIX_MASK); }
  bool operator<(const RenderId &o) const { return value < o.value; }
  bool operator>(const RenderId &o) const { return value > o.value; }
 private:
  uint32_t value;
};

// Decorates Metric numbers with a mask, so they can never be confused
// with any other Id
class MetricId {
 public:
  explicit MetricId(uint64_t metricNumber) {
    assert(((metricNumber & ID_PREFIX_MASK) == 0) ||
           ((metricNumber & ID_PREFIX_MASK) == METRIC_ID_PREFIX));
    value = METRIC_ID_PREFIX | metricNumber;
  }
  MetricId(uint32_t group, uint16_t counter) {
    value = METRIC_ID_PREFIX | (group << 16) | counter;
  }
  MetricId() : value(0) {}

  uint64_t operator()() const { return value; }
  uint32_t group() const { return value >> 16; }
  uint32_t counter() const { return (value & 0x0FFFF); }
  bool operator<(const MetricId &o) const { return value < o.value; }
  bool operator==(const MetricId &o) const { return value == o.value; }
 private:
  // low 16 bits are the counter number
  // middle 32 bits are the group
  // high 4 bits are the mask
  uint64_t value;
};

// Decorates Selection numbers with a mask, so they can never be
// confused with any other Id
class SelectionId {
 public:
  explicit SelectionId(uint32_t selectionNumber) {
    assert(((selectionNumber & ID_PREFIX_MASK) == 0) ||
           ((selectionNumber & ID_PREFIX_MASK) == SELECTION_ID_PREFIX));
    value = SELECTION_ID_PREFIX | selectionNumber;
  }

  uint32_t operator()() const { return value; }
  uint32_t index() const { return value & (~ID_PREFIX_MASK); }
 private:
  uint32_t value;
};

// Decorates Experiment numbers with a mask, so they can never be
// confused with any other Id
class ExperimentId {
 public:
  ExperimentId() : value(0) {}
  explicit ExperimentId(uint32_t experimentNumber) {
    assert(((experimentNumber & ID_PREFIX_MASK) == 0) ||
           ((experimentNumber & ID_PREFIX_MASK) == EXPERIMENT_ID_PREFIX));
    value = EXPERIMENT_ID_PREFIX | experimentNumber;
  }

  uint32_t operator()() const { return value; }
  uint32_t count() const { return value & (~ID_PREFIX_MASK); }
 private:
  uint32_t value;
};


struct MetricSeries {
  MetricId metric;
  std::vector<float> data;
};

struct ShaderAssembly {
  std::string shader;
  std::string ir;
  std::string ssa;
  std::string nir;
  std::string simd8;
  std::string simd16;
};

class OnFrameRetrace {
 public:
  typedef std::vector<unsigned char> uvec;
  virtual void onFileOpening(bool finished,
                             uint32_t percent_complete) = 0;
  virtual void onShaderAssembly(RenderId renderId,
                                const ShaderAssembly &vertex,
                                const ShaderAssembly &fragment,
                                const ShaderAssembly &tess_control,
                                const ShaderAssembly &tess_eval) = 0;
  virtual void onRenderTarget(RenderId renderId, RenderTargetType type,
                              const uvec & pngImageData) = 0;
  virtual void onMetricList(const std::vector<MetricId> &ids,
                            const std::vector<std::string> &names) = 0;
  virtual void onMetrics(const MetricSeries &metricData,
                         ExperimentId experimentCount) = 0;
  virtual void onShaderCompile(RenderId renderId,
                               ExperimentId experimentCount,
                               bool status,
                               const std::string &errorString) = 0;
  virtual void onApi(RenderId renderId,
                     const std::vector<std::string> &api_calls) = 0;
};

class IFrameRetrace {
 public:
  virtual ~IFrameRetrace() {}
  virtual void openFile(const std::string &filename,
                        uint32_t frameNumber,
                        OnFrameRetrace *callback) = 0;
  virtual void retraceRenderTarget(RenderId renderId,
                                   int render_target_number,
                                   RenderTargetType type,
                                   RenderOptions options,
                                   OnFrameRetrace *callback) const = 0;
  virtual void retraceShaderAssembly(RenderId renderId,
                                     OnFrameRetrace *callback) = 0;
  virtual void retraceMetrics(const std::vector<MetricId> &ids,
                              ExperimentId experimentCount,
                              OnFrameRetrace *callback) const = 0;
  virtual void replaceShaders(RenderId renderId,
                              ExperimentId experimentCount,
                              const std::string &vs,
                              const std::string &fs,
                              OnFrameRetrace *callback) = 0;
  virtual void retraceApi(RenderId renderId,
                          OnFrameRetrace *callback) = 0;
};

class FrameState {
 private:
  // not needed yet
  // StateTrack tracker;
  // trace::RenderBookmark frame_start;
  // std::vector<trace::RenderBookmark> renders;
  int render_count;
 public:
  FrameState(const std::string &filename,
             int framenumber);
  int getRenderCount() const { return render_count; }
};

} /* namespace glretrace */


#endif /* _GLFRAME_RETRACE_INTERFACE_HPP_ */
