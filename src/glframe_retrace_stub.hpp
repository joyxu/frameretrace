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

#ifndef _GLFRAME_RETRACE_STUB_HPP_
#define _GLFRAME_RETRACE_STUB_HPP_

#include "glframe_retrace.hpp"

namespace glretrace {

// offloads the request to a thread which serializes request to the
// retrace process, and blocks on the result.
class FrameRetraceStub : public IFrameRetrace {
 public:
    // call once, to set up the retrace socket, and shut it down at
    // exit
    static void Init(int port);
    static void Shutdown();

    virtual void openFile(const std::string &filename,
                          uint32_t frameNumber,
                          OnFrameRetrace *callback);
    virtual void retraceRenderTarget(RenderId renderId,
                                     int render_target_number,
                                     RenderTargetType type,
                                     RenderOptions options,
                                     OnFrameRetrace *callback) const;
    virtual void retraceShaderAssembly(RenderId renderId,
                                       OnFrameRetrace *callback);
 private:
};
}  // namespace glretrace

#endif // _GLFRAME_RETRACE_STUB_HPP_
