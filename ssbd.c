// Spooky Scoreboard Daemon (ssbd)
// https://scoreboard.web.net
// Greg MacKenzie (greg@web.net)

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <cJSON.h>
#include <sys/inotify.h>
#include <sys/time.h>
#include <curl/curl.h>
#include <X11/Xlib.h>
#include <X11/xpm.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include <fontconfig/fontconfig.h>

#define VERSION "0.1"
#define MAX_MACHINE_ID_LEN 36
#define GAME_PATH "/game"

typedef struct {
  char code[4][5];
  uint8_t num_players;
  uint8_t shown;
} login_codes_t;

static CURL *curl;
static volatile sig_atomic_t run = 0;
static char mid[MAX_MACHINE_ID_LEN + 1];
static uint32_t games_played = 0;
static pthread_t ping_tid, login_codes_tid;
static pthread_cond_t ping_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t ping_mutex = PTHREAD_MUTEX_INITIALIZER;
static Display *display;
static login_codes_t login_codes;

static void cleanup(int rc)
{
  printf("Exiting...\n");

  if (ping_tid)
    pthread_mutex_lock(&ping_mutex);

  run = 0;

  if (ping_tid) {
    pthread_cond_signal(&ping_cond);
    pthread_mutex_unlock(&ping_mutex);
    pthread_join(ping_tid, NULL);
  }

  if (login_codes_tid)
    pthread_join(login_codes_tid, NULL);

  if (display)
    XCloseDisplay(display);

  if (curl) {
    curl_easy_cleanup(curl);
    curl_global_cleanup();
  }

  exit(rc);
}

static long curl_post(CURL *cp, const char *endpoint, const char *data)
{
  CURLcode cc;
  long rc = 0;
  struct curl_slist *hdrs = NULL;

  hdrs = curl_slist_append(hdrs, "Content-Type: text/plain");
  curl_easy_setopt(cp, CURLOPT_HTTPHEADER, hdrs);
  curl_easy_setopt(cp, CURLOPT_URL, endpoint);
  curl_easy_setopt(cp, CURLOPT_POSTFIELDS, data);
  curl_easy_setopt(cp, CURLOPT_TIMEOUT, 10L);
#ifdef DEBUG
  curl_easy_setopt(cp, CURLOPT_SSL_VERIFYHOST, 0);
  curl_easy_setopt(cp, CURLOPT_SSL_VERIFYPEER, 0);
#endif

  if ((cc = curl_easy_perform(cp)) != CURLE_OK) {
    fprintf(stderr, "CURL failed: %s\n", curl_easy_strerror(cc));
    return -1;
  }

  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &rc);
  return rc;
}

static void send_score()
{
  FILE *fp;
  long fsize;
  char *post = NULL;
  const char *endpoint = "https://scoreboard.web.net/spooky/score";

  if (!(fp = fopen("/game/highscores.config", "r"))) {
    perror("Failed to open highscores file");
    cleanup(1);
  }

  fseek(fp, 0, SEEK_END);
  fsize = ftell(fp);
  rewind(fp);

  if (!(post = (char *)malloc(MAX_MACHINE_ID_LEN + fsize + 1))) {
    perror("Cannot allocate post buffer memory");
    cleanup(1);
  }

  memset(post, 0, MAX_MACHINE_ID_LEN + fsize + 1);
  memcpy(post, mid, MAX_MACHINE_ID_LEN);
  size_t bytes = fread(&post[MAX_MACHINE_ID_LEN], 1, fsize, fp);
  fclose(fp);

  if (bytes != fsize) {
    perror("Failed to read highscores");
    free(post);
    cleanup(1);
  }

  long rc = curl_post(curl, endpoint, post);
  if (rc == 200)
    printf("Uploaded highscores.\n");
  else
    fprintf(stderr, "Failed to upload highscores: %ld\n", rc);

  free(post);
}

