// Spooky Scoreboard Daemon (ssbd)
// Monitor highscores & uploads to scoreboard server.
// https://scoreboard.web.net
// Greg MacKenzie (greg@web.net)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <sys/time.h>
#include <curl/curl.h>

#define VERSION "0.1"
#define MAX_MACHINE_ID_LEN 36
#define GAME_PATH "/game"

static CURL *curl;
static volatile sig_atomic_t run = 0;
static char mid[MAX_MACHINE_ID_LEN + 1];
static pthread_t ping_tid;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void cleanup(int rc)
{
  printf("Exiting...\n");

  pthread_mutex_lock(&mutex);
  run = 0;
  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&mutex);
  pthread_join(ping_tid, NULL);

  if (curl) {
    curl_easy_cleanup(curl);
    curl_global_cleanup();
  }

  exit(rc);
}

static void score_send()
{
  FILE *fp;
  long fsize;
  char *post, *buf;
  struct curl_slist *hdr = NULL;
  const char *endpoint = "https://scoreboard.web.net/spooky/score";

  if (!(fp = fopen("/game/highscores.config", "r"))) {
    fprintf(stderr, "Failed to open highscores file: %s\n", strerror(errno));
    cleanup(1);
  }

  fseek(fp, 0, SEEK_END);
  fsize = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  if (!(buf = (char *)malloc(fsize + 1))) {
    fprintf(stderr, "Cannot allocate buffer memory: %s\n", strerror(errno));
    cleanup(1);
  }

  size_t bytes = fread(buf, 1, fsize, fp);
  if (bytes != fsize) {
    fprintf(stderr, "Failed to read highscores: %s\n", strerror(errno));
    fclose(fp);
    cleanup(1);
  }

  fclose(fp);
  buf[fsize] = '\0';

  size_t p_len = sizeof(mid) + fsize + 2;
  if (!(post = (char *)malloc(p_len))) {
    fprintf(stderr, "Cannot allocate memory for post data: %s\n", strerror(errno));
    free(buf);
    cleanup(1);
  }

  snprintf(post, p_len, "%s\n%s", mid, buf);

  hdr = curl_slist_append(hdr, "Content-Type: text/plain");

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdr);
  curl_easy_setopt(curl, CURLOPT_URL, endpoint);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post);

  CURLcode rc;
  if ((rc = curl_easy_perform(curl)) != CURLE_OK)
    fprintf(stderr, "CURL failed: %s\n", curl_easy_strerror(rc));

  free(post);
  free(buf);
}

static void process_event(char *buf, ssize_t bytes)
{
  char *ptr = buf;
  while (ptr < buf + bytes) {
    struct inotify_event *evt = (struct inotify_event *)ptr;

    if (evt->len > 0 && strcmp(evt->name, "highscores.config") == 0) {
      score_send(); 
      break;
    }

    ptr += sizeof(struct inotify_event) + evt->len;
  }
}

static void score_watch()
{
  int fd, wd;
  char buf[1024];

  if ((fd = inotify_init()) == -1) {
    fprintf(stderr, "Failed inotify_init(): %s\n", strerror(errno));
    cleanup(1);
  }

  if ((wd = inotify_add_watch(fd, GAME_PATH, IN_CLOSE_WRITE)) == -1) {
    fprintf(stderr, "Failed inotify_add_watch(): %s\n", strerror(errno));
    cleanup(1);
  }

  while (run) {
    ssize_t bytes = read(fd, buf, sizeof(buf));
    if (bytes == -1)
      fprintf(stderr, "Failed reading event: %s\n", strerror(errno));
    else
      process_event(buf, bytes);
  }
}

static void load_machine_id()
{
  FILE *fp;

  if (!(fp = fopen("/.ssbd_mid", "r"))) {
    fprintf(stderr, "Failed to load machine id file: %s\n", strerror(errno));
    cleanup(1);
  }

  size_t bytes = fread(mid, MAX_MACHINE_ID_LEN, 1, fp);
  if (bytes < 1) {
    fprintf(stderr, "Failed to read machine id: %s\n", strerror(errno));
    fclose(fp);
    cleanup(1);
  }

  fclose(fp);
  mid[MAX_MACHINE_ID_LEN] = '\0';
}

size_t register_callback(void *ptr, size_t size, size_t nmemb, void *data)
{
  size_t bytes = size * nmemb;
  if (bytes != MAX_MACHINE_ID_LEN) {
    fprintf(stderr, "Failed to register machine.\n");
    cleanup(1);
  }

  strncpy((char *)data, (char *)ptr, bytes);
  return bytes;
}

