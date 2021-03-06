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
 * Authors:
 *   Mark Janes <mark.a.janes@intel.com>
 **************************************************************************/

#ifndef _GLFRAME_RETRACE_STUB_HPP_
#define _GLFRAME_RETRACE_STUB_HPP_

#include <mutex>
#include <string>
#include <vector>

#include "glframe_retrace.hpp"

namespace glretrace {

class ThreadedRetrace;
class CancellationSocket;

// offloads the request to a thread which serializes request to the
// retrace process, and blocks on the result.
class FrameRetraceStub : public IFrameRetrace {
 public:
  // call once, to set up the retrace socket, and shut it down at
  // exit
  void Init(const char *host, int port);
  void Shutdown();
  void Stop();
  void Flush();

  virtual void openFile(const std::string &filename,
                        const std::vector<unsigned char> &md5,
                        uint64_t fileSize,
                        uint32_t frameNumber,
                        uint32_t frameCount,
                        OnFrameRetrace *callback);
  virtual void retraceRenderTarget(ExperimentId experimentCount,
                                   const RenderSelection &selection,
                                   RenderTargetType type,
                                   RenderOptions options,
                                   OnFrameRetrace *callback) const;
  virtual void retraceShaderAssembly(const RenderSelection &selection,
                                     ExperimentId experimentCount,
                                     OnFrameRetrace *callback);
  virtual void retraceMetrics(const std::vector<MetricId> &ids,
                              ExperimentId experimentCount,
                              OnFrameRetrace *callback) const;
  virtual void retraceAllMetrics(const RenderSelection &selection,
                                 ExperimentId experimentCount,
                                 OnFrameRetrace *callback) const;
  virtual void replaceShaders(RenderId renderId,
                              ExperimentId experimentCount,
                              const std::string &vs,
                              const std::string &fs,
                              const std::string &tessControl,
                              const std::string &tessEval,
                              const std::string &geom,
                              const std::string &comp,
                              OnFrameRetrace *callback);
  virtual void disableDraw(const RenderSelection &selection,
                           bool disable);
  virtual void simpleShader(const RenderSelection &selection,
                            bool simple);
  virtual void oneByOneScissor(const RenderSelection &selection,
                               bool scissor);
  virtual void wireframe(const RenderSelection &selection,
                         bool wireframe);
  virtual void texture2x2(const RenderSelection &selection,
                          bool texture_2x2);
  virtual void retraceApi(const RenderSelection &selection,
                          OnFrameRetrace *callback);
  virtual void retraceBatch(const RenderSelection &selection,
                            ExperimentId experimentCount,
                            OnFrameRetrace *callback);
  virtual void retraceUniform(const RenderSelection &selection,
                              ExperimentId experimentCount,
                              OnFrameRetrace *callback);
  virtual void setUniform(const RenderSelection &selection,
                          const std::string &name,
                          int index,
                          const std::string &data);
  virtual void retraceState(const RenderSelection &selection,
                            ExperimentId experimentCount,
                            OnFrameRetrace *callback);
  virtual void setState(const RenderSelection &selection,
                        const StateKey &item,
                        int offset,
                        const std::string &value);
  virtual void retraceTextures(const RenderSelection &selection,
                               ExperimentId experimentCount,
                               OnFrameRetrace *callback);
  virtual void revertExperiments();
  virtual void cancel(SelectionId selectionCount,
                      ExperimentId experimentCount) { assert(false); }

 private:
  mutable std::mutex m_mutex;
  mutable SelectionId m_current_rt_selection, m_current_met_selection,
    m_current_render_selection;
  mutable ExperimentId m_current_experiment;
  ThreadedRetrace *m_thread = NULL;
  CancellationSocket *m_cancellation = NULL;
};
}  // namespace glretrace

#endif  // _GLFRAME_RETRACE_STUB_HPP_