static void set_games_played(uint32_t *gp_ptr)
{
  FILE *fp;
  long fsize;
  char *buf = NULL;
  *gp_ptr = 0;

  if (!(fp = fopen("/game/_game_audits.json", "r"))) {
    perror("Failed to open game audits file");
    return;
  }

  fseek(fp, 0, SEEK_END);
  fsize = ftell(fp);
  rewind(fp);

  if (!(buf = (char *)malloc(fsize))) {
    perror("Failed to allocate memory for game audits buffer");
    fclose(fp);
    return;
  }

  size_t bytes = fread(buf, 1, fsize, fp);
  fclose(fp);

  if (bytes != fsize) {
    perror("Failed reading game audit file");
    free(buf);
    return;
  }

  cJSON *root = cJSON_Parse(buf);
  free(buf);

  if (!root) {
    const char *errptr = cJSON_GetErrorPtr();
    if (errptr != NULL) fprintf(stderr, "JSON error: %s\n", errptr);
    return;
  }

  cJSON *gp = cJSON_GetObjectItem(root, "games_played");
  cJSON *gpv = cJSON_GetObjectItem(gp, "value");

  if (gpv && cJSON_IsNumber(gpv))
    *gp_ptr = gpv->valueint;

  printf("Set games played: %u\n", *gp_ptr);
  cJSON_Delete(root);
}

static void *show_login_code(void *arg)
{
  uint8_t display_secs = 20;
  struct timeval start_time, current_time, update_time;
  login_codes_t *login_code = (login_codes_t *)arg;
  Window window;
  XftDraw *xft_draw;
  XftFont *xft_font;
  XftColor xft_color;
  Pixmap qr_pixmap;
  XpmAttributes xpm_attr;

  int screen = DefaultScreen(display);

  int w = DisplayWidth(display, screen);
  int h = DisplayHeight(display, screen);

  int ww = 500;
  int wh = 230;

  window = XCreateSimpleWindow(
    display,
    RootWindow(display, screen),
    (w - ww) / 2,
    (h - wh) / 2,
    ww,
    wh,
    1,
    BlackPixel(display, screen),
    WhitePixel(display, screen)
  );

  XMapWindow(display, window);
  XRaiseWindow(display, window);

  int rc = XpmReadFileToPixmap(display, window, "/game/tmp/qrcode.xpm", &qr_pixmap, NULL, &xpm_attr);
  if (rc != XpmSuccess) {
    fprintf(stderr, "Failed to create pixmap: %s\n", XpmGetErrorString(rc));
    cleanup(1);
  }

  GC gc = XCreateGC(display, window, 0, NULL);

  FcBool result = FcConfigAppFontAddFile(
    FcConfigGetCurrent(),
    (const FcChar8 *)"/game/code/assets/fonts/Atari_Hanzel.ttf"
  );

  if (!result) {
    fprintf(stderr, "FontConfig: Failed to load font.\n");
    cleanup(1);
  }

  xft_font = XftFontOpenName(display, screen, "Hanzel:size=21");
  if (!xft_font) {
    fprintf(stderr, "Xft: Unable to open TTF font.\n");
    cleanup(1);
  }

  XftColorAllocName(
    display,
    DefaultVisual(display, screen),
    DefaultColormap(display, screen),
    "black",
    &xft_color
  );

  xft_draw = XftDrawCreate(
    display,
    window,
    DefaultVisual(display, screen),
    DefaultColormap(display, screen)
  );

  XSelectInput(display, window, ExposureMask);
  gettimeofday(&start_time, NULL);
  update_time = start_time;

  while (1) {
    if (XPending(display) > 0) {
      XEvent event;
      XNextEvent(display, &event);

      if (event.type == Expose && event.xexpose.count == 0) {
        XCopyArea(display, qr_pixmap, window, gc, 0, 0, xpm_attr.width, xpm_attr.height, ww - 210, 10);
        XClearArea(display, window, 0, 0, ww - 210, 40 * 4, 0);

        for (int i = 0, ty = 40; i < login_code->num_players; i++, ty += 40) {
          char code[15];
          snprintf(code, sizeof(code), "Player %d: %s", i + 1, login_code->code[i]);
          XftDrawString8(xft_draw, &xft_color, xft_font, 10, ty, (XftChar8 *)code, strlen(code));
        }
      }
    }

    gettimeofday(&current_time, NULL);
    if (current_time.tv_sec - start_time.tv_sec >= display_secs)
      break;

    if (current_time.tv_sec - update_time.tv_sec >= 1) {
      char countdown[3];

      update_time = current_time;
      snprintf(countdown, sizeof(countdown), "%d", (int)(display_secs - (current_time.tv_sec - start_time.tv_sec)));

      XClearArea(display, window, 0, wh - 30, 40, 40, 1);
      XftDrawString8(xft_draw, &xft_color, xft_font, 5, wh - 10, (XftChar8 *)countdown, strlen(countdown));
    }
  }

  XFreePixmap(display, qr_pixmap);
  XftDrawDestroy(xft_draw);

  XftColorFree(
    display,
    DefaultVisual(display, screen),
    DefaultColormap(display, screen),
    &xft_color
  );

  XftFontClose(display, xft_font);
  XDestroyWindow(display, window);
  XFlush(display);

  login_code->shown = 0;
  pthread_detach(login_codes_tid);
}

