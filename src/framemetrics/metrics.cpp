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

#include "metrics.hpp"

using metrics::PerfMetricDescriptor;


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

