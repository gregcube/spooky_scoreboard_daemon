// Spooky Scoreboard Monitoring Daemon

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/inotify.h>
#include <curl/curl.h>

#define VERSION "0.1"
#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + 16))
#define MAX_MACHINE_ID_LEN 36
#define GAME_PATH "/game"

static CURL *curl;
static volatile sig_atomic_t run = 0;
static char mid[MAX_MACHINE_ID_LEN + 1];

static void cleanup(int rc)
{
  run = 0;

  if (curl) {
    curl_easy_cleanup(curl);
    curl_global_cleanup();
  }

  exit(rc);
}

static void score_send()
{
  CURLcode rc;
  FILE *fp;
  long fsize;
  char *post, *buf;
  struct curl_slist *hdr = NULL;
  const char *endpoint = "https://scoreboard.web.net/spooky/score";

  curl_global_init(CURL_GLOBAL_DEFAULT);
  if (!(curl = curl_easy_init())) {
    fprintf(stderr, "Failed to init curl: %s\n", curl_easy_strerror(CURLE_FAILED_INIT));
    cleanup(1);
  }

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
  char buf[BUF_LEN];

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

    if (bytes == -1) {
      fprintf(stderr, "Failed reading event: %s\n", strerror(errno));
      cleanup(1);
    }

    if (bytes > 0)
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
  int opt;
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
      printf("todo: Implement this.\n");
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

  if (run) {
    load_machine_id();
    score_watch();
  }

  return 0;
}

// vi: expandtab shiftwidth=2 softtabstop=2 tabstop=2
