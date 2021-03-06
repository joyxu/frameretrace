/**************************************************************************
 *
 * Copyright 2017 Intel Corporation
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

#include "glframe_qutil.hpp"

using glretrace::RenderSelection;
using glretrace::SelectionId;

void
glretrace::renderSelectionFromList(SelectionId id,
                                   const QList<int> &l,
                                   RenderSelection *rs) {
  rs->id = id;
  if (l.empty())
    return;
  RenderSeries &series = rs->series;
  series.clear();
  auto i = l.begin();
  RenderId begin(*i);
  RenderId end(*i + 1);

  while (true) {
    ++i;
    if (i == l.end()) {
      series.push_back(RenderSequence(begin, end));
      break;
    }
    if ((uint32_t)(*i) == end.index()) {
      // part of a contiguous sequence
      ++end;
      continue;
    }
    // else
    series.push_back(RenderSequence(begin, end));
    begin = RenderId(*i);
    end = RenderId(*i + 1);
  }
}

