#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <time.h>
#include <math.h>
#include <byteswap.h>
#include <pthread.h>
#include <assert.h>

#include <sodium.h>

#define BENCHMARK_ITERATIONS 2000

struct control_data
{
  bool done;
  pthread_mutex_t mutex;
};

static struct control_data *control = NULL;

#ifdef O64
static const char decoding[] = {62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -2, -1, -1, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};

// pthread_mutex_t print_result_mutex;

uint64_t b64_decode_filter(const char *src)
{
  size_t len = strlen(src);
  size_t end_shift = 64 - 6 * len;
  uint64_t result = 0;

  for (size_t i = 0; i < len; i++)
  {
    result <<= 6;
    result += (uint64_t)decoding[src[i] - '+'];
  }
  return __bswap_64(result << end_shift);
}

uint64_t b64_decode_mask(const char *src)
{
  size_t len = strlen(src);
  size_t end_shift = 64 - 6 * len;
  uint64_t result = -1;
  return __bswap_64(result << end_shift);
}

void mine_keys(char const *filters[], size_t filters_len, size_t iterations, bool stop_on_find)
{
  uint64_t publickey[crypto_box_PUBLICKEYBYTES / 8];
  uint64_t secretkey[crypto_box_SECRETKEYBYTES / 8];
  char b64publickey[128];
  char b64privatekey[128];
  uint64_t filter_values[filters_len];
  uint64_t filter_masks[filters_len];

  for (size_t i = 0; i < filters_len; i++)
  {
    filter_values[i] = b64_decode_filter(filters[i]);
    filter_masks[i] = b64_decode_mask(filters[i]);
  }

  if (iterations)
  {
    for (size_t c = 0; c < iterations; c++)
    {
      randombytes_buf((char *)secretkey, sizeof secretkey);
      crypto_scalarmult_base((char *)publickey, (char *)secretkey);

      for (size_t i = 0; i < filters_len; i++)
      {
        if (filter_values[i] == (publickey[0] & filter_masks[i]))
        {
          pthread_mutex_lock(&control->mutex);
          if (control->done)
          {
            // discard solution to guarantee single result to output
            pthread_mutex_unlock(&control->mutex);
            return;
          }
          sodium_bin2base64(b64publickey, sizeof b64publickey, (char *)publickey, sizeof publickey, sodium_base64_VARIANT_ORIGINAL);
          sodium_bin2base64(b64privatekey, sizeof b64privatekey, (char *)secretkey, sizeof publickey, sodium_base64_VARIANT_ORIGINAL);
          fprintf(stdout, "%s %s\n", b64publickey, b64privatekey);
          fflush(stdout);
          if (stop_on_find)
          {
            control->done = true;
            pthread_mutex_unlock(&control->mutex);
            return;
          }
          pthread_mutex_unlock(&control->mutex);
          break;
        }
      }
    }
  }
  else
  {
    for (;;)
    {
      randombytes_buf((char *)secretkey, sizeof secretkey);
      crypto_scalarmult_base((char *)publickey, (char *)secretkey);

      for (size_t i = 0; i < filters_len; i++)
      {
        if (filter_values[i] == (publickey[0] & filter_masks[i]))
        {
          pthread_mutex_lock(&control->mutex);
          if (control->done)
          {
            // discard solution to guarantee single result to output
            pthread_mutex_unlock(&control->mutex);
            return;
          }
          sodium_bin2base64(b64publickey, sizeof b64publickey, (char *)publickey, sizeof publickey, sodium_base64_VARIANT_ORIGINAL);
          sodium_bin2base64(b64privatekey, sizeof b64privatekey, (char *)secretkey, sizeof publickey, sodium_base64_VARIANT_ORIGINAL);
          fprintf(stdout, "%s %s\n", b64publickey, b64privatekey);
          fflush(stdout);
          if (stop_on_find)
          {
            control->done = true;
            pthread_mutex_unlock(&control->mutex);
            return;
          }
          pthread_mutex_unlock(&control->mutex);
          break;
        }
      }
    }
  }
}
#else
void mine_keys(char const *filters[], size_t filters_len, size_t iterations, bool stop_on_find)
{
  unsigned char publickey[crypto_box_PUBLICKEYBYTES];
  unsigned char secretkey[crypto_box_SECRETKEYBYTES];
  char b64publickey[128];
  char b64privatekey[128];
  size_t filters_lengths[filters_len];

  for (size_t i = 0; i < filters_len; i++)
  {
    filters_lengths[i] = strlen(filters[i]);
  }

  if (iterations)
  {
    for (size_t c = 0; c < iterations; c++)
    {
      randombytes_buf(secretkey, sizeof secretkey);
      crypto_scalarmult_base(publickey, secretkey);
      sodium_bin2base64(b64publickey, sizeof b64publickey, publickey, sizeof publickey, sodium_base64_VARIANT_ORIGINAL);

      for (size_t i = 0; i < filters_len; i++)
      {
        if (strncmp(filters[i], b64publickey, filters_lengths[i]) == 0)
        {
          pthread_mutex_lock(&control->mutex);
          if (control->done)
          {
            pthread_mutex_unlock(&control->mutex);
            // discard solution to guarantee single result to output
            return;
          }
          sodium_bin2base64(b64privatekey, sizeof b64privatekey, secretkey, sizeof publickey, sodium_base64_VARIANT_ORIGINAL);
          fprintf(stdout, "%s %s\n", b64publickey, b64privatekey);
          fflush(stdout);
          if (stop_on_find)
          {
            control->done = true;
            pthread_mutex_unlock(&control->mutex);
            return;
          }
          pthread_mutex_unlock(&control->mutex);
          break;
        }
      }
    }
  }
  else
  {
    for (;;)
    {
      randombytes_buf(secretkey, sizeof secretkey);
      crypto_scalarmult_base(publickey, secretkey);
      sodium_bin2base64(b64publickey, sizeof b64publickey, publickey, sizeof publickey, sodium_base64_VARIANT_ORIGINAL);

      for (size_t i = 0; i < filters_len; i++)
      {
        if (strncmp(filters[i], b64publickey, filters_lengths[i]) == 0)
        {
          pthread_mutex_lock(&control->mutex);
          if (control->done)
          {
            // discard solution to guarantee single result to output
            pthread_mutex_unlock(&control->mutex);
            return;
          }
          sodium_bin2base64(b64privatekey, sizeof b64privatekey, secretkey, sizeof publickey, sodium_base64_VARIANT_ORIGINAL);
          fprintf(stdout, "%s %s\n", b64publickey, b64privatekey);
          fflush(stdout);
          if (stop_on_find)
          {
            control->done = true;
            pthread_mutex_unlock(&control->mutex);
            return;
          }
          pthread_mutex_unlock(&control->mutex);
          break;
        }
      }
    }
  }
}
#endif

