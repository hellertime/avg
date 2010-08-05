/** Compute various averages from stdin to stdout */
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * The cumulative average is computed as: CA[i+1] = (x[i+1] + (i * CA[i])) / (i + 1); where CA[0] = 0
 */
struct cumulative_average_t
{ 
  double average;
  double total;
  long count;
};

double cumulative_average(struct cumulative_average_t *s, double x)
{
  s->total += x;
  s->count++;
  s->average = s->total / s->count;
  return s->average;
}

double cma(struct cumulative_average_t *s)
{
  return s->average;
}

void cma_reset(struct cumulative_average_t *s)
{
  s->total = s->count = s->average = 0;
}

/**
 * The simple moving average is computed by:
 *  Setting a windows size, taking the cumulative average of the window size
 *  then taking the cumulative average of the window averages
 */
struct simple_moving_average_t
{
  struct cumulative_average_t current_window;
  struct cumulative_average_t simple_average;
  long window;
};

double simple_moving_average(struct simple_moving_average_t *s, double x)
{
  cumulative_average(&s->current_window, x);
  if(s->current_window.count == s->window)
  {
    cumulative_average(&s->simple_average, cma(&s->current_window));
    cma_reset(&s->current_window);
  }
  return cma(&s->simple_average);
}

double sma(struct simple_moving_average_t *s)
{
  return cma(&s->simple_average);
}

void sma_reset(struct simple_moving_average_t *s, long window)
{
  cma_reset(&s->current_window);
  cma_reset(&s->simple_average);
  s->window = window;
}

enum runtime_mode_t
{
  UNKNOWN_RUNTIME_MODE
, CUMULATIVE_AVERAGE
, SIMPLE_MOVING_AVERAGE
};

struct runtime_modes_t
{
  const char *mode;
  enum runtime_mode_t id;
};

static struct runtime_modes_t runtime_modes[] = 
{
  { "CMA", CUMULATIVE_AVERAGE }
, { "SMA", SIMPLE_MOVING_AVERAGE }
, { NULL, UNKNOWN_RUNTIME_MODE }
};

enum runtime_mode_t lookup_runtime_mode(const char *s)
{
  int i;
  for(i = 0; i < (sizeof(runtime_modes) / sizeof(runtime_modes[0])); i++)
    if(runtime_modes[i].mode && strcmp(runtime_modes[i].mode, s) == 0)
      return runtime_modes[i].id;

  return UNKNOWN_RUNTIME_MODE;
}

struct runtime_flags_t
{
  const char *data_filename;
  const char *program_name;
  enum runtime_mode_t runtime_mode;
  int show_intermediates;
  long window_size;
};

static struct runtime_flags_t runtime_flags = {0};

enum {
  SHOW_HELP = 'h'
, SHOW_INTERMEDIATES = 'I'
, SHOW_VERSION = 'V'
, SET_MODE = 'm'
, SET_WINDOW_SIZE = 'W'
};

static struct option runtime_options[] = 
{
  { "help", no_argument, NULL, SHOW_HELP }
, { "mode", required_argument, NULL, SET_MODE }
, { "show-intermediates", no_argument, NULL, SHOW_INTERMEDIATES }
, { "version", no_argument, NULL, SHOW_VERSION }
, { 0, 0, 0, 0}
};

/** TODO: autoconf these away */
#define PACKAGE_BUGREPORT "heller@teragram.com"
#define PACKAGE_NAME "avg"
#define VERSION "0.0.1"

int print_version(struct runtime_flags_t *f, FILE *out)
{
  out = out ? out : stdout;
  fprintf(out, "%s (" PACKAGE_NAME ") Version " VERSION "\n", f->program_name);
  fprintf(out, "Copyright 2010 Teragram\n");
  fprintf(out, "\n");
  fprintf(out, "Written by Chris Heller <heller@teragram.com>\n");
  return 0;
}