static void register_machine(const char *code)
{
  struct curl_slist *hdr = NULL;
  const char *endpoint = "https://scoreboard.web.net/spooky/register";

  hdr = curl_slist_append(hdr, "Content-Type: text/plain");

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdr);
  curl_easy_setopt(curl, CURLOPT_URL, endpoint);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, code);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, register_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, mid);

  CURLcode rc;
  if ((rc = curl_easy_perform(curl)) != CURLE_OK) {
    fprintf(stderr, "CURL failed: %s\n", curl_easy_strerror(rc));
    cleanup(1);
  }
  else {
    printf("%s\n", mid);
  }

  cleanup(0);
}

static void *ping_send()
{
  CURLcode cc;
  CURL *cp;
  struct curl_slist *hdr = NULL;
  struct timeval now;
  struct timespec timeout;
  const char *endpoint = "https://scoreboard.web.net/spooky/ping";

  if (!(cp = curl_easy_init())) {
    fprintf(stderr, "Failed to init ping: %s\n", curl_easy_strerror(CURLE_FAILED_INIT));
    cleanup(1);
  }

  hdr = curl_slist_append(hdr, "Content-Type: text/plain");
  curl_easy_setopt(cp, CURLOPT_HTTPHEADER, hdr);
  curl_easy_setopt(cp, CURLOPT_URL, endpoint);
  curl_easy_setopt(cp, CURLOPT_POSTFIELDS, mid);

  while (run) {
    int rc;

    if ((cc = curl_easy_perform(cp)) != CURLE_OK) {
      fprintf(stderr, "CURL ping failed: %s\n", curl_easy_strerror(cc));
      cleanup(1);
    }

    gettimeofday(&now, NULL);
    timeout.tv_sec = now.tv_sec + 60;
    timeout.tv_nsec = now.tv_usec * 1000;

    pthread_mutex_lock(&mutex);
    rc = pthread_cond_timedwait(&cond, &mutex, &timeout);
    pthread_mutex_unlock(&mutex);
    if (rc == ETIMEDOUT) continue;
  }

  curl_easy_cleanup(cp);
  return NULL;
}

static void ping_setup()
{
  int tr = pthread_create(&ping_tid, NULL, ping_send, NULL);
  if (tr != 0) {
    fprintf(stderr, "Failed to start ping thread.\n");
    cleanup(1);
  }
}

static void print_usage()
{
  fprintf(stderr, "Spooky Scoreboard Daemon (ssbd) v%s\n\n", VERSION);
  fprintf(stderr, " -m Monitor highscores & send to server\n");
  fprintf(stderr, " -d Run in background (daemon mode)\n");
  fprintf(stderr, " -r <code> Register your pinball machine\n");
  fprintf(stderr, " -h Displays this\n\n");
  exit(0);
}

static void sig_handler(int signo)
{
  cleanup(0);
}

int main(int argc, char **argv)
{
  int opt, reg = 0;
  struct sigaction sa;
  pid_t pid;

  if (argc < 2) {
    fprintf(stderr, "Missing parameter:\n-r or -m is required.\n\n");
    print_usage();
    return 1;
  }

  sa.sa_handler = sig_handler;
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  for (optind = 1;;) {
    if ((opt = getopt(argc, argv, "r:mdh")) == -1) break;

    switch (opt) {
    case 'h':
      print_usage();
      break;

    case 'r':
      if (argv[2] != NULL && strlen(argv[2]) == 4) reg = 1;
      break;

    case 'd':
      pid = fork();

      if (pid < 0) {
        fprintf(stderr, "Failed to fork: %s\n", strerror(errno));
        exit(1);
      }

      if (pid > 0) exit(0);
      break;

    case 'm':
      run = 1;
      break;
    }
  }

  if (run && reg) {
    fprintf(stderr, "Cannot use -r and -m together.\n");
    return 1;
  }

  if (run || reg) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    if (!(curl = curl_easy_init())) {
      fprintf(stderr, "Failed to init curl: %s\n", curl_easy_strerror(CURLE_FAILED_INIT));
      cleanup(1);
    }
  }

  if (reg) {
    register_machine(argv[2]);
  }

  if (run) {
    load_machine_id();
    ping_setup();
    score_watch();
  }

  return 0;
}

// vi: expandtab shiftwidth=2 softtabstop=2 tabstop=2