float benchmark(unsigned cpus, unsigned filters_len, unsigned iterations)
{
  unsigned pid, pids[cpus];
  struct timespec start, end;
  const char *test[] = {"########"}; // will never match

  clock_gettime(CLOCK_MONOTONIC_RAW, &start);

  for (size_t i = 0; i < cpus; i++)
  {
    pid = fork();
    if (pid == 0)
    {
      break;
    }
    else
    {
      pids[i] = pid;
    }
  }

  if (pid == 0)
  {
    mine_keys(test, filters_len || 1, iterations, false);
    exit(0);
  }
  else
  {
    for (size_t i = 0; i < cpus; i++)
    {
      waitpid(pids[i], NULL, 0);
    }
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);

    float delta_s = (float)(end.tv_sec - start.tv_sec) + (float)(end.tv_nsec - start.tv_nsec) / 1000000000;

    float keys_per_second = (iterations * cpus) / delta_s;
    fprintf(stderr, "Benchmark result: %.0f keys/second\n", keys_per_second);
    return keys_per_second;
  }
}

void sigchld_rcv(int x) {}

int main(int argc, char const *argv[])
{
  int cpus = sysconf(_SC_NPROCESSORS_ONLN);
  bool batch_mode_enabled = false;
  pid_t pid;
  pid_t pids[cpus];

  fprintf(stderr, "Multiprocessing: %d\n", cpus);
  fprintf(stderr, "Benchmarking...\n");

  float kps = benchmark(cpus, argc - 1, BENCHMARK_ITERATIONS * cpus);

  if (argc < 2)
  {
    printf("No argument passed, stopping.\n");
    return 0;
  }

  if (strcmp(argv[1], "-b") == 0)
  {
    batch_mode_enabled = true;
    fprintf(stderr, "Running batch mode with %d prefixes\n", argc - 2);
  }
  else if (argc > 2)
  {
    fprintf(stderr, "Too many prefixes. Did you mean to run batch mode? (use flag -b)\n");
    return -1;
  }

  for (size_t i = 1 + batch_mode_enabled; i < argc; i++)
  {
    size_t len = strlen(argv[i]);
#ifdef O64
    if (len > 12)
    {
      fprintf(stderr, "Value too long. 'O64' Optimization hard limits values to 12 characters. Then again, this would take millennia.\n");
      return -1;
    }
#endif
    if (len > 8)
    {
      fprintf(stderr, "Your values **may** be too log. Good luck.\n");
    }

    for (size_t c = 0; c < len; c++)
    {
      char chr = argv[i][c];
      if (chr != '+' && (chr < '/' || chr > '9') && (chr < 'A' || chr > 'Z') && (chr < 'a' || chr > 'z'))
      {
        fprintf(stderr, "Invalid value: '%s'. Only base64 characters are allowed.\n", argv[i]);
        return -1;
      }
    }

    float expected_wait_s = (float)pow(64, len) / kps;
    float expected_throuhput_s = 1 / expected_wait_s;
    if (batch_mode_enabled)
    {
      fprintf(stderr,
              "Expecting to find a key for '%s' once every %.1f seconds. [%.3f keys per hour]\n",
              argv[i], expected_wait_s, expected_throuhput_s * 60 * 60);
    }
    else
    {
      fprintf(stderr, "Estimated wait time: %.1f seconds.\n", expected_wait_s);
    }
  }

  // initialize shared memory
  int prot = PROT_READ | PROT_WRITE;
  int flags = MAP_SHARED | MAP_ANONYMOUS;
  control = mmap(NULL, sizeof(struct control_data), prot, flags, -1, 0);
  assert(control);

  control->done = false;

  // initialise mutex so it works properly in shared memory
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(&control->mutex, &attr);

  for (size_t i = 0; i < cpus; i++)
  {
    pid = fork();
    if (pid == 0)
    {
      break;
    }
    else
    {
      pids[i] = pid;
    }
  }
  if (pid)
  {
    if (!batch_mode_enabled)
    {
      signal(SIGCHLD, sigchld_rcv);
    }
    pause();
    for (size_t i = 0; i < cpus; i++)
    {
      killpg(pids[i], SIGKILL);
    }
    return 0;
  }

  mine_keys(&argv[1 + batch_mode_enabled], argc - 1 - batch_mode_enabled, 0, !batch_mode_enabled);
  return 0;
}