static size_t set_login_code(const char *buf, size_t size, size_t nmemb, login_codes_t *login_codes)
{
  // @todo Check what we expect on content length header?
  size_t bytes = size * nmemb;

  for (size_t i = 0; i < bytes / 4; i++) {
    memcpy(login_codes->code[i], buf, 4);
    login_codes->code[i][4] = '\0';
    buf += 4;
  }

  return bytes;
}

static void open_player_spot()
{
  uint32_t now_played;
  static uint32_t last_played = 0;
  uint8_t gamediff;
  char *post = NULL;
  const char *endpoint = "https://scoreboard.web.net/spooky/spot";

  set_games_played(&now_played);
  gamediff = now_played - games_played;

  if (gamediff == 0 || gamediff > 4 || now_played == last_played)
    return;

  size_t p_len = MAX_MACHINE_ID_LEN + 4;
  if (!(post = (char *)malloc(p_len))) {
    perror("Cannot allocate memory for post");
    return;
  }

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, set_login_code);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &login_codes);

  snprintf(post, p_len, "%s%d", mid, gamediff);
  long rc = curl_post(curl, endpoint, post);

  if (rc == 200) {
    printf("Number of players: %u\n", gamediff);
    login_codes.num_players = gamediff;

    if (login_codes.shown == 0) {
      pthread_create(&login_codes_tid, NULL, show_login_code, &login_codes);
      login_codes.shown = 1;
    }
  }
  else
    fprintf(stderr, "Failed to open player spot: %ld\n", rc);

  free(post);
  last_played = now_played;
}

static void process_event(char *buf, ssize_t bytes)
{
  char *ptr = buf;
  while (ptr < buf + bytes) {
    struct inotify_event *evt = (struct inotify_event *)ptr;

    if (evt->len > 0) {
      printf("Event: %s\n", evt->name);

      if (strcmp(evt->name, "highscores.config") == 0) {
        send_score();
        set_games_played(&games_played);
        break;
      }

      if (strcmp(evt->name, "_game_audits.json") == 0) {
        open_player_spot();
        break;
      }
    }

    ptr += sizeof(struct inotify_event) + evt->len;
  }
}

static void watch()
{
  int fd, wd;
  char buf[1024];

  if ((fd = inotify_init()) == -1) {
    perror("Failed inotify_init()");
    cleanup(1);
  }

  if ((wd = inotify_add_watch(fd, GAME_PATH, IN_CLOSE_WRITE)) == -1) {
    perror("Failed inotify_add_watch()");
    cleanup(1);
  }

  while (run) {
    ssize_t bytes = read(fd, buf, sizeof(buf));

    if (bytes == -1)
      perror("Failed reading event");
    else {
      printf("Processing event...\n");
      process_event(buf, bytes);
    }

    fflush(stderr);
    fflush(stdout);
  }
}

size_t write_qrcode(void *ptr, size_t size, size_t nmemb, FILE *fp)
{
  size_t bytes = fwrite(ptr, size, nmemb, fp);
  return bytes;
}