int print_usage(struct runtime_flags_t *f, FILE *out)
{
  out = out ? out : stdout;
  fprintf(out, "usage: %s [OPTIONS] [DATAFILE]\n", f->program_name);
  fprintf(out, "\n");
  fprintf(out, " Options:\n\n");
  fprintf(out, "    -m, --mode=MODE             set the runtime mode (default: CMA)\n");
  fprintf(out, "\n");
  fprintf(out, "                                  The following modes are supported:\n");
  fprintf(out, "                                    CMA -- Cumulative Moving Average\n");
  fprintf(out, "                                    SMA -- Simple Moving Average\n");
  fprintf(out, "\n");
  fprintf(out, "    -I, --show-intermediates    for compatible modes, show intermediate results\n");
  fprintf(out, "                                  not just the final result\n");
  fprintf(out, "    -W, --window-size=W        for compatible modes, set a windows size of W\n");
  fprintf(out, "\n\n");
  fprintf(out, "    -V, --version               show version information\n");
  fprintf(out, "    -h, --help                  show this help\n");
  fprintf(out, "\n\n");
  fprintf(out, "Report bugs to <" PACKAGE_BUGREPORT ">\n");
  return 0;
}

int parse_argv(struct runtime_flags_t *f, int argc, char **argv, int *last_arg_n)
{
  char code;
  opterr = 1;
  while((code = getopt_long(argc, argv, "hIm:VW:", runtime_options, NULL)) != -1)
  {
    switch(code)
    {
      case SET_MODE:
      {
        enum runtime_mode_t m = lookup_runtime_mode(optarg);
        if(m == UNKNOWN_RUNTIME_MODE)
        {
          fprintf(stderr, "Unkown runtime mode: %s\n\n", optarg);
          print_usage(f, stderr);
          exit(EXIT_FAILURE);
        }
        f->runtime_mode = m;
        break;
      }
      case SET_WINDOW_SIZE:
      {
        f->window_size = atol(optarg);
        break;
      }
      case SHOW_HELP:
      {
        print_usage(f, NULL);
        exit(EXIT_SUCCESS);
      }
      case SHOW_INTERMEDIATES:
      {
        f->show_intermediates = 1;
        break;
      }
      case SHOW_VERSION:
      {
        print_version(f, NULL);
        exit(EXIT_SUCCESS);
      }
      default:
      {
        fprintf(stderr, "Unregognized argument: %s\n\n", optarg);
        print_usage(f, stderr);
        exit(EXIT_FAILURE);
      }
    }
  }

  if(optind == (argc - 1))
  {
    f->data_filename = argv[optind];
  }

  if(last_arg_n)
    *last_arg_n = optind;

  return 0;
}

int main(int argc, char **argv)
{
  runtime_flags.program_name = "avg";
  runtime_flags.runtime_mode = CUMULATIVE_AVERAGE;
  runtime_flags.window_size = 10;

  parse_argv(&runtime_flags, argc, argv, NULL);

  FILE *in = stdin;
  if(runtime_flags.data_filename)
  {
    in = fopen(runtime_flags.data_filename, "r");
  }

  switch(runtime_flags.runtime_mode)
  {
    case CUMULATIVE_AVERAGE:
    {
      struct cumulative_average_t state;
      double x;

      cma_reset(&state);
      if(runtime_flags.show_intermediates)
      {
        while(fscanf(in, "%lf", &x) != EOF)
          printf("%lf\n", cumulative_average(&state, x));
      }
      else
      {
        while(fscanf(in, "%lf", &x) != EOF)
          cumulative_average(&state, x);

        printf("%lf\n", cma(&state));
      }
      break;
    }
    case SIMPLE_MOVING_AVERAGE:
    {
      struct simple_moving_average_t state;
      double x;
      long count = 0;

      sma_reset(&state, runtime_flags.window_size);
      while(fscanf(in, "%lf", &x) != EOF)
      {
        count++;
        simple_moving_average(&state, x);
        if(count == runtime_flags.window_size)
        {
          count = 0;
          if(runtime_flags.show_intermediates)
            printf("%lf\n", sma(&state));
        }
      }
      if(! runtime_flags.show_intermediates)
        printf("%lf\n", sma(&state));
      break;
    }
    case UNKNOWN_RUNTIME_MODE:
    {
      print_usage(&runtime_flags, stderr);
      exit(EXIT_FAILURE);
    }
  }

  if(runtime_flags.data_filename)
    fclose(in);

  return 0;
}
