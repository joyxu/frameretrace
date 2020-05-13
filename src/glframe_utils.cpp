/**************************************************************************
 *
 * Copyright 2020 Intel Corporation
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

#include "glframe_utils.hpp"

#include <climits>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <stdio.h>

int
glretrace::strtou(const char *str, unsigned *ret)
{
  char *end;

  errno = 0;
  unsigned long tmp = std::strtoul(str, &end, 0);
  if (*end != 0) {
    fprintf(stderr, "Can't parse '%s' as unsigned number\n", str);
    return -1;
  }

  if (errno) {
    fprintf(stderr, "Can't parse '%s' as unsigned number: %s\n", str,
        strerror(errno));
    return -1;
  }

  if (tmp > UINT_MAX) {
    fprintf(stderr, "'%s' is too large (> %u)\n", str, UINT_MAX);
    return -1;
  }

  *ret = (unsigned)tmp;
  return 0;
}

