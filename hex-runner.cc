/*
 * Copyright (c) 2018, Carnegie Mellon University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * hex_runner.cc a simple benchmark so we know how much snprintf
 * and sscanf would cost in cpu cycles.
 */

#include <assert.h>
#include <getopt.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <string>

/*
 * helper/utility functions, included inline here so we are self-contained
 * in one single source file...
 */
static char* argv0; /* argv[0], program name */

/*
 * default values
 */
#define DEF_TIMEOUT 120 /* alarm timeout */

/*
 * gs: shared global data (e.g. from the command line)
 */
static struct gs {
  int n;       /* number particles per rank */
  int depth;   /* depth of work */
  int timeout; /* alarm timeout */
} g;

/*
 * alarm signal handler
 */
static void sigalarm(int foo) {
  fprintf(stderr, "SIGALRM detected\n");
  fprintf(stderr, "Alarm clock\n");
  exit(1);
}

/*
 * usage
 */
static void usage(const char* msg) {
  if (msg) fprintf(stderr, "%s: %s\n", argv0, msg);
  fprintf(stderr, "usage: %s [options]\n", argv0);
  fprintf(stderr, "\noptions:\n");
  fprintf(stderr, "\t-d depth    depth of work [0,3]\n");
  fprintf(stderr, "\t-n num      number of particles\n");
  fprintf(stderr, "\t-t sec      timeout (alarm), in seconds\n");

  exit(1);
}

static uint64_t timeval_to_micros(const struct timeval* tv) {
  uint64_t t;
  t = static_cast<uint64_t>(tv->tv_sec) * 1000000;
  t += tv->tv_usec;
  return t;
}

static uint64_t now_micros() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return timeval_to_micros(&tv);
}

/*
 * forward prototype decls.
 */
static void encdec();

/*
 * main program.
 */
int main(int argc, char* argv[]) {
  uint64_t ts;
  int ch;

  argv0 = argv[0];

  /* we want lines, even if we are writing to a pipe */
  setlinebuf(stdout);

  /* setup default to zero/null, except as noted below */
  memset(&g, 0, sizeof(g));
  g.timeout = DEF_TIMEOUT;
  g.n = 8 << 20;
  g.depth = 1;

  while ((ch = getopt(argc, argv, "d:n:t:")) != -1) {
    switch (ch) {
      case 'd':
        g.depth = atoi(optarg);
        if (g.depth < 0 || g.depth > 3) usage("bad work depth");
        break;
      case 'n':
        g.n = atoi(optarg);
        if (g.n < 0) usage("bad num particles per rank");
        break;
      case 't':
        g.timeout = atoi(optarg);
        if (g.timeout < 0) usage("bad timeout");
        break;
      default:
        usage(NULL);
    }
  }

  printf("== Options:\n");
  printf(" > depth         = %d\n", g.depth);
  printf(" > num_particles = %d (%d M)\n", g.n, g.n >> 20);
  printf(" > timeout       = %d secs\n", g.timeout);
  printf("\n");

  signal(SIGALRM, sigalarm);
  alarm(g.timeout);
  printf("== Starting ...\n");
  ts = now_micros();
  encdec();

  printf("%.3f secs\n", (now_micros() - ts) / 1000000.0);
  printf("Done!\n");
  return 0;
}

static void binary2hex(std::string* input) {
  char tmp[3];
  const size_t len = input->size();
  if (len == 0) return;
  input->resize(len << 1);
  for (size_t i = 0; i < len; i++) {
    sprintf(tmp, "%02x", static_cast<unsigned char>(input->at(len - 1 - i)));
    memcpy(&input->at((len - 1 - i) << 1), tmp, 2);
  }
}

static void hex2binary(std::string* input) {
  char tmp[3];
  unsigned int h = 0;
  tmp[2] = 0;
  const size_t len = input->size() >> 1;
  if (len == 0) return;
  for (size_t i = 0; i < len; i++) {
    memcpy(tmp, &input->at(i << 1), 2);
    sscanf(tmp, "%02x", &h);
    input->at(i) = static_cast<char>(h);
    h = 0;
  }
  input->resize(len);
}

static void encdec() {
  char tmp[20];
  for (int i = 0; i < g.n; i++) {
    if (g.depth >= 1) {
      snprintf(tmp, sizeof(tmp), "%08x%08x", i, i);
      if (g.depth >= 2) {
        std::string x(tmp, 16);
        hex2binary(&x);
        if (g.depth >= 3) {
          std::string y = x;
          binary2hex(&y);
        }
      }
    }
  }
}