static void get_qrcode()
{
  FILE *fp;
  char *post = NULL;
  const char *endpoint = "https://scoreboard.web.net/spooky/qr";

  if (!(post = (char *)malloc(MAX_MACHINE_ID_LEN + 1))) {
    perror("Cannot allocate memory for post");
    cleanup(1);
  }

  if (!(fp = fopen("/game/tmp/qrcode.xpm", "wb"))) {
    perror("Unable to open qrcode.xpm");
    cleanup(1);
  }

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_qrcode);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

  snprintf(post, MAX_MACHINE_ID_LEN + 1, "%s", mid);
  long rc = curl_post(curl, endpoint, post);

  if (rc == 200)
    printf("Downloaded QR Code.\n");
  else {
    fprintf(stderr, "Failed to download QR Code: %ld\n", rc);
    cleanup(1);
  }

  fclose(fp);
  free(post);
}

static void load_machine_id()
{
  FILE *fp;

  if (!(fp = fopen("/.ssbd_mid", "r"))) {
    perror("Failed to load machine id file");
    cleanup(1);
  }

  size_t bytes = fread(mid, 1, MAX_MACHINE_ID_LEN, fp);
  if (bytes != MAX_MACHINE_ID_LEN) {
    perror("Failed to read machine id");
    fclose(fp);
    cleanup(1);
  }

  fclose(fp);
  mid[MAX_MACHINE_ID_LEN] = '\0';
}

size_t register_callback(const char *ptr, size_t size, size_t nmemb, char *data)
{
  size_t bytes = size * nmemb;

  if (bytes != MAX_MACHINE_ID_LEN) {
    perror("Failed to register machine");
    cleanup(1);
  }

  strncpy(data, ptr, bytes);
  return bytes;
}

static void register_machine(const char *code)
{
  const char *endpoint = "https://scoreboard.web.net/spooky/register";

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, register_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, mid);
  long rc = curl_post(curl, endpoint, code);

  if (rc != 200) {
    fprintf(stderr, "Failed to register machine: %ld\n", rc);
    cleanup(1);
  }

  printf("%s\n", mid);
  curl_easy_cleanup(curl);
  curl_global_cleanup();
}

static void *send_ping()
{
  CURL *cp;
  struct timeval now;
  struct timespec timeout;
  const char *endpoint = "https://scoreboard.web.net/spooky/ping";

  if (!(cp = curl_easy_init())) {
    fprintf(stderr, "Failed to initialize curl ping handler: %s\n", curl_easy_strerror(CURLE_FAILED_INIT));
    cleanup(1);
  }

  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

  while (run) {
    long rescode = curl_post(cp, endpoint, mid);

    if (rescode != 200) {
      fprintf(stderr, "Failed to send ping: %ld\n", rescode);
      fflush(stderr);
    }

    gettimeofday(&now, NULL);
    timeout.tv_sec = now.tv_sec + 60;
    timeout.tv_nsec = now.tv_usec * 1000;

    pthread_mutex_lock(&ping_mutex);
    int rc = pthread_cond_timedwait(&ping_cond, &ping_mutex, &timeout);
    pthread_mutex_unlock(&ping_mutex);

    if (rc == ETIMEDOUT) continue;
    if (rc == 0) pthread_exit(NULL);
  }

  curl_easy_cleanup(cp);
  pthread_exit(NULL);
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
        perror("Failed to fork");
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
    perror("Cannot use -m and -r together");
    return 1;
  }

  if (run || reg) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    if (!(curl = curl_easy_init())) {
      fprintf(stderr, "Failed to initialize curl handler: %s\n", curl_easy_strerror(CURLE_FAILED_INIT));
      cleanup(1);
    }
  }

  if (reg)
    register_machine(argv[2]);

  if (run) {
    load_machine_id();

    // Wait a bit for the internet to come up.
    // Otherwise get_qrcode() will fail.
    // @todo Should probs implement something better.
    sleep(10);

    if (pthread_create(&ping_tid, NULL, send_ping, NULL) != 0) {
      perror("Failed to create ping thread");
      cleanup(1);
    }
    else
      printf("Ping thread created.\n");

    setenv("DISPLAY", ":0", 0);
    if ((display = XOpenDisplay(NULL)) == NULL) {
      fprintf(stderr, "Unable to open X display.\n");
      cleanup(1);
    }
    else
      printf("X11 display opened.\n");

    set_games_played(&games_played);
    get_qrcode();
    watch();
  }

  return 0;
}

// vi: expandtab shiftwidth=2 softtabstop=2 tabstop=2

