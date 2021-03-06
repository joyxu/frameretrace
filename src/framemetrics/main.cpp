// Copyright (C) Intel Corp.  2019.  All Rights Reserved.

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

#include <getopt.h>

#include <string>
#include <vector>

#include "os_time.hpp"
#include "glframe_glhelper.hpp"
#include "glframe_runner.hpp"
#include "glframe_logger.hpp"
#include "glframe_utils.hpp"
#include "metrics.hpp"

using glretrace::FrameRunner;
using metrics::PerfMetricDescriptor;

int main(int argc, char *argv[]) {
  std::vector<PerfMetricDescriptor> metrics_descs;
  std::vector<std::string> metrics_names;
  std::string frame_file, out_file;
  std::vector<unsigned> frames;
  const char *usage = "USAGE: framemetrics [-a|-d] [-i {interval}] -g {metrics_group} [-o {out_file}] "
                      "-f {trace} frame_start frame_end\n"
                      "\t-d metric events measured for draw calls\n"
                      "\t-a metrics measured for full trace\n"
                      "\t   default metric interval is per-frame\n"
                      "\t-i number of events to record before reporting metrics\n"
                      "\t-g format is \"Group Name:Optional,comma separated,individual metrics\"\n"
                      "\tPrints available metrics groups and metrics when no group is specified\n";

  int opt;
  unsigned event_interval = 1;
  FrameRunner::MetricInterval interval = FrameRunner::kPerFrame;
  while ((opt = getopt(argc, argv, "adg:f:p:o:i:h")) != -1) {
    switch (opt) {
      case 'a':
        if (interval == FrameRunner::kPerRender) {
          printf("ERROR: -a and -d are mutually exclusive\n%s", usage);
          return -1;
        }
        interval = FrameRunner::kPerTrace;
        continue;
      case 'd':
        if (interval == FrameRunner::kPerTrace) {
          printf("ERROR: -a and -d are mutually exclusive\n%s", usage);
          return -1;
        }
        interval = FrameRunner::kPerRender;
        continue;
      case 'i':
        if (glretrace::strtou(optarg, &event_interval))
          return -1;
        continue;
      case 'g':
        metrics_descs.push_back(PerfMetricDescriptor(optarg));
        continue;
      case 'f':
        frame_file = optarg;
        continue;
      case 'o':
        out_file = optarg;
        continue;
      case 'h':
      default: /* '?' */
        printf("%s", usage);
        // list supported metric groups
        return 0;
    }
  }

  for (int index = optind; index < argc; index++) {
    unsigned frame;

    if (glretrace::strtou(argv[index], &frame))
      return -1;

    frames.push_back(frame);
  }
  if (FILE *file = fopen(frame_file.c_str(), "r")) {
    fclose(file);
  } else {
    printf("ERROR: frame file not found: %s\n", frame_file.c_str());
    printf("%s", usage);
    return -1;
  }
  if (frames.empty()) {
    printf("ERROR: target frames not specified.\n");
    printf("%s", usage);
    return -1;
  }

  glretrace::GlFunctions::Init();
  glretrace::Logger::Create();
  glretrace::Logger::EnableStderr();
  glretrace::Logger::Begin();

  FrameRunner runner(frame_file, out_file, metrics_descs, frames.back(), interval, event_interval);

  if (metrics_descs.size() == 0) {
    runner.dumpGroupsAndCounters();
    return 0;
  }

  runner.advanceToFrame(frames[0]);
  runner.init();
  long long startTime = os::getTime();
  runner.run(frames[1]);
  long long endTime = os::getTime();
  const int frameNo = frames[1] - frames[0];
  float timeInterval = (endTime - startTime) * (1.0 / os::timeFrequency);

  std::cout << 
      "Rendered " << frameNo << " frames"
      " in " <<  timeInterval << " secs,"
      " average of " << (frameNo/timeInterval) << " fps\n";

  glretrace::Logger::Destroy();
  return 0;
}
