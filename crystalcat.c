#define _XOPEN_SOURCE 600
#include <stdio.h> 
/* #include <glib/gstdio.h> */ 
#include <fcntl.h> 
#include <termios.h> 	
#include <sys/ioctl.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <math.h> 
#include <gtk/gtk.h>
#include <glib.h> 
#include <cairo.h>
#include <webkit2/webkit2.h>
#include <jsc/jsc.h>
#include <sqlite3.h>
#include <time.h> 
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtksourceview/gtksource.h>
#pragma GCC diagnostic pop
#define ASSISTANT_WIDTH  32
#define ASSISTANT_HEIGHT 32
#define TOTAL_FRAMES     35 
#define MAX_RECENT_FILES 10
#define NUM_BANDS 32
#define UA_DEFAULT  "Mozilla/5.0 (X11; Crystalcat x86_64; rv:128.0) Gecko/20100101 Firefox/128.0"
#define UA_FIREFOX  "Mozilla/5.0 (X11; Linux x86_64; rv:128.0) Gecko/20100101 Firefox/128.0"
#define UA_CHROME   "Mozilla/5.0 (ipad; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36"
#define UA_EDGE     "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36 Edg/126.0.0.0"
#define UA_MOBILE   "Mozilla/5.0 (iPhone; CPU iPhone OS 17_5 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.5 Mobile/15E148 Safari/604.1"
#define UA_WINDOWS  "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/150.0.0.0 Safari/537.36"
#define UA_MACOS    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:152.0) Gecko/20100101 Firefox/152.0"
#define UA_ANDROID  "Mozilla/5.0 (Linux; Android 14; Pixel 8 Build/AP4A.250105.002) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/132.0.0.0 Mobile Safari/537.36"

/* Global state for script defusal */
static gboolean g_block_hostile_scripts = TRUE;

typedef struct {
    WebKitCookieManager *manager;
    SoupCookie *cookie;
    GtkWidget *name_entry;
    GtkWidget *value_entry;
    GtkWidget *domain_entry;
    GtkWidget *path_entry;
    GtkWidget *dialog;
} CookieEditContext;

typedef struct {
    GtkWidget *overlay;
    GtkWidget *drawing_area;
    float spectrum_data[NUM_BANDS];
    guint timer_id;
} VisualizerState;

typedef struct {
    char *filepath;
    char *display_name;
    time_t last_accessed;
} RecentFileItem;

typedef struct {
    RecentFileItem items[MAX_RECENT_FILES];
    int count;
    GtkWidget *parent_menu_or_container; /* Reference to GTK container (e.g., GtkMenu / GtkListBox) */
} RecentFilesWrapper;

/* Global state variable tracking the toggle status */
static gboolean g_cookie_spoofing_active = FALSE;

static RecentFilesWrapper g_recent_wrapper = {0};

/* Inline C-compatible 1-bit XBM data representing a terminal-style pet */

static unsigned char assistant_bits[] = {
	/* Prototype idea of rabbit */
   0x00, 0x00, 0x00, 0x00,  0x00, 0x30, 0x0c, 0x00,  0x00, 0x30, 0x0c, 0x00,  0x00, 0x30, 0x0c, 0x00,
   0x00, 0x38, 0x1c, 0x00,  0x00, 0x28, 0x14, 0x00,  0x00, 0x28, 0x14, 0x00,  0x00, 0x28, 0x14, 0x00,
   0x00, 0x38, 0x1c, 0x00,  0x00, 0xf0, 0x0f, 0x00,  0x00, 0xf8, 0x1f, 0x00,  0x00, 0xfc, 0x3f, 0x00,
   0x00, 0xcc, 0x33, 0x00,  0x00, 0xcc, 0x33, 0x00,  0x00, 0xfc, 0x3f, 0x00,  0x00, 0xf8, 0x1f, 0x00,
   0x00, 0xb0, 0x0b, 0x00,  0x00, 0xe0, 0x07, 0x00,  0x00, 0xf0, 0x0f, 0x00,  0x00, 0xf8, 0x1f, 0x00,
   0x00, 0xfc, 0x3f, 0x00,  0x00, 0xfe, 0x7f, 0x00,  0x00, 0xce, 0x73, 0x00,  0x00, 0xcf, 0xf3, 0x00,
   0x00, 0xc7, 0xe3, 0x00,  0x00, 0xde, 0x7b, 0x00,  0x00, 0xfc, 0x3f, 0x00,  0x00, 0xf8, 0x1f, 0x00,
   0x00, 0xf0, 0x0f, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00
};

/* Frame 0: Standard Idle State (Neutral) */
static unsigned char frame_0[] = {
   0x00,0x00,0x00,0x00, 0x00,0x30,0x0c,0x00, 0x00,0x30,0x0c,0x00, 0x00,0x30,0x0c,0x00,
   0x00,0x38,0x1c,0x00, 0x00,0x28,0x14,0x00, 0x00,0x28,0x14,0x00, 0x00,0x28,0x14,0x00,
   0x00,0x38,0x1c,0x00, 0x00,0xf0,0x0f,0x00, 0x00,0xf8,0x1f,0x00, 0x00,0xfc,0x3f,0x00,
   0x00,0xcc,0x33,0x00, 0x00,0xcc,0x33,0x00, 0x00,0xfc,0x3f,0x00, 0x00,0xf8,0x1f,0x00,
   0x00,0xb0,0x0b,0x00, 0x00,0xe0,0x07,0x00, 0x00,0xf0,0x0f,0x00, 0x00,0xf8,0x1f,0x00,
   0x00,0xfc,0x3f,0x00, 0x00,0xfe,0x7f,0x00, 0x00,0xce,0x73,0x00, 0x00,0xcf,0xf3,0x00,
   0x00,0xc7,0xe3,0x00, 0x00,0xde,0x7b,0x00, 0x00,0xfc,0x3f,0x00, 0x00,0xf8,0x1f,0x00,
   0x00,0xf0,0x0f,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00
};

/* Frame 1: Ear Twitch (Ears lean outward) */
static unsigned char frame_1[] = {
   0x00,0x00,0x00,0x00, 0x00,0x60,0x06,0x00, 0x00,0x60,0x06,0x00, 0x00,0x60,0x06,0x00,
   0x00,0x70,0x0e,0x00, 0x00,0x50,0x0a,0x00, 0x00,0x50,0x0a,0x00, 0x00,0x50,0x0a,0x00,
   0x00,0x70,0x0e,0x00, 0x00,0xf0,0x0f,0x00, 0x00,0xf8,0x1f,0x00, 0x00,0xfc,0x3f,0x00,
   0x00,0xcc,0x33,0x00, 0x00,0xcc,0x33,0x00, 0x00,0xfc,0x3f,0x00, 0x00,0xf8,0x1f,0x00,
   0x00,0xb0,0x0b,0x00, 0x00,0xe0,0x07,0x00, 0x00,0xf0,0x0f,0x00, 0x00,0xf8,0x1f,0x00,
   0x00,0xfc,0x3f,0x00, 0x00,0xfe,0x7f,0x00, 0x00,0xce,0x73,0x00, 0x00,0xcf,0xf3,0x00,
   0x00,0xc7,0xe3,0x00, 0x00,0xde,0x7b,0x00, 0x00,0xfc,0x3f,0x00, 0x00,0xf8,0x1f,0x00,
   0x00,0xf0,0x0f,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00
};

/* Frame 2: Half-Blink (Eyes narrowing) */
static unsigned char frame_2[] = {
   0x00,0x00,0x00,0x00, 0x00,0x30,0x0c,0x00, 0x00,0x30,0x0c,0x00, 0x00,0x30,0x0c,0x00,
   0x00,0x38,0x1c,0x00, 0x00,0x28,0x14,0x00, 0x00,0x28,0x14,0x00, 0x00,0x28,0x14,0x00,
   0x00,0x38,0x1c,0x00, 0x00,0xf0,0x0f,0x00, 0x00,0xf8,0x1f,0x00, 0x00,0xfc,0x3f,0x00,
   0x00,0xfc,0x3f,0x00, 0x00,0xfc,0x3f,0x00, 0x00,0xfc,0x3f,0x00, 0x00,0xf8,0x1f,0x00,
   0x00,0xb0,0x0b,0x00, 0x00,0xe0,0x07,0x00, 0x00,0xf0,0x0f,0x00, 0x00,0xf8,0x1f,0x00,
   0x00,0xfc,0x3f,0x00, 0x00,0xfe,0x7f,0x00, 0x00,0xce,0x73,0x00, 0x00,0xcf,0xf3,0x00,
   0x00,0xc7,0xe3,0x00, 0x00,0xde,0x7b,0x00, 0x00,0xfc,0x3f,0x00, 0x00,0xf8,0x1f,0x00,
   0x00,0xf0,0x0f,0x00, 0x00,0x02,0x01,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00
};

/* Frame 3: Full Blink + Nose Twitch (Eyes flat lines, nose shifted) */
static unsigned char frame_3[] = {
   0x00,0x00,0x00,0x00, 0x00,0x30,0x0c,0x00, 0x00,0x30,0x0c,0x00, 0x00,0x30,0x0c,0x00,
   0x00,0x38,0x1c,0x00, 0x00,0x28,0x14,0x00, 0x00,0x28,0x14,0x00, 0x00,0x28,0x14,0x00,
   0x00,0x38,0x1c,0x00, 0x00,0xf0,0x0f,0x00, 0x00,0xf8,0x1f,0x00, 0x00,0xfc,0x3f,0x00,
   0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0xfc,0x3f,0x00, 0x00,0xf8,0x1f,0x00,
   0x00,0xd0,0x0b,0x00, 0x00,0xb0,0x0d,0x00, 0x00,0xf0,0x0f,0x00, 0x00,0xf8,0x1f,0x00,
   0x00,0xfc,0x3f,0x00, 0x00,0xfe,0x7f,0x00, 0x00,0xce,0x73,0x00, 0x00,0xcf,0xf3,0x00,
   0x00,0xc7,0xe3,0x00, 0x00,0xde,0x7b,0x00, 0x00,0xfc,0x3f,0x00, 0x00,0xf8,0x1f,0x00,
   0x00,0xf0,0x0f,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00
};

/* Frame 4: Return Recovery (Eyes opening back up) */
static unsigned char frame_4[] = {
   0x00,0x01,0x00,0x00, 0x00,0x30,0x0c,0x00, 0x00,0x30,0x0c,0x00, 0x00,0x30,0x0c,0x00,
   0x00,0x38,0x1c,0x00, 0x00,0x28,0x14,0x00, 0x00,0x28,0x14,0x00, 0x00,0x28,0x14,0x00,
   0x00,0x38,0x1c,0x01, 0x00,0xf0,0x0f,0x00, 0x00,0xf8,0x1f,0x00, 0x00,0xfc,0x3f,0x00,
   0x00,0xfc,0x3f,0x00, 0x00,0xfc,0x3f,0x00, 0x01,0xfc,0x3f,0x00, 0x00,0xf8,0x1f,0x00,
   0x00,0xb0,0x0b,0x00, 0x00,0xe0,0x07,0x00, 0x00,0xf0,0x0f,0x00, 0x00,0xf8,0x1f,0x00,
   0x00,0xfc,0x3f,0x00, 0x00,0xfe,0x7f,0x00, 0x00,0xce,0x73,0x00, 0x00,0xcf,0xf3,0x00,
   0x00,0xc7,0xe3,0x00, 0x00,0xde,0x7b,0x00, 0x00,0xfc,0x3f,0x00, 0x00,0xf8,0x1f,0x00,
   0x00,0xf0,0x0f,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00
};
/* Frame 5: Ear twitching glitching right eye blink */
static unsigned char frame_5[] = {
   0x00,0x00,0x00,0x00, 0x00,0x30,0x03,0x00, 0x00,0x30,0x07,0x00,  
   0x00,0x30,0x0f,0x01, 0x00,0x38,0x1c,0x00, 0x00,0x20,0x14,0x00,  
   0x00,0x28,0x14,0x00, 0x00,0x28,0x14,0x00, 0x00,0x38,0x1c,0x02,  
   0x00,0xf0,0x0f,0x00, 0x00,0xf8,0x1f,0x00, 0x00,0xfc,0x3f,0x00,
   0x00,0xcc,0x33,0x00, 0x00,0xcc,0x33,0x00, 0x00,0xfc,0x3f,0x00,  
   0x00,0xf8,0x1f,0x00, 0x00,0xb0,0x0b,0x00, 0x00,0xe0,0x07,0x00,  
   0x00,0xf0,0x0f,0x00, 0x00,0xf8,0x1f,0x00, 0x00,0xfc,0x3f,0x00,  
   0x00,0xfe,0x7f,0x00, 0x00,0xce,0x73,0x00, 0x00,0xcf,0xf3,0x01,
   0x00,0xc7,0xe3,0x00, 0x00,0xde,0x7b,0x00, 0x00,0xfc,0x3f,0x00,  
   0x00,0xf8,0x1f,0x00, 0x00,0xf0,0x0f,0x00, 0x00,0x00,0x00,0x00,  
   0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00
};

/* Frame 6: Left eye blinking no glitch. */ 
static unsigned char frame_6[] = {
   0x00,0x00,0x00,0x00, 0x00,0x31,0x03,0x00, 0x00,0x30,0x07,0x00,  
   0x00,0x30,0x0f,0x00, 0x00,0x38,0x1c,0x00, 0x00,0x28,0x04,0x00,  
   0x00,0x28,0x14,0x00, 0x00,0x28,0x14,0x00, 0x00,0x38,0x1c,0x00,  
   0x00,0xf0,0x0f,0x00, 0x00,0xf8,0x1f,0x00, 0x00,0xfc,0x3f,0x00,
   0x00,0xcc,0x33,0x00, 0x00,0xcc,0x33,0x00, 0x00,0xfc,0x3f,0x00,  
   0x00,0xf8,0x1f,0x00, 0x00,0xb0,0x0b,0x00, 0x00,0xe0,0x07,0x01,  
   0x00,0xf0,0x0f,0x00, 0x00,0xf8,0x1f,0x00, 0x00,0xfc,0x3f,0x00,  
   0x00,0xfe,0x7f,0x00, 0x00,0xce,0x73,0x00, 0x00,0xcf,0xf3,0x00,
   0x00,0xc7,0xe3,0x00, 0x00,0xde,0x7b,0x00, 0x00,0xfc,0x3f,0x00,  
   0x00,0xf8,0x1f,0x00, 0x00,0xf0,0x0f,0x00, 0x00,0x00,0x00,0x00,  
   0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00
};


/* Cat Frame 0: Idle State */
static unsigned char cat_frame_0[] = {
   0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x08,
   0x30, 0x00, 0x00, 0x0c, 0x70, 0x00, 0x00, 0x0e,
   0xf0, 0x00, 0x00, 0x0f, 0xf8, 0x01, 0x80, 0x1f,
   0x7c, 0x03, 0xc0, 0x3e, 0x3e, 0xff, 0xff, 0x7c,
   0x1e, 0xff, 0xff, 0x78, 0xff, 0xff, 0xff, 0xff,
   0xfe, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x7e, 0x7e, 0x00, 0x00, 0xff, 0xff, 0x00,
   0x00, 0xff, 0xff, 0x00, 0x00, 0x7e, 0x7e, 0x00,
   0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x01, 0xff,
   0x00, 0x00, 0x02, 0x7f, 0x00, 0x00, 0x7e, 0x3f,
   0x00, 0x00, 0x5a, 0x03, 0xff, 0x00, 0x7e, 0xff,
   0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00,
   0x00, 0xff, 0xff, 0x00, 0x00, 0xfc, 0x3f, 0x00,
   0x00, 0xf0, 0x0f, 0x00, 0x00, 0xc0, 0x03, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* Cat Running Frame 0: Legs Stretched */
static unsigned char cat_run_0[] = {
   0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x08,
   0x30, 0x00, 0x00, 0x0c, 0x70, 0x00, 0x00, 0x0e,
   0xf0, 0x00, 0x00, 0x0f, 0xf8, 0x01, 0x80, 0x1f,
   0x7c, 0x03, 0xc0, 0x3e, 0x3e, 0xff, 0xff, 0x7c,
   0x1e, 0xff, 0xff, 0x78, 0xff, 0xff, 0xff, 0xff,
   0xfe, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x7e, 0x00,
   0x00, 0x7e, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00,
   0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x01, 0xff,
   0x00, 0x00, 0x02, 0x7f, 0x00, 0x00, 0x7e, 0x3f,
   0x00, 0x00, 0x5a, 0x03, 0xff, 0x00, 0x7e, 0xff,
   0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00,
   0x00, 0xff, 0xff, 0x00, 0x00, 0xfc, 0x3f, 0x00,
   0x00, 0xf0, 0x0f, 0x00, 0x00, 0xc0, 0x03, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* Cat Running Frame 1: Legs Gathered */
static unsigned char cat_run_1[] = {
   0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x08,
   0x30, 0x00, 0x00, 0x0c, 0x70, 0x00, 0x00, 0x0e,
   0xf0, 0x00, 0x00, 0x0f, 0xf8, 0x01, 0x80, 0x1f,
   0x7c, 0x03, 0xc0, 0x3e, 0x3e, 0xff, 0xff, 0x7c,
   0x1e, 0xff, 0xff, 0x78, 0xff, 0xff, 0xff, 0xff,
   0xfe, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x7e, 0x00,
   0x00, 0x66, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00,
   0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x01, 0xff,
   0x00, 0x00, 0x02, 0x7f, 0x00, 0x00, 0x7e, 0x3f,
   0x00, 0x00, 0x5a, 0x03, 0xff, 0x00, 0x7e, 0xff,
   0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00,
   0x00, 0xff, 0xff, 0x00, 0x00, 0xfc, 0x3f, 0x00,
   0x00, 0xf0, 0x0f, 0x00, 0x00, 0xc0, 0x03, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* The pointer tracker arrays for our rabbit and cat */
static unsigned char *animation_sequence[TOTAL_FRAMES] = {
   assistant_bits, frame_0, frame_0, frame_0, frame_1, frame_2, frame_3, frame_4, frame_6, frame_6, frame_6, frame_6,
   frame_6, frame_6, frame_6, frame_5, frame_5, frame_5, frame_5, frame_5, frame_1, frame_1, frame_1, frame_0, frame_0, 
   frame_0, frame_0, frame_0, frame_0, frame_0, frame_1, frame_0, frame_1, frame_0, frame_1
};
static unsigned char *cat_animation_sequence[] = {  
	cat_frame_0, cat_run_0, cat_run_1 
};

/* --- Companion Mood Tracks --- */
typedef enum {
    MOOD_IDLE, MOOD_TYPING,
    MOOD_COMPILING, MOOD_ERROR   
} AssistantMood;

static AssistantMood current_mood = MOOD_IDLE;
static int typing_cooldown_counter = 0; 

/* --- Global State Management --- */
typedef struct {
    GtkWidget *window;
    GtkTextBuffer *editor_buffer;
    GtkWidget *editor_view; 
    GtkWidget *browser_view;
    GtkWidget *search_entry;
    GtkTextBuffer *console_buffer;
    GtkWidget *console_view;
    GtkWidget *sidebar_notebook;
    GtkTreeStore *tree_store;
    GtkTreeStore *recent_tree_store;
    GtkWidget    *recent_tree_view;   
    sqlite3 *db;
    char *current_filename; 
    GtkWidget *audio_popup;
    GtkWidget *audio_webview;
    GtkWidget *stream_window;
    GtkWidget *stream_webview;
    int master_fd;   
    GtkWidget *find_dialog;
    GtkWidget *find_entry;
    GtkCheckButton *case_check;
    GtkCheckButton *word_check;
    GtkWidget *statusbar;
    guint status_context_id;
    int color_toggle; 
    GtkCssProvider *status_css_provider; 
    GtkWidget *assistant_box;
    GtkWidget *assistant_label;
    GtkWidget *avatar_canvas;
    GtkWidget *cat_canvas;
    int current_frame;
    int rabbit_x; 
    int rabbit_y;
    int cat_x;
    int cat_y;
    int cat_frame;
    int cat_active; /* 1 if running, 0 if idle */ 
    int cat_remaining; /* Number of animation loops left to reset */
    int cat_animation_timer; 
    int cat_speed_divider;   
    int cat_loop_count;     
    GtkWidget *main_vpaned;
    WebKitWebView *embedded_audio_view;
    GtkWidget *audio_mode_item;
    GtkWidget *compile_button_widget;
    GtkWidget *btn_box;
    GtkTextBuffer *webkit_log_buffer;
    GtkWidget *webkit_log_dialog;
    GtkWidget *menu_time_label; 
} AppState;

/* --- Modular Prototypes Initialization & Cleanup ---*/
static void ui_apply_styles(void); 
static void app_init_state(AppState *app);
static void app_init_db(AppState *app);
static void app_cleanup(AppState *app);
/* UI Modules */
static void ui_open_audio_player(AppState *app, const char *url);
static void cb_on_stream_request(GtkWidget *widget, gpointer data);
static void ui_build_layout(AppState *app);
static void ui_update_statusbar(GtkTextBuffer *buffer, gpointer user_data);
static GtkWidget* ui_create_menu(AppState *app);
static GtkWidget* ui_create_search_section(AppState *app);
static GtkWidget* ui_create_workspace(AppState *app);
static GtkWidget* ui_create_console_panel(AppState *app);
static void cb_on_open(GtkWidget *widget, gpointer data);
static void on_save_dialog_response(GtkNativeDialog *native, gint response_id, gpointer data);
static void cb_on_save(GtkWidget *widget, gpointer data);
static void cb_on_save_as(GtkWidget *widget, gpointer data);
static void cb_on_snippet_insert(GtkWidget *widget, gpointer data);
static void logic_refresh_function_tree(AppState *app);
static void ui_show_find_dialog(AppState *app);
static void logic_find_text(AppState *app, gboolean forward);
/* UI Menu Construction */
static GtkWidget* ui_create_menu(AppState *app);
static void cb_on_find_next(GtkWidget *widget, gpointer data);
static void cb_on_find_prev(GtkWidget *widget, gpointer data);
static void cb_on_font_size_spin_changed(GtkSpinButton *spin_button, gpointer user_data); 
static void cb_on_change_font_size(GtkWidget *widget, gpointer data);
static void cb_on_select_font_type(GtkWidget *widget, gpointer data);
static void logic_init_recent_db(AppState *app);
static void cb_on_recent_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data);
void logic_add_recent_file(AppState *app, const char *file_path);
void logic_refresh_recent_files_tree(AppState *app);
void recent_files_wrapper_rebuild_ui(void);
static void cb_on_toggle_editor_focus(GtkCheckMenuItem *check_item, gpointer user_data);
static void cb_on_toggle_compile_button(GtkCheckMenuItem *check_item, gpointer user_data); 
static void logic_update_dialog_closed_message(AppState *app);
static gboolean cb_update_menu_bar_time(gpointer data); 
/* assistent rabbit track website url text only callback*/
static void on_browser_uri_notify(WebKitWebView *web_view, GParamSpec *pspec, gpointer user_data);
static char *get_short_url(const char *uri); 
static gboolean on_permission_request(WebKitWebView *web_view, WebKitPermissionRequest *request, gpointer user_data); 

/* Advanced Article Search Declarations */
typedef struct {
    GtkWidget *dialog;
    GtkWidget *entry;
    GtkWidget *results_view;
    WebKitWebView *web_view;
} ArticleSearchData;

static void ui_show_article_search_dialog(GtkWindow *parent, WebKitWebView *web_view);
static void apply_dark_mode_stylesheet(WebKitWebView *web_view);
static void on_article_load_changed(WebKitWebView *web_view, WebKitLoadEvent load_event, gpointer user_data);
static void on_search_button_clicked(GtkButton *button, gpointer user_data);
static void on_tools_article_search_activated(GtkMenuItem *menuitem, gpointer user_data);

/* Webkit log book editor version 2 Prototypes */
static const char *webkit_console_capture_script =
    "(function() {"
    "  if (window.__crystalcatConsoleInstalled) return;"
    "  window.__crystalcatConsoleInstalled = true;"
    "  "
    "  /* Expanded console methods */"
    "  var methods = ['log', 'info', 'warn', 'error', 'debug', 'assert', 'trace', 'count', 'table'];"
    "  methods.forEach(function(level) {"
    "    var original = console[level];"
    "    console[level] = function() {"
    "      try {"
    "        var timestamp = new Date().toISOString();"
    "        var args = Array.prototype.slice.call(arguments).map(function(arg) {"
    "          if (arg === null) return 'null';"
    "          if (arg === undefined) return 'undefined';"
    "          if (typeof arg === 'string') return arg;"
    "          try { return JSON.stringify(arg); }"
    "          catch (e) { return String(arg); }"
    "        });"
    "        var stack = '';"
    "        if (level === 'error' || level === 'warn' || level === 'trace' || level === 'assert') {"
    "          try { throw new Error(); } catch (e) { stack = e.stack || ''; }"
    "        }"
    "        var payload = {"
    "          level: level.toUpperCase(),"
    "          timestamp: timestamp,"
    "          args: args,"
    "          stack: stack"
    "        };"
    "        window.webkit.messageHandlers.crystalcatConsole.postMessage(JSON.stringify(payload));"
    "      } catch (e) {}"
    "      if (typeof original === 'function') {"
    "        return original.apply(console, arguments);"
    "      }"
    "    };"
    "  });"
    ""
    "  /* Catch uncaught JavaScript runtime errors */"
    "  window.onerror = function(message, source, lineno, colno, error) {"
    "    try {"
    "      var payload = {"
    "        level: 'UNCAUGHT_ERROR',"
    "        timestamp: new Date().toISOString(),"
    "        args: [message, 'at ' + source + ':' + lineno + ':' + colno],"
    "        stack: error ? error.stack : ''"
    "      };"
    "      window.webkit.messageHandlers.crystalcatConsole.postMessage(JSON.stringify(payload));"
    "    } catch (e) {}"
    "  };"
    ""
    "  /* Catch unhandled promise rejections (async/fetch errors) */"
    "  window.addEventListener('unhandledrejection', function(event) {"
    "    try {"
    "      var reason = event.reason;"
    "      var payload = {"
    "        level: 'UNHANDLED_REJECTION',"
    "        timestamp: new Date().toISOString(),"
    "        args: [reason ? reason.toString() : 'Unknown rejection'],"
    "        stack: (reason && reason.stack) ? reason.stack : ''"
    "      };"
    "      window.webkit.messageHandlers.crystalcatConsole.postMessage(JSON.stringify(payload));"
    "    } catch (e) {}"
    "  });"
    "})();";
    
static void ui_show_webkit_console_dialog( GtkMenuItem *menuitem, gpointer user_data);
static void cb_webkit_console_message(WebKitUserContentManager *manager, WebKitJavascriptResult *result, gpointer user_data);
static void cb_webkit_console_clear(GtkButton *button, gpointer user_data);
static void cb_webkit_console_remove_selected(GtkButton *button, gpointer user_data);
static void cb_webkit_console_dialog_destroy(GtkWidget *widget, gpointer user_data);
static void cb_webkit_console_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data);
static void ui_show_webkit_console_dialog(GtkMenuItem *menuitem, gpointer user_data);
/* --- Assistant / Helper Component Prototypes --- */
static gboolean on_draw_assistant_avatar(GtkWidget *widget, cairo_t *cr, gpointer data);
static void ui_build_assistant_panel(GtkWidget *parent_container, AppState *app);
static void logic_update_assistant_message(AppState *app, const char *msg);
static void cb_on_editor_buffer_changed(GtkTextBuffer *buffer, gpointer user_data);
static gboolean cb_animate_rabbit_tick(gpointer data);
static void update_rabbit_animation_state(AppState *app);
void on_menu_help_faq_activate(GtkMenuItem *menuitem, gpointer user_data);
static void cb_on_open_local_html(GtkWidget *widget, gpointer data);
/* Prototypes for the Cat-Chase logic */
static void draw_entity(cairo_t *cr, unsigned char *bits, int x, int y, double r, double g, double b);
static void update_cat_position(AppState *app);
void render_character(cairo_t *cr, unsigned char *bits, int x, int y, double r, double g, double b);
void trigger_cat_animation(AppState *app); 

/* Logic & Callbacks */
static void cb_on_toggle_menu_time(GtkCheckMenuItem *check_item, gpointer user_data);
static void cb_on_new_session_save_prompt(GtkWidget *widget, gpointer data);
static void cb_on_search(GtkWidget *widget, gpointer data);
static void cb_on_destroy(GtkWidget *widget, gpointer data);
static void cb_on_tree_row_activated(GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *col, gpointer data);
static void cb_on_generate_outline(GtkWidget *widget, gpointer data);
static void cb_on_search_dor(GtkWidget *widget, gpointer data);
static void cb_execute_ddg_search(GtkWidget *widget, gpointer data);
static void cb_play_dubstep_beyond(GtkWidget *widget, gpointer data);
static void cb_execute_engine_search(GtkWidget *widget, gpointer data); 
static void cb_on_help_page(GtkWidget *widget, gpointer data);
void cb_execute_google_search(GtkEntry *entry, AppState *app);
static void ui_show_goto_dialog(AppState *app);
static void logic_goto_line(AppState *app, const char *line_str);
static gboolean cb_rotate_status_color(gpointer data);
static void cb_on_color_picker(GtkWidget *widget, gpointer data);
static void ui_show_advanced_search_dialog(GtkMenuItem *menuitem, gpointer data);
static void cb_on_quick_save(GtkWidget *widget, gpointer data);
static gboolean cb_on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data);
static void cb_on_open_basic_audio_player(GtkWidget *widget, gpointer data);
/* Forward Declarations for Cookie Callbacks */
static void on_reject_all_cookies_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_menu_alter_cookie_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_alter_cookie_activated(GtkWidget *widget, gpointer user_data);

/* Directory Operations Callbacks */
static void cb_on_create_directory(GtkWidget *widget, gpointer data);
/* HTML TOOLS */
static void cb_on_wrap_tag_dialog(GtkWidget *widget, gpointer data);
static void cb_on_html_escape(GtkWidget *widget, gpointer data);
static void cb_on_insert_link_dialog(GtkWidget *widget, gpointer data);
static void cb_on_insert_meta_boilerplate(GtkWidget *widget, gpointer data);
static void cb_on_html_unescape(GtkWidget *widget, gpointer data);
static void cb_on_insert_image_dialog(GtkWidget *widget, gpointer data);
static void cb_on_insert_lorem_ipsum(GtkWidget *widget, gpointer data);
static void cb_on_insert_table_dialog(GtkWidget *widget, gpointer data);
static void cb_on_insert_full_html_page(GtkWidget *widget, gpointer data);
static void cb_on_wrap_html_comment(GtkWidget *widget, gpointer data);
static void cb_on_wrap_unordered_list(GtkWidget *widget, gpointer data);
static void on_calculator_menu_activate(GtkMenuItem *menuitem, gpointer user_data);
/* Terminal Logic Prototypes */
static void logic_spawn_terminal(AppState *app);
static gboolean cb_console_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data);
static gboolean cb_terminal_out(GIOChannel *source, GIOCondition condition, gpointer data);
/* Status bar alert system */
static void ui_set_status_alert(AppState *app, const char *msg, gboolean is_error);
static gboolean cb_clear_status_alert(gpointer data) ; 
/* For gcc systems compiler */
static void logic_run_gcc_advanced(AppState *app, const char *src, const char *out, const char *flags);
static void cb_on_compile(GtkWidget *widget, gpointer data);
/* File I/O & Menu Callbacks */
static void cb_on_open(GtkWidget *widget, gpointer data);
static void cb_on_save(GtkWidget *widget, gpointer data);
static void cb_on_save_as(GtkWidget *widget, gpointer data);
static void cb_on_snippet_insert(GtkWidget *widget, gpointer data);

/* AD blacking filter callbacks */ 
static char* logic_get_filters_as_json(sqlite3 *db);
static void apply_ad_filters(WebKitWebView *web_view, sqlite3 *db);
static void on_toggle_js(GtkCheckMenuItem *check_menu_item, gpointer data); 
static void on_cookies_fetched(WebKitCookieManager *manager, GAsyncResult *res, gpointer user_data); 
static void ui_show_cookie_dialog(AppState *app);
static void on_toggle_do_not_track(GtkCheckMenuItem *check_menu_item, gpointer data);
static void apply_privacy_profile(WebKitSettings *settings, gboolean hardened);
static void on_toggle_hardened_mode(GtkCheckMenuItem *item, gpointer data);
static void cb_execute_flag_direct(GtkWidget *widget, gpointer data);
void on_add_filter_menu_activated(GtkMenuItem *menuitem, gpointer data); 
GtkWidget* ui_create_cookie_spoof_menu_item(AppState *app);
/* Forward declaration for CSS blocking helper */
void apply_css_element_blocking(WebKitWebView *web_view, sqlite3 *db);
/* Forward declarations for content filtering functions */
static void on_filter_saved_2(WebKitUserContentFilterStore *store, GAsyncResult *res, gpointer user_data);
static char* logic_get_filters_as_json_2(sqlite3 *db);
void apply_ad_filters_2(WebKitWebView *web_view, sqlite3 *db);
void apply_css_element_blocking_2(WebKitWebView *web_view, sqlite3 *db);
gboolean deduplicate_ad_filters(sqlite3 *db); 
void inject_script_defusal(WebKitUserContentManager *manager);
/* Script Defusal & Anti-Exploit Security Prototypes */
void inject_www_script_defusal(WebKitUserContentManager *manager);
static void on_toggle_script_defusal(GtkCheckMenuItem *check_item, gpointer user_data);

/* Content Filter JSON & Isolation Logic */
static char* logic_get_article_only_filter_json(void);
/* Article Reader Mode Toggle Handler */
void toggle_article_image_reader_mode(AppState *app, gboolean enable);
/* UI & Menu Callbacks */
static void article_isolation_set_normal_defaults(void);
static void article_isolation_set_preset_controls(GtkComboBox *combo);
static void on_reader_preset_changed(GtkComboBox *combo, gpointer user_data);
static void  on_toggle_article_mode(GtkWidget *widget, gpointer data);
GtkWidget*   ui_create_article_isolation_menu_item(AppState *app);
static void ui_show_article_isolation_dialog(GtkMenuItem *menuitem, gpointer user_data);
/* Article & Image Isolation Settings */
static void article_isolation_apply_settings(AppState *app);
static char* logic_get_article_only_filter_json(void);

/* Tools Menu Callbacks for View Toggles */
static void cb_on_toggle_sidebar_panel(GtkCheckMenuItem *check_item, gpointer user_data);
static void cb_on_toggle_console_panel(GtkCheckMenuItem *check_item, gpointer user_data);
/* --- Recent Files Wrapper Functions --- */
void recent_files_wrapper_add(const char *filepath);
void recent_files_wrapper_rebuild_ui(void);
void recent_files_wrapper_init(GtkWidget *menu_container);
void recent_files_wrapper_cleanup(void);
void open_file_from_tree(const char *path);
static void cb_on_toggle_audio_mode(GtkCheckMenuItem *item, gpointer user_data);
/* --- Browser & Ad-Blocking Logic --- */
void block_new_domain(AppState *app, const char *pattern, const char *action);
/* --- Editor Logic --- */
static void cb_on_toggle_comment(GtkWidget *widget, gpointer data);
/* -- Browser clean up --*/
static void cb_on_clear_browsing_data(GtkWidget *widget, gpointer data);
/* -- Browser ID spoofing -- */ 
static void cb_on_ua_selected(GtkRadioMenuItem *item, gpointer user_data); 

/* Forward declarations for geolocation spoofing */
static void apply_random_geolocation_script(WebKitWebView *web_view);
static void on_randomize_location_activate(GtkMenuItem *menuitem, gpointer user_data);
static void on_load_changed_refresh_geo(WebKitWebView *web_view, WebKitLoadEvent load_event, gpointer user_data);

/* CORE RING BUFFER / PUSHING LOGIC */
void recent_files_wrapper_add(const char *filepath) {
    if (!filepath || strlen(filepath) == 0) return;
    int existing_idx = -1;
    for (int i = 0; i < g_recent_wrapper.count; i++) {
        if (g_recent_wrapper.items[i].filepath && 
            strcmp(g_recent_wrapper.items[i].filepath, filepath) == 0) {
            existing_idx = i;
            break;
        }
    }
    if (existing_idx != -1) {
        free(g_recent_wrapper.items[existing_idx].filepath);
        free(g_recent_wrapper.items[existing_idx].display_name);
        for (int i = existing_idx; i < g_recent_wrapper.count - 1; i++) {
            g_recent_wrapper.items[i] = g_recent_wrapper.items[i + 1];
        }
        g_recent_wrapper.count--;
    } 
    else if (g_recent_wrapper.count >= MAX_RECENT_FILES) {
        int last = g_recent_wrapper.count - 1;
        free(g_recent_wrapper.items[last].filepath);
        free(g_recent_wrapper.items[last].display_name);
        g_recent_wrapper.count--;
    }
    for (int i = g_recent_wrapper.count; i > 0; i--) {
        g_recent_wrapper.items[i] = g_recent_wrapper.items[i - 1];
    }
    g_recent_wrapper.items[0].filepath = strdup(filepath);    
    const char *basename = strrchr(filepath, '/');
    g_recent_wrapper.items[0].display_name = strdup(basename ? basename + 1 : filepath);
    g_recent_wrapper.items[0].last_accessed = time(NULL);
    g_recent_wrapper.count++;
    recent_files_wrapper_rebuild_ui();
}

static void on_recent_item_activated(GtkMenuItem *menuitem, gpointer user_data) {
    const char *filepath = (const char *)user_data;    
    if (!filepath) return;
    recent_files_wrapper_add(filepath);
}

void recent_files_wrapper_rebuild_ui(void) {
    if (!g_recent_wrapper.parent_menu_or_container) return;
    GList *children = gtk_container_get_children(GTK_CONTAINER(g_recent_wrapper.parent_menu_or_container));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);
    if (g_recent_wrapper.count == 0) {
        GtkWidget *empty_item = gtk_menu_item_new_with_label("No Recent Files");
        gtk_widget_set_sensitive(empty_item, FALSE);
        gtk_menu_shell_append(GTK_MENU_SHELL(g_recent_wrapper.parent_menu_or_container), empty_item);
        gtk_widget_show(empty_item);
        return;
    }
    for (int i = 0; i < g_recent_wrapper.count; i++) {
        GtkWidget *item = gtk_menu_item_new_with_label(g_recent_wrapper.items[i].display_name);
        g_signal_connect(G_OBJECT(item), "activate", 
                         G_CALLBACK(on_recent_item_activated), 
                         g_recent_wrapper.items[i].filepath);
        
        gtk_menu_shell_append(GTK_MENU_SHELL(g_recent_wrapper.parent_menu_or_container), item);
        gtk_widget_show(item);
    }
}

void recent_files_wrapper_init(GtkWidget *menu_container) {
    g_recent_wrapper.parent_menu_or_container = menu_container;
    g_recent_wrapper.count = 0;
    recent_files_wrapper_rebuild_ui();
}

void recent_files_wrapper_cleanup(void) {
    for (int i = 0; i < g_recent_wrapper.count; i++) {
        free(g_recent_wrapper.items[i].filepath);
        free(g_recent_wrapper.items[i].display_name);
    }
    g_recent_wrapper.count = 0;
}

void open_file_from_tree(const char *path) {    recent_files_wrapper_add(path); }

/* toggle java script on/off */ 
static void on_toggle_js(GtkCheckMenuItem *check_menu_item, gpointer data) {
    AppState *app = (AppState *)data;
    gboolean enable_js = gtk_check_menu_item_get_active(check_menu_item);
    WebKitSettings *settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(app->browser_view));
    webkit_settings_set_enable_javascript(settings, enable_js);
    webkit_settings_set_enable_javascript_markup(settings, enable_js);
   webkit_web_view_reload(WEBKIT_WEB_VIEW(app->browser_view));
    /* Less is more sometimes optional Quick at a glance message,   
     * ui_set_status_alert(app, "Event: Java toggled on/off", FALSE); 
     */ 
    
}

/* Internet browser filters addblckers  */
static void on_filter_saved(WebKitUserContentFilterStore *store, GAsyncResult *res, gpointer user_data) {
    GError *error = NULL;
    WebKitUserContentFilter *filter = webkit_user_content_filter_store_save_finish(store, res, &error);
    if (filter) {
        WebKitUserContentManager *manager = WEBKIT_USER_CONTENT_MANAGER(user_data);
        webkit_user_content_manager_add_filter(manager, filter);
        webkit_user_content_filter_unref(filter);
    }
}

void apply_ad_filters(WebKitWebView *web_view, sqlite3 *db) {
	/* Guard against NULL pointers or uninitialized GTK/WebKit widgets */
    if (!web_view || !WEBKIT_IS_WEB_VIEW(web_view)) { return; }
    if (!db) { return; }
    WebKitUserContentManager *manager = webkit_web_view_get_user_content_manager(web_view);
    inject_script_defusal(manager); 
    if (!manager) { return; } 
    /* Clear existing user style sheets / script filters before applying new ones */
    webkit_user_content_manager_remove_all_style_sheets(manager);
  
  /* Query active rules from SQLite and inject scripts/styles */
    const char *sql = "SELECT url_pattern, action_type FROM ad_filters WHERE enabled = 1;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char *pattern = sqlite3_column_text(stmt, 0);
            if (pattern) {
				  
    const char *cache_path = g_get_user_cache_dir();
    WebKitUserContentFilterStore *store = webkit_user_content_filter_store_new(cache_path);
    
    /* Get JSON string from SQLite */
    char *json_data = logic_get_filters_as_json(db);
    
    if (json_data) {
        /* Convert string to GBytes for WebKit */
        GBytes *bytes = g_bytes_new_take(json_data, strlen(json_data));
        
        /* Save directly from the buffer (no external .json file needed) */
        webkit_user_content_filter_store_save(store, "db-filters", bytes, NULL, 
                                              (GAsyncReadyCallback)on_filter_saved, manager);
        g_bytes_unref(bytes);
            }
          }
        }
        sqlite3_finalize(stmt);
    }
}


static char* logic_get_filters_as_json(sqlite3 *db) {
    sqlite3_stmt *stmt;
    /* This SQL query builds the JSON structure directly in the database engine */
    const char *sql = "SELECT json_group_array(json_object("
                      "'trigger', json_object('url-filter', url_pattern, 'if-domain', domain_target), "
                      "'action', json_object('type', action_type))) "
                      "FROM ad_filters WHERE enabled = 1;";
                      
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return NULL;
    }

    char *json_output = NULL;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *result = (const char*)sqlite3_column_text(stmt, 0);
        if (result) json_output = g_strdup(result);
    }

    sqlite3_finalize(stmt);
    return json_output;
}

void block_new_domain(AppState *app, const char *pattern, const char *action) {
    char *query = sqlite3_mprintf("INSERT OR IGNORE INTO ad_filters (url_pattern, action_type) VALUES (%Q, %Q);", pattern, action);
    sqlite3_exec(app->db, query, NULL, NULL, NULL);
    sqlite3_free(query);
    apply_ad_filters(WEBKIT_WEB_VIEW(app->browser_view), app->db);
}

/* Callback function to handle the "Add New Pattern" menu activation.
 * This creates a modal dialog, captures the pattern, and inserts it into the database.
 */
void on_add_filter_menu_activated(GtkMenuItem *menuitem, gpointer data) {
    AppState *app = (AppState *)data;
    
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Manage Filters",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Add", GTK_RESPONSE_ACCEPT,
        "_Close", GTK_RESPONSE_REJECT,
        NULL);
        
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content_area), 10);
    gtk_box_set_spacing(GTK_BOX(content_area), 10);
    
    GtkWidget *label = gtk_label_new("Current Filters:");
    gtk_box_pack_start(GTK_BOX(content_area), label, FALSE, FALSE, 0);

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scrolled_window, 650, 250); 
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), 
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(content_area), scrolled_window, TRUE, TRUE, 0);

    GtkListStore *store = gtk_list_store_new(5, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);
    GtkWidget *tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    
    /* Add ID Column */
    GtkCellRenderer *renderer_id = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view), -1, "ID", renderer_id, "text", 0, NULL);

    /* Add URL Pattern Column */
    GtkCellRenderer *renderer_pattern = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view), -1, "URL Pattern", renderer_pattern, "text", 1, NULL);

    /* Add Action Type Column */
    GtkCellRenderer *renderer_action = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view), -1, "Action", renderer_action, "text", 2, NULL);

    /* Add Selector Column */
    GtkCellRenderer *renderer_selector = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view), -1, "CSS Selector", renderer_selector, "text", 3, NULL);

    /* Add Enabled Column (as a Toggle Checkbox) */
    GtkCellRenderer *renderer_toggle = gtk_cell_renderer_toggle_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view), -1, "Enabled", renderer_toggle, "active", 4, NULL);

    gtk_container_add(GTK_CONTAINER(scrolled_window), tree_view);

    sqlite3_stmt *stmt;
    /* Select all required columns from table */
    const char *sql = "SELECT id, url_pattern, action_type, selector, enabled FROM ad_filters;";
    
    if (app->db == NULL) {
        g_print("Error: Database connection is NULL!\n");
    }

    int rows = 0; 
    int rc = sqlite3_prepare_v2(app->db, sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            const unsigned char *pattern = sqlite3_column_text(stmt, 1);
            const unsigned char *action = sqlite3_column_text(stmt, 2);
            const unsigned char *selector = sqlite3_column_text(stmt, 3);
            int enabled = sqlite3_column_int(stmt, 4);

            GtkTreeIter iter;
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter,
                               0, id,
                               1, pattern ? (char*)pattern : "",
                               2, action ? (char*)action : "block",
                               3, selector ? (char*)selector : "",
                               4, enabled ? TRUE : FALSE,
                               -1);
            rows++;
        }
        g_print("Successfully populated %d rows from database.\n", rows);
    } else {
        g_print("Failed to prepare statement: %s\n", sqlite3_errmsg(app->db));
    }
    sqlite3_finalize(stmt);
    g_object_unref(store); 

    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter new pattern...");
    gtk_box_pack_start(GTK_BOX(content_area), entry, FALSE, FALSE, 0);

    GtkWidget *label_2 = gtk_label_new(NULL);
    char *markup = g_strdup_printf("<b>Total filter patterns: %d</b>", rows);
    gtk_label_set_markup(GTK_LABEL(label_2), markup);
    g_free(markup);
    gtk_label_set_line_wrap(GTK_LABEL(label_2), TRUE);
    gtk_box_pack_start(GTK_BOX(content_area), label_2, FALSE, FALSE, 0);

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, "label { color: darkgreen; }", -1, NULL);
    GtkStyleContext *context = gtk_widget_get_style_context(label_2);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(provider);    

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        const char *pattern = gtk_entry_get_text(GTK_ENTRY(entry));
        if (pattern && strlen(pattern) > 0) {
            block_new_domain(app, pattern, "block");
            g_print("Added new filter pattern: %s\n", pattern); 
        }
    }
    logic_update_dialog_closed_message(app); 
    gtk_widget_destroy(dialog);
}

/* Draw Cat assistant avatar */
void trigger_cat_animation(AppState *app) {
    if (!app) return;

    /* Reset cat animation state */
    app->cat_active = 1;
    app->cat_frame = 0;
    app->cat_loop_count = 0;
    app->cat_speed_divider = 0;
    app->cat_animation_timer = 0;

    /* Queue a redraw on the avatar drawing area */
    if (app->avatar_canvas && GTK_IS_WIDGET(app->avatar_canvas)) {
        gtk_widget_queue_draw(app->avatar_canvas);
    }
}

/* Draw assistant avatar */
static gboolean on_draw_assistant_avatar(GtkWidget *widget, cairo_t *cr, gpointer data) {
    AppState *app = (AppState *)data;

    int frame = app->current_frame % TOTAL_FRAMES;
    unsigned char *bits = animation_sequence[frame];

    /* Expand 1-bit XBM data to 8-bit alpha (Cairo A8 format) */
    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_A8, ASSISTANT_WIDTH);
    unsigned char *alpha_data = g_malloc0(stride * ASSISTANT_HEIGHT);

    for (int y = 0; y < ASSISTANT_HEIGHT; y++) {
        for (int x = 0; x < ASSISTANT_WIDTH; x++) {
            int byte_index = y * 4 + x / 8;   /* 4 bytes per row for 32-wide XBM */
            int bit_index  = x % 8;
            /* XBM is LSB-first */
            if (bits[byte_index] & (1 << bit_index)) {
                alpha_data[y * stride + x] = 0xFF;
            }
        }
    }

    cairo_surface_t *surface = cairo_image_surface_create_for_data(
        alpha_data, CAIRO_FORMAT_A8,
        ASSISTANT_WIDTH, ASSISTANT_HEIGHT, stride
    ); 
    
    /* Set color based on mood */
    if (current_mood == MOOD_COMPILING)
        cairo_set_source_rgb(cr, 0.0, 0.75, 1.0);
    else if (current_mood == MOOD_ERROR)
        cairo_set_source_rgb(cr, 1.0, 0.2, 0.2);
    else if (current_mood == MOOD_TYPING) {
		/* Generate random values between 0.0 and 1.0 */
    double r = (double)rand() / (double)RAND_MAX;
    double g = (double)rand() / (double)RAND_MAX;
    double b = (double)rand() / (double)RAND_MAX;
    cairo_set_source_rgb(cr, r, g, b);
		}
                    
    else
    cairo_set_source_rgb(cr, 0.22, 1.0, 0.08);

    cairo_mask_surface(cr, surface, 5, 5);
    cairo_surface_destroy(surface);
    g_free(alpha_data);
    update_rabbit_animation_state(app); 
        
    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);

    
    app->cat_x = (int)(width * 0.9) - ASSISTANT_WIDTH; 
    app->cat_y = height / 2;                           

/* Notes: 
 * Render Rabbit 
 *   draw_entity(cr, animation_sequence[app->current_frame % TOTAL_FRAMES], 
 *                app->rabbit_x, app->rabbit_y, 0.2, 0.8, 0.2); 
 * Render Cat    
 *  draw_entity(cr, cat_animation_sequence[app->cat_frame % 3], 
 *                app->cat_x, app->cat_y, 0.8, 0.2, 0.2); 
 * */
static int counter = 1; 
           counter++; 

if (app->cat_active) {
    cairo_set_source_rgb(cr, 0.0666, 0.0666, 0.0666); 
    cairo_paint(cr);

    app->cat_animation_timer++;

    const int SLOWDOWN_FACTOR = 10;
    app->cat_speed_divider++;
    
    if (app->cat_speed_divider >= SLOWDOWN_FACTOR) {
        app->cat_speed_divider = 0;
        app->cat_frame = (app->cat_frame + 1) % 3; 
    }

    draw_entity(cr, cat_animation_sequence[app->cat_frame], 
                app->cat_x, app->cat_y, 0.8, 0.2, 1.2);

      
   
    if (app->cat_animation_timer >= 1000) {
        app->cat_active = 0;
        app->cat_animation_timer = 0; 
    }
}
  
if (counter >= 300) {
    if (!app->cat_active) {
        if ((rand() % 100) < 7) {
            app->cat_active = 1;
            app->cat_loop_count = 0;
            app->cat_frame= 0;
        }
    }

    cairo_set_source_rgb(cr, 0.17, 0.17, 0.17); 
    cairo_paint(cr);

    if (app->cat_active) {
		static int cat_animation_speed = 0;
        cat_animation_speed++;
        if (cat_animation_speed >= 8) { 
			app->cat_frame++; 
            cat_animation_speed = 0;
        }
       /* draw_entity(cr, cat_animation_sequence[app->cat_frame % 3], 
                    app->cat_x, app->cat_y, 0.8, 0.2, 0.2); // Using appropriate color
         */
                   draw_entity(cr, animation_sequence[app->current_frame % TOTAL_FRAMES], 
                    app->rabbit_x, app->rabbit_y, 0.2, 0.8, 0.2);    
        // Update Cat state
        if (app->cat_frame % 3 == 2) { 
            app->cat_loop_count++;
        }
        if (app->cat_frame == 3) { app->cat_frame= 0; } 
        
        if (app->cat_loop_count >= 5) {
            app->cat_active = 0;
            app->cat_loop_count = 0;
        }
    } else {
         draw_entity(cr, animation_sequence[app->current_frame % TOTAL_FRAMES], 
                    app->rabbit_x, app->rabbit_y, 0.2, 0.8, 1.0); 
         counter = 0;
	 } 
 }
    return TRUE;
}

/* Animate XBMP of the rabbit */
static gboolean cb_animate_rabbit_tick(gpointer data) {
	AppState *app = (AppState *)data;
    if (!app) return FALSE; 
    if (!app->avatar_canvas || !GTK_IS_WIDGET(app->avatar_canvas)) {
         app->current_frame = (app->current_frame + 1) % TOTAL_FRAMES;
         return TRUE; 
   } else { app->current_frame = (app->current_frame + 1) % TOTAL_FRAMES; }
      
   GtkWidget *avatar_canvas = app->avatar_canvas;
    static int chatter_countdown = 100; // Fires roughly every 20-30 seconds

    if (current_mood == MOOD_IDLE) {
        chatter_countdown--;
        if (chatter_countdown <= 0) {
            chatter_countdown = rand() % 150 + 100; // Reset timer
            
            const char *idle_thoughts[] = {
                "💭 Remember to compile early and compile often!",
                "🥕 Sure is quiet... time to smash out a new feature?",
                "✨ You are doing great. Don't forget to hydrate!",
                "🤖 Systems nominal. Workspace operating at peak efficiency.",
                "✨The cat is going to eat the rabbit its 3AM go to bed!✨"
                
            };
            logic_update_assistant_message(app, idle_thoughts[rand() % 5]);
        }
    }

    /* Call the new stupid subfunction*/ 
    update_rabbit_animation_state(app);
    
static int organic_idle_timer = 0;
app->current_frame = (app->current_frame + 1) % TOTAL_FRAMES;

    switch (current_mood) {
        case MOOD_TYPING:
            app->current_frame = (app->current_frame == 0) ? 1 : 0;
            if (app->current_frame == 2 || app->current_frame == 2) {
                app->current_frame = 1; // Ear Twitch / Alert          
            } else {
                app->current_frame = (app->current_frame == 0) ? 1 : 5;
                app->current_frame = rand() % TOTAL_FRAMES;
            }
            
            typing_cooldown_counter--;
            if (typing_cooldown_counter <= 0) {
                current_mood = MOOD_IDLE;
            }
            if (app->current_frame > 1) app->current_frame = 0;
            else 
            app->current_frame = 6; 
            break;

        case MOOD_COMPILING:
            app->current_frame = (app->current_frame + 1) % TOTAL_FRAMES;
            break;

        case MOOD_ERROR:
            app->current_frame = 3;
            app->current_frame = (rand() % 2 == 0) ? 0 : (TOTAL_FRAMES - 1);  
         
         
            break;

        case MOOD_IDLE:
        app->current_frame = (app->current_frame + 1) % TOTAL_FRAMES;
        break; 
        
        default:
            if (app->current_frame == 0) {
                organic_idle_timer++;
                if (organic_idle_timer >= (rand() % 15 + 15)) {
                    organic_idle_timer = 0;
                     app->current_frame = (rand() % 2 == 0) ? 1 : 2; // Split into twitch or half-blink
                }
            } else {
                if (app->current_frame == 1 || app->current_frame == 2)  app->current_frame = (rand() % 2 == 0) ? 1 : (TOTAL_FRAMES - 1);
            }
            app->current_frame = (app->current_frame + 1) % TOTAL_FRAMES;
            break;
    }
    /* Force GTK to invoke the on_draw_assistant_avatar method */
    update_rabbit_animation_state(app);
    update_cat_position(app);
    gtk_widget_queue_draw(avatar_canvas);
    return TRUE;
}

/* Font dialog and helper functions */

/* Helper callback to update the preview label inside the dialog */
static void cb_on_font_size_spin_changed(GtkSpinButton *spin_button, gpointer user_data) {
    GtkLabel *preview_label = GTK_LABEL(user_data);
    int font_size = gtk_spin_button_get_value_as_int(spin_button);

    /* Construct Pango Markup using point size scaled by 1024 */
    char *markup = g_strdup_printf(
        "<span size='%d'>The quick <b>furry</b> <i>freakish</i> <b>fox</b> jumps over the lazy <u>dog</u> mascots.</span>",
        font_size * 1024
    );

    gtk_label_set_markup(preview_label, markup);
    g_free(markup);
}

/* Change font size of main text editor */
static void cb_on_change_font_size(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;

    if (!app->editor_view) {
        ui_set_status_alert(app, "No active editor view available.", TRUE);
        return;
    }

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Change Font Size",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Apply", GTK_RESPONSE_ACCEPT,
        "_Cancel", GTK_RESPONSE_REJECT,
        NULL);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content_area), 15);
    gtk_box_set_spacing(GTK_BOX(content_area), 10);

    /* Spin button container */
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *label = gtk_label_new("Font Size (pt):");
    
    GtkWidget *spin_button = gtk_spin_button_new_with_range(6.0, 72.0, 1.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button), 12.0);

    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), spin_button, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(content_area), hbox);

    /* --- Preview Frame & Label Setup --- */
    GtkWidget *frame = gtk_frame_new(" Preview ");
    GtkWidget *preview_label = gtk_label_new(NULL);
    gtk_label_set_line_wrap(GTK_LABEL(preview_label), TRUE);
    gtk_widget_set_size_request(preview_label, 320, 80);

    gtk_container_add(GTK_CONTAINER(frame), preview_label);
    gtk_container_add(GTK_CONTAINER(content_area), frame);

    /* Connect signal to update preview on spin adjustment */
    g_signal_connect(spin_button, "value-changed", 
                     G_CALLBACK(cb_on_font_size_spin_changed), preview_label);

    /* Initialize preview label with default size */
    cb_on_font_size_spin_changed(GTK_SPIN_BUTTON(spin_button), preview_label);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        int font_size = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_button));

        /* Apply dynamic CSS provider to editor_view */
        GtkCssProvider *provider = gtk_css_provider_new();
        char css_data[128];
        snprintf(css_data, sizeof(css_data), "textview { font-size: %dpt; }", font_size);
        
        gtk_css_provider_load_from_data(provider, css_data, -1, NULL);

        GtkStyleContext *context = gtk_widget_get_style_context(app->editor_view);
        gtk_style_context_add_provider(context, 
                                        GTK_STYLE_PROVIDER(provider), 
                                        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        
        g_object_unref(provider);

        char *msg = g_strdup_printf("Editor font size updated to %dpt", font_size);
        ui_set_status_alert(app, msg, FALSE);
        g_free(msg);
    }
    logic_update_dialog_closed_message(app);
    gtk_widget_destroy(dialog);
}

/* Font type - Callback to present the native GTK Font Chooser Dialog */
static void cb_on_select_font_type(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;

    if (!app->editor_view) {
        ui_set_status_alert(app, "No active editor view available.", TRUE);
        return;
    }

    /* Create the native GTK Font Chooser Dialog */
    GtkWidget *dialog = gtk_font_chooser_dialog_new("Select Editor Font", GTK_WINDOW(app->window));
    
    /* Set current font if already configured */
    PangoContext *pango_ctx = gtk_widget_get_pango_context(app->editor_view);
    PangoFontDescription *current_desc = pango_context_get_font_description(pango_ctx);
    if (current_desc) {
        char *font_name = pango_font_description_to_string(current_desc);
        if (font_name) {
            /* Ensure the font string passed to GTK dialog is clean UTF-8 */
            gchar *safe_font_name = g_utf8_make_valid(font_name, -1);
            gtk_font_chooser_set_font(GTK_FONT_CHOOSER(dialog), safe_font_name);
            g_free(safe_font_name);
            g_free(font_name);
        }
    }

    /* Process user response */
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        PangoFontDescription *font_desc = gtk_font_chooser_get_font_desc(GTK_FONT_CHOOSER(dialog));
        if (font_desc) {
            const char *raw_family = pango_font_description_get_family(font_desc);
            
            /* Guarantee non-NULL, valid UTF-8 family name */
            gchar *safe_family = NULL;
            if (raw_family) {
                safe_family = g_utf8_make_valid(raw_family, -1);
            } else {
                safe_family = g_strdup("Monospace");
            }

            gint size_pt = pango_font_description_get_size(font_desc) / PANGO_SCALE;
            if (size_pt <= 0) size_pt = 12;

            /* Construct GTK CSS descriptor */
            char css_data[256];
            snprintf(css_data, sizeof(css_data), 
                     "textview { font-family: '%s'; font-size: %dpt; }", 
                     safe_family, 
                     size_pt);

            GtkCssProvider *provider = gtk_css_provider_new();
            gtk_css_provider_load_from_data(provider, css_data, -1, NULL);

            GtkStyleContext *context = gtk_widget_get_style_context(app->editor_view);
            gtk_style_context_add_provider(context, 
                                           GTK_STYLE_PROVIDER(provider), 
                                           GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

            g_object_unref(provider);
            pango_font_description_free(font_desc);

            /* Construct status message using validated UTF-8 string */
            char *status_msg = g_strdup_printf("Font updated: %s %dpt", safe_family, size_pt);
            ui_set_status_alert(app, status_msg, FALSE);
            
            g_free(status_msg);
            g_free(safe_family);
        }
    }
    logic_update_dialog_closed_message(app);
    gtk_widget_destroy(dialog);
}

/* =========================================================================
 * EDIT MENU HELPER FUNCTIONS & CALLBACKS
 * ========================================================================= */

/* --- TOGGLE COMMENT (Ctrl+/) --- */
static void cb_on_toggle_comment(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    if (!app->editor_view) return;

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->editor_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_selection_bounds(buffer, &start, &end);

    /* If no selection, target the current line */
    if (gtk_text_iter_equal(&start, &end)) {
        gtk_text_iter_set_line_offset(&start, 0);
        end = start;
        if (!gtk_text_iter_ends_line(&end)) {
            gtk_text_iter_forward_to_line_end(&end);
        }
    } else {
        /* Align iterator to the beginning of start line */
        gtk_text_iter_set_line_offset(&start, 0);
    }

    gint start_line = gtk_text_iter_get_line(&start);
    gint end_line = gtk_text_iter_get_line(&end);

    gtk_text_buffer_begin_user_action(buffer);

    for (gint line = start_line; line <= end_line; line++) {
        GtkTextIter line_iter;
        gtk_text_buffer_get_iter_at_line(buffer, &line_iter, line);
        
        GtkTextIter check_iter = line_iter;
        gunichar c1 = gtk_text_iter_get_char(&check_iter);
        gtk_text_iter_forward_char(&check_iter);
        gunichar c2 = gtk_text_iter_get_char(&check_iter);

        /* Toggle // comment */
        if (c1 == '/' && c2 == '/') {
            GtkTextIter del_end = line_iter;
            gtk_text_iter_forward_chars(&del_end, 2);
            gtk_text_buffer_delete(buffer, &line_iter, &del_end);
        } else {
            gtk_text_buffer_insert(buffer, &line_iter, "//", 2);
        }
    }

    gtk_text_buffer_end_user_action(buffer);
}

/* --- DUPLICATE LINE (Ctrl+D) --- */
static void cb_on_duplicate_line(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    if (!app->editor_view) return;

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->editor_view));
    GtkTextIter start, end;

    gtk_text_buffer_get_iter_at_mark(buffer, &start, gtk_text_buffer_get_insert(buffer));
    gtk_text_iter_set_line_offset(&start, 0);
    
    end = start;
    if (!gtk_text_iter_ends_line(&end)) {
        gtk_text_iter_forward_to_line_end(&end);
    }

    gchar *line_text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    gchar *insert_text = g_strdup_printf("\n%s", line_text);

    gtk_text_buffer_begin_user_action(buffer);
    gtk_text_buffer_insert(buffer, &end, insert_text, -1);
    gtk_text_buffer_end_user_action(buffer);

    g_free(line_text);
    g_free(insert_text);
}

/* --- MOVE LINE UP / DOWN (Alt+Up / Alt+Down) --- */
static void move_current_line(AppState *app, gboolean move_up) {
    if (!app->editor_view) return;

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->editor_view));
    GtkTextMark *insert_mark = gtk_text_buffer_get_insert(buffer);
    
    GtkTextIter cur_start;
    gtk_text_buffer_get_iter_at_mark(buffer, &cur_start, insert_mark);
    gint cur_line = gtk_text_iter_get_line(&cur_start);

    gint target_line = move_up ? cur_line - 1 : cur_line + 1;
    if (target_line < 0 || target_line >= gtk_text_buffer_get_line_count(buffer)) return;

    /* Get current line text */
    GtkTextIter line_start, line_end;
    gtk_text_buffer_get_iter_at_line(buffer, &line_start, cur_line);
    line_end = line_start;
    if (!gtk_text_iter_ends_line(&line_end)) {
        gtk_text_iter_forward_to_line_end(&line_end);
    }

    gchar *line_text = gtk_text_buffer_get_text(buffer, &line_start, &line_end, FALSE);

    gtk_text_buffer_begin_user_action(buffer);
    
    /* Remove original line */
    if (gtk_text_iter_get_line(&line_end) < gtk_text_buffer_get_line_count(buffer) - 1) {
        GtkTextIter del_end = line_end;
        gtk_text_iter_forward_char(&del_end);
        gtk_text_buffer_delete(buffer, &line_start, &del_end);
    } else {
        gtk_text_buffer_delete(buffer, &line_start, &line_end);
    }

    /* Insert into target position */
    GtkTextIter target_iter;
    gtk_text_buffer_get_iter_at_line(buffer, &target_iter, target_line);
    
    if (move_up) {
        gchar *inserted_str = g_strdup_printf("%s\n", line_text);
        gtk_text_buffer_insert(buffer, &target_iter, inserted_str, -1);
        g_free(inserted_str);
    } else {
        if (!gtk_text_iter_ends_line(&target_iter)) {
            gtk_text_iter_forward_to_line_end(&target_iter);
        }
        gchar *inserted_str = g_strdup_printf("\n%s", line_text);
        gtk_text_buffer_insert(buffer, &target_iter, inserted_str, -1);
        g_free(inserted_str);
    }

    gtk_text_buffer_end_user_action(buffer);
    g_free(line_text);
}

static void cb_on_move_line_up(GtkWidget *widget, gpointer data) {
    move_current_line((AppState *)data, TRUE);
}

static void cb_on_move_line_down(GtkWidget *widget, gpointer data) {
    move_current_line((AppState *)data, FALSE);
}

/* --- TRANSFORM CASE (UPPERCASE / lowercase) --- */
static void cb_on_transform_case(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    gboolean to_upper = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "to_upper"));

    if (!app->editor_view) return;

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->editor_view));
    GtkTextIter start, end;

    if (!gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) return;

    gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    if (!text) return;

    gchar *transformed = to_upper ? g_utf8_strup(text, -1) : g_utf8_strdown(text, -1);

    gtk_text_buffer_begin_user_action(buffer);
    gtk_text_buffer_delete(buffer, &start, &end);
    gtk_text_buffer_insert(buffer, &start, transformed, -1);
    gtk_text_buffer_end_user_action(buffer);

    g_free(text);
    g_free(transformed);
}

/* --- LINE SETTINGS (Line Numbers / Word Wrap) --- */
static void cb_on_toggle_line_numbers(GtkCheckMenuItem *item, gpointer data) {
    AppState *app = (AppState *)data;
    gboolean active = gtk_check_menu_item_get_active(item);

    /* Native GtkSourceView compatibility check */
    if (app->editor_view && G_OBJECT_TYPE(app->editor_view)) {
        g_object_set(G_OBJECT(app->editor_view), "show-line-numbers", active, NULL);
    }
}

static void cb_on_toggle_word_wrap(GtkCheckMenuItem *item, gpointer data) {
    AppState *app = (AppState *)data;
    gboolean active = gtk_check_menu_item_get_active(item);

    if (app->editor_view) {
        gtk_text_view_set_wrap_mode(
            GTK_TEXT_VIEW(app->editor_view),
            active ? GTK_WRAP_WORD_CHAR : GTK_WRAP_NONE
        );
    }
}



/* Cookie Monster dialog */
/* Cookie Management Callback */
static void on_cookies_fetched(WebKitCookieManager *manager, GAsyncResult *res, gpointer user_data) {
    GtkListStore *store = GTK_LIST_STORE(user_data);
    GError *error = NULL;
    
    /* Changed from GSList to GList to match webkit_cookie_manager_get_cookies_finish return type */
    GList *cookies = webkit_cookie_manager_get_cookies_finish(manager, res, &error);

    if (error) {
        g_printerr("Error fetching cookies: %s\n", error->message);
        g_error_free(error);
        return;
    }

    /* Iterate through the GList and populate the UI store */
    for (GList *l = cookies; l; l = l->next) {
        SoupCookie *cookie = (SoupCookie *)l->data;
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, soup_cookie_get_domain(cookie),
                           1, soup_cookie_get_name(cookie),
                           2, cookie, 
                           -1);
    }
    
    /* Use g_list_free_full for proper memory cleanup */
    g_list_free_full(cookies, (GDestroyNotify)soup_cookie_free);
}

/* Cookie Management Dialog */
static void ui_show_cookie_dialog(AppState *app) {
    //GtkWidget *dialog = gtk_dialog_new_with_buttons("Cookie Manager",
        //GTK_WINDOW(app->window),
        //GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        //"_Close", GTK_RESPONSE_CLOSE,
        //"_Clear All Cookies", GTK_RESPONSE_REJECT,
        //NULL);

GtkWidget *dialog = gtk_dialog_new_with_buttons("Cookie Manager",
    GTK_WINDOW(app->window),
    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
    "_Alter / inject Cookie", GTK_RESPONSE_ACCEPT,
    "_Clear All Cookies", GTK_RESPONSE_REJECT,
    "_Close", GTK_RESPONSE_CLOSE,
    NULL);

                 
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
   
    /* Create and add the Header Text */
    GtkWidget *header_label = gtk_label_new(NULL);
    /* Use Pango Markup to make it look nice (Bold) */ 
    gtk_label_set_markup(GTK_LABEL(header_label), 
        "<b><i><u>COOKIE MONSTER TRACKER</u></i>\n\nActive Cookies in the jar,</b>\n"
        "<small>Below are the cookies currently stored for the active session.</small>");
    
    /* Align text to the left and add padding */ 
    gtk_widget_set_halign(header_label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(header_label, 10);
    gtk_widget_set_margin_end(header_label, 10);
    gtk_widget_set_margin_top(header_label, 10);
    gtk_widget_set_margin_bottom(header_label, 5);
    
    gtk_box_pack_start(GTK_BOX(content_area), header_label, FALSE, FALSE, 0);

    /* Setup the Scrolled Window and TreeView */
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scrolled_window, 450, 300);
    
    GtkListStore *store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
    GtkWidget *tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view),
        gtk_tree_view_column_new_with_attributes("Domain", gtk_cell_renderer_text_new(), "text", 0, NULL));
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view),
        gtk_tree_view_column_new_with_attributes("Name", gtk_cell_renderer_text_new(), "text", 1, NULL));

    gtk_container_add(GTK_CONTAINER(scrolled_window), tree_view);
    gtk_box_pack_start(GTK_BOX(content_area), scrolled_window, TRUE, TRUE, 0);
    

                 
    /* Make everything visible */
    gtk_widget_show_all(content_area);

    /* Logic: Fetch Cookies */
    WebKitWebContext *context = webkit_web_view_get_context(WEBKIT_WEB_VIEW(app->browser_view));
    WebKitCookieManager *manager = webkit_web_context_get_cookie_manager(context);
    WebKitWebsiteDataManager *data_manager = webkit_web_context_get_website_data_manager(context);

    const char *current_uri = webkit_web_view_get_uri(WEBKIT_WEB_VIEW(app->browser_view));
    webkit_cookie_manager_get_cookies(manager, current_uri, NULL, (GAsyncReadyCallback)on_cookies_fetched, store);

/* Run Dialog once */
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    
    if (response == GTK_RESPONSE_ACCEPT) {
        /* Alter cookie */
        on_alter_cookie_activated(dialog, app);
    } else if (response == GTK_RESPONSE_REJECT) {
        /* Clear all cookies */
        webkit_website_data_manager_clear(data_manager, WEBKIT_WEBSITE_DATA_COOKIES, 0, NULL, NULL, NULL);
        ui_set_status_alert(app, "All session cookies cleared.", FALSE);
    }
    logic_update_dialog_closed_message(app);
    gtk_widget_destroy(dialog);
    g_object_unref(store);
}

static void on_toggle_do_not_track(GtkCheckMenuItem *check_menu_item, gpointer data) {
    AppState *app = (AppState *)data;
    gboolean enabled = gtk_check_menu_item_get_active(check_menu_item);
    
    WebKitSettings *settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(app->browser_view));
    
    /* Check if the property exists before trying to set it*/
    if (g_object_class_find_property(G_OBJECT_GET_CLASS(settings), "enable-do-not-track")) {
        g_object_set(G_OBJECT(settings), "enable-do-not-track", enabled, NULL);
            logic_update_assistant_message(app, "Enabled do not track mode.."); 
    } else {
           ui_set_status_alert(app, "Do-Not-Track is not supported by your WebKit version.", TRUE);
           logic_update_assistant_message(app, enabled ? "Do-Not-Track is not supported now enabled (Intelligent Tracking Prevention) as plan B  - ITP Tracking Protection: ENABLED" : "ITP Tracking Protection now: DISABLED"); 
           WebKitWebView *web_view = WEBKIT_WEB_VIEW(app->browser_view);
           WebKitWebContext *context = webkit_web_view_get_context(web_view);
           WebKitWebsiteDataManager *data_manager = webkit_web_context_get_website_data_manager(context);
           webkit_website_data_manager_set_itp_enabled(data_manager, enabled);
           ui_set_status_alert(app, enabled ? "ITP Tracking Protection: ENABLED" : "ITP Tracking Protection: DISABLED", FALSE);
           gboolean itp_active = webkit_website_data_manager_get_itp_enabled(data_manager);
           g_print("ITP status is now: %s\n", itp_active ? "ACTIVE" : "INACTIVE");
    }
    
    webkit_web_view_reload(WEBKIT_WEB_VIEW(app->browser_view));
}


static void apply_privacy_profile(WebKitSettings *settings, gboolean hardened) {
    g_object_set(G_OBJECT(settings),
        "enable-javascript", !hardened,            
        "enable-javascript-markup", !hardened,
        "enable-site-specific-quirks", !hardened,
        NULL);
}

static void on_toggle_hardened_mode(GtkCheckMenuItem *item, gpointer data) {
    AppState *app = (AppState *)data;
    gboolean hardened = gtk_check_menu_item_get_active(item);
    
    WebKitSettings *settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(app->browser_view));
    apply_privacy_profile(settings, hardened);
    ui_set_status_alert(app, hardened ? "Privacy Mode: HARDENED" : "Privacy Mode: STANDARD", FALSE);
    WebKitWebView *web_view = WEBKIT_WEB_VIEW(app->browser_view);
    WebKitWebContext *context = webkit_web_view_get_context(web_view);
    WebKitWebsiteDataManager *data_manager = webkit_web_context_get_website_data_manager(context);
    webkit_website_data_manager_set_itp_enabled(data_manager, hardened);
    WebKitCookieManager *cookie_mgr = webkit_web_context_get_cookie_manager(context);
    WebKitCookieAcceptPolicy policy = hardened ? WEBKIT_COOKIE_POLICY_ACCEPT_NO_THIRD_PARTY : WEBKIT_COOKIE_POLICY_ACCEPT_ALWAYS;

    webkit_cookie_manager_set_accept_policy(cookie_mgr, policy);
    ui_set_status_alert(
        app,
        hardened ? "Privacy Mode: HARDENED / Tracking Protection & Third-Party Cookie Block: ENABLED"
                : "Privacy ModeL DISABLED / Tracking Protection & Third-Party Cookie Block: DISABLED",
        FALSE
    );
    logic_update_assistant_message(app, hardened ? 
				"Privacy Mode: HARDENED / Tracking Protection & Third-Party Cookie Block: ENABLED"
                : "Privacy ModeL DISABLED / Tracking Protection & Third-Party Cookie Block: DISABLED");
                
    webkit_web_view_reload(WEBKIT_WEB_VIEW(app->browser_view));
}


/* Callback function for chromebooks hidden under-the-hood configureations 
 * Secondary callback specifically for the HUD button */
static void cb_hud_launch_browser(GtkWidget *button, gpointer data) {
    const char *uri = (const char *)g_object_get_data(G_OBJECT(button), "target_uri");
    GtkWidget *popup = (GtkWidget *)data;

    if (uri) {
        char command[512];
        /* Now we launch the browser ONLY after the button is clicked */
        snprintf(command, sizeof(command), 
                 "xdg-open 'https://www.google.com/search?q=%s' > /dev/null 2>&1 &", uri);
        system(command);
    }

    /* Close the HUD */
    gtk_widget_destroy(popup);
}

/* Logic to execute the internal system URI loads */

static void cb_execute_flag_direct(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    /* Retrieve the internal URI we stored in the widget object earlier */
    const char *uri = (const char *)g_object_get_data(G_OBJECT(widget), "system_uri");

    if (uri && app->browser_view) {
        /* Load the internal chrome:// or webkit:// URI */
        webkit_web_view_load_uri(WEBKIT_WEB_VIEW(app->browser_view), uri);
        
        /* Optional: Send a notification to your status bar */
        ui_set_status_alert(app, g_strdup_printf("Navigating to: %s", uri), FALSE);
    } 
    /* if sandbox is healthy then it will launch external browser, 
     * but if we have a rootkit active on this computer it will mirror what your -
     * attacker is seeing maybe ? This is just an idea and is very, very unstable 
     * and hackish. We are really mindfucking the sandbox here and things may go ,
     * off the rails! */ 
       if (uri) {
        /* Arm the Clipboard immediately */
        GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
        gtk_clipboard_set_text(clipboard, uri, -1);
        gtk_clipboard_store(clipboard);

        /* Create the Persistent HUD */
        GtkWidget *popup = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_decorated(GTK_WINDOW(popup), FALSE); 
        gtk_window_set_keep_above(GTK_WINDOW(popup), TRUE);  
        gtk_window_set_position(GTK_WINDOW(popup), GTK_WIN_POS_CENTER);
        gtk_container_set_border_width(GTK_CONTAINER(popup), 15);

        /* Cyber-style Styling */
        GtkCssProvider *provider = gtk_css_provider_new();
        gtk_css_provider_load_from_data(provider,
            "window { background-color: #000000; border: 3px solid #00FF00; }\n"
            "label { color: #00FF00; }\n"
            "button { background: #111111; color: #00FFFF; border: 1px solid #00FFFF; padding: 5px; font-weight: bold; }", -1, NULL);
        gtk_style_context_add_provider(gtk_widget_get_style_context(popup),
            GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_container_add(GTK_CONTAINER(popup), vbox);

        GtkWidget *label = gtk_label_new(NULL);
       char *markup = g_strdup_printf(
    "<span font='12' foreground='#FF0000'><b>[ SECURITY BRIDGE ]</b></span>\n\n"
    "<span foreground='#FFFFFF'>Google Chrome is <b>paranoid</b>. Direct launch blocked.</span>\n\n"
    "<span foreground='#00FF00'>URL copied to clipboard. Once Chrome opens:\n"
    "1. Press <b>CTRL+L</b> (Address Bar)\n"
    "2. Press <b>CTRL+V</b> (Paste <b>&amp;</b> Execute)</span>\n\n"
    "<span foreground='#FFFF00'>TARGET: <i>%s</i></span>", uri);
    
        gtk_label_set_markup(GTK_LABEL(label), markup);
        g_free(markup);
        gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 5);

        /* 3. The "Launch" Button */
        GtkWidget *launch_btn = gtk_button_new_with_label("LAUNCH BROWSER & CLOSE HUD");
        g_object_set_data(G_OBJECT(launch_btn), "target_uri", (gpointer)uri);
        g_signal_connect(launch_btn, "clicked", G_CALLBACK(cb_hud_launch_browser), popup);      
        gtk_box_pack_start(GTK_BOX(vbox), launch_btn, FALSE, FALSE, 0);
        gtk_widget_show_all(popup);
        ui_set_status_alert(app, "HUD Active. Waiting for user to confirm launch...", FALSE);
    }


/* All my failed attempts at this very simple hackish building a brige ,
 * over safer waters and sharp cromeOS tears cutting at my bloodshot eyes ! */

    g_print("Running brute-forcing every method and listing outputs... "); 
    if (uri) {
        /* Copy the internal URI to the clipboard */
        GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
        gtk_clipboard_set_text(clipboard, uri, -1);
        gtk_clipboard_store(clipboard);

        /* Update Status Bar */
        char *msg = g_strdup_printf("Link Armed: %s (CTRL+V in Chrome)", uri);
        ui_set_status_alert(app, msg, FALSE);
        g_free(msg);

        /* Use a standard HTTPS URL to force the browser to focus.
           'google.com' is a safe bet that Garcon will always allow.
           The '> /dev/null 2>&1' part keeps your terminal clean. */
        char command[512];
        snprintf(command, sizeof(command), "xdg-open https://about:blank > /dev/null 2>&1 &");
        system(command);
    }

   if (uri) {
        /*  Copy the internal URI to the system clipboard */
        GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
        gtk_clipboard_set_text(clipboard, uri, -1);
        gtk_clipboard_store(clipboard);

        /*  Update Status Bar with Neon-styled feedback */
        char *msg = g_strdup_printf("Link Armed: %s (CTRL+V in Browser)", uri);
        ui_set_status_alert(app, msg, FALSE);
        g_free(msg);

        /*  Open a 'New Tab' in the host browser to grab focus.
           Using 'https://google.com' or 'about:blank' usually works best 
           to bring the Chrome window to the foreground. */
        char command[512];
        snprintf(command, sizeof(command), "xdg-open about:blank > /dev/null 2>&1 &");
        system(command);
        
        printf("[Crystalcat] Clipboard armed. Focus shifted to host browser.\n");
    } 
    if (uri) {
        /*  Copy to Clipboard (The Guaranteed Success) */
        GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
        gtk_clipboard_set_text(clipboard, uri, -1);
        gtk_clipboard_store(clipboard);

        /*  Update the UI to tell the user it's ready to paste */
        char *msg = g_strdup_printf("Copied: %s (Paste in Chrome)", uri);
        ui_set_status_alert(app, msg, FALSE);
        g_free(msg);

        /*  The System Call (The "Best Effort" Launch) 
           We use 'xdg-open' instead of 'google-chrome' because xdg-open 
           is the standard bridge to the host OS. */
        char command[512];
        snprintf(command, sizeof(command), "xdg-open %s > /dev/null 2>&1 &", uri);
        system(command);
        
        /* Note: '> /dev/null 2>&1' hides the error messages from your console 
           so your terminal stays clean even when ChromeOS blocks the link. */
    }

    if (uri) {
        /* * Construct the command to launch the external browser.
         * 'xdg-open' will trigger the system's default browser (Chrome/Chromium)
         * and the '&' ensures it runs in the background so your MDE doesn't freeze.
         */
        char command[512];
        snprintf(command, sizeof(command), "xdg-open %s &", uri);
        
        /* Execute the system call */
        system(command);
        
        /* Alert the status bar */
        ui_set_status_alert(app, g_strdup_printf("Launched External Browser for: %s", uri), FALSE);
    }
    if (uri) {
        char command[512];
        /* * Attempt to use google-chrome or chromium specifically. 
         * Using the --new-window flag often helps bypass garcon's URL filter.
         */
        snprintf(command, sizeof(command), "google-chrome --new-window %s &", uri);
        
        if (system(command) != 0) {
            // If that fails, try chromium
            snprintf(command, sizeof(command), "chromium --new-window %s &", uri);
            system(command);
        }
        
        ui_set_status_alert(app, "Attempted external system flag launch", FALSE);
    }
    if (uri) {
        /* Copy to clipboard */ 
        GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
        gtk_clipboard_set_text(clipboard, uri, -1);

        /* Notify user via your status bar logic */
        char *msg = g_strdup_printf("URL copied: Paste %s into Chrome address bar", uri);
        ui_set_status_alert(app, msg, FALSE);
        g_free(msg);
    }
    
}

/* Advanced Search dialog */
static void ui_show_advanced_search_dialog(GtkMenuItem *menuitem, gpointer data) {
    AppState *app = (AppState *)data;

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Advanced Search",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Search", GTK_RESPONSE_ACCEPT,
        "_Cancel", GTK_RESPONSE_REJECT,
        NULL);
   
    gtk_window_set_default_size(GTK_WINDOW(dialog), 450, 350);
    GtkWidget *dork_info_label = gtk_label_new(
    "This tool will perform an advanced search via the Google Dorking method, using specific search operators to locate precise information."
    );
    gtk_label_set_line_wrap(GTK_LABEL(dork_info_label), FALSE);
    gtk_label_set_max_width_chars(GTK_LABEL(dork_info_label), 40); 
    gtk_widget_set_halign(dork_info_label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(dork_info_label, 12);
    gtk_widget_set_margin_end(dork_info_label, 12);
    gtk_widget_set_margin_top(dork_info_label, 12);
    gtk_widget_set_margin_bottom(dork_info_label, 12);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_pack_start(GTK_BOX(content_area), dork_info_label, FALSE, FALSE, 0);     
    gtk_container_set_border_width(GTK_CONTAINER(content_area), 15);
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    /* Allow the grid to use the extra vertical space and center the form */
    gtk_box_pack_start(GTK_BOX(content_area), grid, TRUE, TRUE, 0);
    gtk_widget_set_valign(grid, GTK_ALIGN_CENTER);

    /* Keywords */
    GtkWidget *key_entry = gtk_entry_new();
    gtk_widget_set_hexpand(key_entry, TRUE);
    gtk_widget_set_halign(key_entry, GTK_ALIGN_FILL);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Keywords:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), key_entry, 1, 0, 1, 1);

    /* Target Site */ 
    GtkWidget *site_entry = gtk_entry_new();
    gtk_widget_set_hexpand(site_entry, TRUE);
    gtk_widget_set_halign(site_entry, GTK_ALIGN_FILL);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Target Site:"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), site_entry, 1, 1, 1, 1);

    /* Location */
    GtkWidget *loc_entry = gtk_entry_new();
    gtk_widget_set_hexpand(loc_entry, TRUE);
    gtk_widget_set_halign(loc_entry, GTK_ALIGN_FILL);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Location:"), 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), loc_entry, 1, 2, 1, 1);

    /* Days of post 1 to 21 days */ 
    GtkWidget *days_entry = gtk_entry_new();
    gtk_widget_set_hexpand(days_entry, TRUE);
    gtk_widget_set_halign(days_entry, GTK_ALIGN_FILL);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Days Recent:"), 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), days_entry, 1, 3, 1, 1);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        const char *key = gtk_entry_get_text(GTK_ENTRY(key_entry));
        const char *site = gtk_entry_get_text(GTK_ENTRY(site_entry));
        const char *loc = gtk_entry_get_text(GTK_ENTRY(loc_entry));
        const char *days = gtk_entry_get_text(GTK_ENTRY(days_entry));
        
        /* Build the query: "keyword" site:example.com location:us */ 
        char *query = g_strdup_printf(" allintitle:%s site:%s weather:%s location:%s date:%s &as_qdr=d%s", key, site, loc,loc, days, days);
        
        if (app->search_entry) {
            gtk_entry_set_text(GTK_ENTRY(app->search_entry), query);
            /* cb_execute_ddg_search(NULL, app); for duckduckgo.com */ 
            cb_execute_google_search(NULL, app); 
        }
        g_free(query);
    }
    logic_update_dialog_closed_message(app);
    gtk_widget_destroy(dialog);
}

/* --- Main Entry Point --- */
int main(int argc, char *argv[]) {
	/* Seed pseudo-random number generator */
    srand((unsigned int)time(NULL));
   
    gtk_init(&argc, &argv);

    AppState *app = g_new0(AppState, 1);
        
    app_init_state(app);
    
    app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL); 
    gtk_window_set_title(GTK_WINDOW(app->window), "Crystalcat MDE");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 1000, 700);
    g_signal_connect(app->window, "destroy", G_CALLBACK(gtk_main_quit), NULL); 
    g_signal_connect(app->window, "key-press-event", G_CALLBACK(cb_on_key_press), app);
    g_print("Terminal output: \n");
    app_init_db(app);
    ui_apply_styles(); 
    ui_build_layout(app);
   g_timeout_add(70, cb_animate_rabbit_tick, app); /* 70ms interval */ 
 
    gtk_widget_show_all(app->window);
    gtk_main();

    app_cleanup(app);
    g_free(app); 
    return 0;
}

/* --- Initialization Modules --- */

static void app_init_state(AppState *app) {
	app->color_toggle = 0; 
    app->editor_buffer = NULL;
    app->browser_view = NULL;
    app->editor_view = NULL; 
    app->find_dialog = NULL; 
    app->find_entry = NULL; 
    app->case_check = NULL; 
    app->word_check = NULL; 
    app->search_entry = NULL;
    app->console_buffer = NULL;
    app->tree_store = NULL;
    app->db = NULL;
    app->current_filename = NULL;
    app->current_frame = 0;
    g_timeout_add(40, (GSourceFunc)cb_animate_rabbit_tick, app);
    app->cat_active = 0;
    app->cat_remaining = 0;   
    app->webkit_log_buffer = gtk_text_buffer_new(NULL);
    app->webkit_log_dialog = NULL;
    srand(time(NULL)); 
 }

/* New function to handle rabbit animation frames */
static void update_cat_position(AppState *app) {    
    /* Animate the cat hunting the rabbit */ 
    app->cat_frame = (app->cat_frame + 1) % 2; 
}

/* * Helper function: Draws an XBM bitmap at (x, y) coordinates using Cairo.
 * Place this implementation at file scope (outside any other function).
 */
static void draw_entity(cairo_t *cr, unsigned char *bits, int x, int y, double r, double g, double b) {
    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_A8, ASSISTANT_WIDTH);
    unsigned char *alpha_data = g_malloc0(stride * ASSISTANT_HEIGHT);

    for (int y_bit = 0; y_bit < ASSISTANT_HEIGHT; y_bit++) {
        for (int x_bit = 0; x_bit < ASSISTANT_WIDTH; x_bit++) {
            int byte_index = y_bit * 4 + x_bit / 8;
            if (bits[byte_index] & (1 << (x_bit % 8))) {
                alpha_data[y_bit * stride + x_bit] = 0xFF;
            }
        }
    }

    cairo_surface_t *surface = cairo_image_surface_create_for_data(
        alpha_data, CAIRO_FORMAT_A8, ASSISTANT_WIDTH, ASSISTANT_HEIGHT, stride
    );
    
    /* Apply the color parameters here */
    cairo_set_source_rgb(cr, r, g, b);
    
    cairo_mask_surface(cr, surface, x, y);
    
    cairo_surface_destroy(surface);
    g_free(alpha_data);
}

static void update_rabbit_animation_state(AppState *app) {
    if (!app) return;

    /* Static counter persists between function calls */
    static int animation_clock = 0;
    const int frame_delay = 99; /* Controls the speed (Higher = Slower) */
    
    animation_clock++;

    /* Only perform the state update when the delay is met */
    if (animation_clock >= frame_delay) {
        animation_clock = 0; 
        switch (current_mood) {
            case MOOD_TYPING:
                app->current_frame = (app->current_frame == 0) ? 1 : 0;
                break;
            case MOOD_COMPILING:
                app->current_frame = (app->current_frame + 1) % TOTAL_FRAMES;
                break;
            case MOOD_ERROR:
                app->current_frame = 3;
                break;
            case MOOD_IDLE:
            app->current_frame = (app->current_frame + 1) % TOTAL_FRAMES;
            default:
                /* Idle wiggle, wiggle - Womp-womp! */
                app->current_frame = (rand() % 10 == 0) ? 1 : 0;
                break;
        }
        if (app->avatar_canvas) {
            gtk_widget_queue_draw(app->avatar_canvas);
        }
    }
}

/* Helper functions for our Recent tree  */
/* Ensure the recent_files table exists */
/* Initialize SQLite database table for recent files */
void logic_init_recent_db(AppState *app) {
    char *err_msg = NULL;
    const char *sql = "CREATE TABLE IF NOT EXISTS recent_files ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "file_path TEXT UNIQUE, "
                      "last_opened DATETIME DEFAULT CURRENT_TIMESTAMP);";

    if (sqlite3_exec(app->db, sql, 0, 0, &err_msg) != SQLITE_OK) {
        g_printerr("SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
}

/* Add or update a recent file in the database */
void logic_add_recent_file(AppState *app, const char *file_path) {
    if (!file_path) return;

    const char *sql = "INSERT INTO recent_files (file_path, last_opened) "
                      "VALUES (?, CURRENT_TIMESTAMP) "
                      "ON CONFLICT(file_path) DO UPDATE SET last_opened = CURRENT_TIMESTAMP;";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(app->db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, file_path, -1, SQLITE_STATIC);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);

    /* Refresh UI after inserting */
    logic_refresh_recent_files_tree(app);
}


void logic_refresh_recent_files_tree(AppState *app) {
    if (!app->recent_tree_store) return;

    /* Clear current UI tree entries */
    gtk_tree_store_clear(app->recent_tree_store);

    const char *sql = "SELECT file_path FROM recent_files ORDER BY last_opened DESC LIMIT 15;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(app->db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        GtkTreeIter iter;

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *full_path = (const char *)sqlite3_column_text(stmt, 0);
            
            /* Extract base file name for display */
            char *base_name = g_path_get_basename(full_path);

            gtk_tree_store_append(app->recent_tree_store, &iter, NULL);
            gtk_tree_store_set(app->recent_tree_store, &iter,
                               0, base_name,   /* Column 0: Short display name */
                               1, full_path,   /* Column 1: Full path string */
                               -1);

            g_free(base_name);
        }
    }
    sqlite3_finalize(stmt);
}

GtkWidget *ui_create_sidebar(AppState *app) {
    app->sidebar_notebook = gtk_notebook_new();

    /* --- TAB 1: Functions Tree (Existing) --- */
    GtkWidget *functions_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(functions_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    /* Ensure tree store exists */
    if (!app->tree_store) {
        app->tree_store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    }
    
    GtkWidget *func_tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(app->tree_store));
    GtkCellRenderer *func_renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *func_col = gtk_tree_view_column_new_with_attributes("Functions", func_renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(func_tree_view), func_col);
    gtk_container_add(GTK_CONTAINER(functions_scroll), func_tree_view);

    /* Attach your existing functions GtkTreeView here */
    gtk_notebook_append_page(GTK_NOTEBOOK(app->sidebar_notebook), functions_scroll, gtk_label_new("Functions"));


    /* --- TAB 2: Recent Files Tree --- */
    GtkWidget *recent_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(recent_scroll), 
                                   GTK_POLICY_AUTOMATIC, 
                                   GTK_POLICY_AUTOMATIC);

    /* 2 Columns: 0 = Display Name, 1 = Hidden Full Path */
    app->recent_tree_store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
    app->recent_tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(app->recent_tree_store));

    /* Create visible column for Display Name */
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes("Recent Files", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(app->recent_tree_view), col);

    /* Connect signals */
    g_signal_connect(app->recent_tree_view, "row-activated", G_CALLBACK(cb_on_recent_row_activated), app);

    gtk_container_add(GTK_CONTAINER(recent_scroll), app->recent_tree_view);
    gtk_notebook_append_page(GTK_NOTEBOOK(app->sidebar_notebook), recent_scroll, gtk_label_new("Recent"));

    /* Populate tree once at setup */
    logic_refresh_recent_files_tree(app);
    
    /* Show all widgets in sidebar */
    gtk_widget_show_all(app->sidebar_notebook);

    return app->sidebar_notebook;
}

void cb_on_recent_row_activated(GtkTreeView *tree_view, 
                                GtkTreePath *path, 
                                GtkTreeViewColumn *column, 
                                gpointer user_data) {
    AppState *app = (AppState *)user_data;
    if (!app || !app->editor_buffer) return;

    GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
    GtkTreeIter iter;

    if (!gtk_tree_model_get_iter(model, &iter, path)) return;

    char *full_path = NULL;
    gtk_tree_model_get(model, &iter, 1, &full_path, -1);

    if (!full_path) return;
    if (gtk_text_buffer_get_modified(app->editor_buffer)) {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(app->window),
            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_NONE,
            "You have unsaved changes in the current file.\nDo you want to save them before opening a new file?"
        );

        gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                               "_Save", GTK_RESPONSE_YES,
                               "_Discard", GTK_RESPONSE_NO,
                               "_Cancel", GTK_RESPONSE_CANCEL,
                               NULL);

        gint response = gtk_dialog_run(GTK_DIALOG(dialog));
        logic_update_dialog_closed_message(app);
        gtk_widget_destroy(dialog);

        if (response == GTK_RESPONSE_YES) {
            cb_on_save(NULL, app);
            if (gtk_text_buffer_get_modified(app->editor_buffer)) {
                g_free(full_path);
                return;
            }
        } else if (response == GTK_RESPONSE_CANCEL || response == GTK_RESPONSE_DELETE_EVENT) {
            g_free(full_path);
            return;
        }
    }
    if (!g_file_test(full_path, G_FILE_TEST_EXISTS)) {
        ui_set_status_alert(app, "Error: Selected file no longer exists on disk.", TRUE);
        g_free(full_path);
        return;
    }

    char *contents = NULL;
    gsize length = 0;
    GError *err = NULL;

    if (g_file_get_contents(full_path, &contents, &length, &err)) {
        gtk_text_buffer_set_text(app->editor_buffer, contents, length);
        gtk_text_buffer_set_modified(app->editor_buffer, FALSE);
        if (app->current_filename) {
            g_free(app->current_filename);
        }
        app->current_filename = g_strdup(full_path);
        ui_update_statusbar(app->editor_buffer, app);
        logic_refresh_function_tree(app);
        logic_add_recent_file(app, full_path);
        ui_set_status_alert(app, g_strdup_printf("Loaded: %s", app->current_filename), FALSE);
        g_free(contents);
    } else {
        char *err_msg = g_strdup_printf("Failed to read file: %s", err ? err->message : "Unknown error");
        ui_set_status_alert(app, err_msg, TRUE);
        g_free(err_msg);
        if (err) g_error_free(err);
    }


    gtk_tree_model_get(model, &iter, 1, NULL, -1);

    if (full_path) {
        open_file_from_tree(full_path);
        g_free(full_path);
    }
}

static void app_init_db(AppState *app) {
 int rc = sqlite3_open("ide_context.db", &app->db);
    if (rc != SQLITE_OK) {
        return;
    }

    logic_init_recent_db(app);

    sqlite3_exec(app->db, 
        "CREATE TABLE IF NOT EXISTS code_index (name TEXT, line INT, type TEXT);", 
        NULL, NULL, NULL);

    /* Update table schema to include selector column */
    sqlite3_exec(app->db, 
        "CREATE TABLE IF NOT EXISTS ad_filters ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "url_pattern TEXT UNIQUE NOT NULL, "
        "action_type TEXT DEFAULT 'block', "
        "selector TEXT DEFAULT NULL, "  
        "enabled INTEGER DEFAULT 1);", 
        NULL, NULL, NULL);

    /* Seed default network blocking rules */
    sqlite3_exec(app->db, 
        "INSERT OR IGNORE INTO ad_filters (id, url_pattern, action_type) VALUES "
        "(1, '.*doubleclick.net.*', 'block'), "
        "(2, '.*google-analytics.com.*', 'block');", 
        NULL, NULL, NULL);

    const char *sql_network_blocks = 
        "INSERT OR IGNORE INTO ad_filters (url_pattern, action_type) VALUES "
        /* Google Ads */
        "('.*googleadservices\\.com.*', 'block'), "
        "('.*adservice\\.google.*', 'block'), "
        "('.*google\\.com/aclk.*', 'block'), "
        "('.*google\\.com/ads.*', 'block'), "
        "('.*g\\.doubleclick\\.net.*', 'block'), "
        "('.*securepubads\\.g\\.doubleclick\\.net.*', 'block'), "
        "('.*stats\\.g\\.doubleclick\\.net.*', 'block'), "
        "('.*ad\\.doubleclick\\.net.*', 'block'), "
        "('.*cm\\.g\\.doubleclick\\.net.*', 'block'), "
        "('.*googlesyndication\\.com.*', 'block'), "
        "('.*pagead2\\.googlesyndication\\.com.*', 'block'), "
        "('.*tpc\\.googlesyndication\\.com.*', 'block'), "
        "('.*pagead2\\.googleadservices\\.com.*', 'block'), "
        "('.*googleadmanager\\.com.*', 'block'), "
        "('.*pubads\\.g\\.doubleclick\\.net.*', 'block'), "
        "('.*adservice\\.google\\.com.*', 'block'), "
        "('.*googlemobileads\\.g\\.doubleclick\\.net.*', 'block'), "
        "('.*media\\.admob\\.com.*', 'block'), "
        "('.*admob\\.com.*', 'block'), "
        "('.*s\\.youtube\\.com.*', 'block'), "
        "('.*youtube\\.com/api/stats/ads.*', 'block'), "
        "('.*youtube\\.com/ptracking.*', 'block'), "
        "('.*ads\\.youtube\\.com.*', 'block'), "
        "('.*googletagmanager\\.com.*', 'block'), "
        "('.*googletagservices\\.com.*', 'block'), "
        "('.*analytics\\.google\\.com.*', 'block'), "
        "('.*amazon-adsystem\\.com.*', 'block'), "
        "('.*aax-us-east\\.amazon-adsystem\\.com.*', 'block'), "
        "('.*criteo\\.com.*', 'block'), "
        "('.*scorecardresearch\\.com.*', 'block'), "
        "('.*casalemedia\\.com.*', 'block'), "
        "('.*openx\\.net.*', 'block'), "
        "('.*indexww\\.com.*', 'block'), "
        "('.*ads-api\\.tiktok\\.com.*', 'block'), "
        "('.*an\\.facebook\\.com.*', 'block'), "
        "('.*pixel\\.facebook\\.com.*', 'block'), "
        "('.*connect\\.facebook\\.net.*', 'block'), "
        "('.*segment\\.io.*', 'block'), "
        "('.*sentry\\.io.*', 'block'), "   
        "('.*clarity\\.ms.*', 'block'), "        
        "('.*stats\\.wp\\.com.*', 'block'), "
        "('.*hotjar\\.com.*', 'block'), "
        "('.*aiv-cdn\\.net.*', 'block'), "
        "('.*mixpanel\\.com.*', 'block'), "
        "('.*adnxs\\.com.*', 'block'), "
        "('.*pubmatic\\.com.*', 'block'), "
        "('.*rubiconproject\\.com.*', 'block'), "
        "('.*outbrain\\.com.*', 'block'), "
        "('.*taboola\\.com.*', 'block'), "
        "('.*yieldmo\\.com.*', 'block'), "
        "('.*platform\\.twitter\\.com.*', 'block'), "
        "('.*criteo\\.net.*', 'block'), "
        "('.*smartadserver\\.com.*', 'block'), "
        "('.*media.net.*', 'block'), "
        "('.*adform\\.net.*', 'block'), "
        "('.*revcontent\\.com.*', 'block'), "
        "('.*mgid\\.com.*', 'block'), "
        "('.*zemanta\\.com.*', 'block'), "
        "('.*chartbeat\\.com.*', 'block'), "
        "('.*crazyegg\\.com.*', 'block'), "
        "('.*mouseflow\\.com.*', 'block'), "
        "('.*quantserve\\.com.*', 'block'), "
        "('.*amplitude\\.com.*', 'block'), "
        "('.*appsflyer\\.com.*', 'block'), "
        "('.*adjust\\.com.*', 'block'), "
        "('.*branch\\.io.*', 'block'), "
        "('.*telemetry\\.microsoft\\.com.*', 'block'), "
        "('.*onetrust\\.com.*', 'block'), "
        "('.*cookielaw\\.org.*', 'block'), "
        "('.*bluekai\\.com.*', 'block'), "
        "('.*krxd\\.net.*', 'block'), "          
        "('.*syndication\\.twitter\\.com.*', 'block');";

    sqlite3_exec(app->db, sql_network_blocks, NULL, NULL, NULL);

    /* Seed default CSS Element Hiding Rules */
    const char *sql_css_blocks = 
        "INSERT OR IGNORE INTO ad_filters (url_pattern, action_type, selector) VALUES "
        "('.*', 'css-display-none', '.ad-container'), "
        "('.*', 'css-display-none', '#ad-container'), "
        "('.*', 'css-display-none', '[class*=\"ad-container\"]'), "
        "('.*', 'css-display-none', '[id*=\"ad-container\"]');";

    sqlite3_exec(app->db, sql_css_blocks, NULL, NULL, NULL);

    /* Apply the CSS filters to your WebKit View if browser_view is initialized */
    if (app->browser_view) {
        apply_css_element_blocking(WEBKIT_WEB_VIEW(app->browser_view), app->db);
    }  
    if (deduplicate_ad_filters(app->db)) {
		apply_ad_filters(WEBKIT_WEB_VIEW(app->browser_view), app->db); 
		}
}

static void app_cleanup(AppState *app) {
    if (app->db) {
        sqlite3_close(app->db);
    }
    if (app->current_filename) {
		g_free(app->current_filename); 
		app->current_filename = NULL; 
	}
	if (app->webkit_log_buffer) {
    g_object_unref(app->webkit_log_buffer);
    app->webkit_log_buffer = NULL;
    }
}


/* Assistant build ui panel */
static void ui_build_assistant_panel(GtkWidget *parent_container, AppState *app) {
    app->assistant_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(app->assistant_box), 8);
    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_provider,
        "box { background-color: #111111; border: 2px solid #39ff14; border-radius: 4px; }\n"
        "label { color: #39ff14; font-family: monospace; font-size: 12pt; }", -1, NULL);
    gtk_style_context_add_provider(gtk_widget_get_style_context(app->assistant_box),
        GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css_provider);

/* Create Cat Canvas */
/* Create a horizontal box to hold both canvases */
GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
app->cat_canvas = gtk_drawing_area_new();
gtk_widget_set_size_request(app->cat_canvas, ASSISTANT_WIDTH+10, ASSISTANT_HEIGHT+10);
gtk_box_pack_start(GTK_BOX(hbox), app->cat_canvas, FALSE, TRUE, 0);

    GtkWidget *avatar_canvas = gtk_drawing_area_new();
    gtk_widget_set_size_request(avatar_canvas, ASSISTANT_WIDTH + 10, ASSISTANT_HEIGHT + 10);
    g_signal_connect(G_OBJECT(avatar_canvas), "draw", G_CALLBACK(on_draw_assistant_avatar), app);

    /* Tick faster (200ms) to ensure smooth multi-frame playback sequence */
    g_timeout_add(200, cb_animate_rabbit_tick, app);
    gtk_box_pack_start(GTK_BOX(app->assistant_box), avatar_canvas, FALSE, FALSE, 0);

    app->assistant_label = gtk_label_new("System initialized. Monitoring workspace...");
    gtk_label_set_xalign(GTK_LABEL(app->assistant_label), 0.0);
    gtk_box_pack_start(GTK_BOX(app->assistant_box), app->assistant_label, TRUE, TRUE, 5);

    gtk_box_pack_end(GTK_BOX(parent_container), app->assistant_box, FALSE, FALSE, 2);
    
    gtk_widget_set_no_show_all(app->assistant_box, FALSE);
    gtk_widget_show_all(app->assistant_box);
}

static void ui_apply_styles(void);

/* UI Find Dialog functions */ 
static void ui_show_find_dialog(AppState *app) {
	 ui_set_status_alert(app, " MDE System Searching..........", FALSE);
    /* SAFEGUARD: Check if the buffer exists before opening the dialog */
    if (!app->editor_buffer) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->window),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_WARNING,
            GTK_BUTTONS_OK,
            "No active editor buffer found. Please open a file first.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        logic_update_dialog_closed_message(app);
        gtk_widget_destroy(dialog);
        return;
    }
    if (app->find_dialog) {
        gtk_window_present(GTK_WINDOW(app->find_dialog));
        return;
    }
    
    app->find_dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->find_dialog), "Find");
    
    gtk_window_set_transient_for(GTK_WINDOW(app->find_dialog), GTK_WINDOW(app->window));
    
    gtk_container_set_border_width(GTK_CONTAINER(app->find_dialog), 10);
     /* Guard against app->window being NULL or invalid */
    if (app->window && GTK_IS_WINDOW(app->window)) {
        gtk_window_set_transient_for(GTK_WINDOW(app->find_dialog), GTK_WINDOW(app->window));
    }
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(app->find_dialog), vbox);

    /* Search Entry */
    app->find_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app->find_entry), "Search for...");
    gtk_box_pack_start(GTK_BOX(vbox), app->find_entry, FALSE, FALSE, 0);

    /* Options */
    app->case_check = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Case sensitive"));
    app->word_check = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Match whole word"));
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(app->case_check), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(app->word_check), FALSE, FALSE, 0);

    /* Buttons */
    GtkWidget *bbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
    
    GtkWidget *btn_prev = gtk_button_new_with_label("Previous");
    GtkWidget *btn_next = gtk_button_new_with_label("Next");
    GtkWidget *btn_close = gtk_button_new_with_label("Close");

    gtk_container_add(GTK_CONTAINER(bbox), btn_prev);
    gtk_container_add(GTK_CONTAINER(bbox), btn_next);
    gtk_container_add(GTK_CONTAINER(bbox), btn_close);
    gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 5);

    /* Signals */
    g_signal_connect(btn_next, "clicked", G_CALLBACK(cb_on_find_next), app);
    g_signal_connect(btn_prev, "clicked", G_CALLBACK(cb_on_find_prev), app);
    g_signal_connect_swapped(btn_close, "clicked", G_CALLBACK(gtk_widget_destroy), app->find_dialog);
    g_signal_connect(app->find_dialog, "destroy", G_CALLBACK(gtk_widget_destroyed), &app->find_dialog);

    gtk_widget_show_all(app->find_dialog);
}

static void logic_find_text(AppState *app, gboolean forward) {
	/* SAFEGUARD STAGE 2: Prevent logic execution on NULL pointers */
    if (!app->editor_buffer || !app->find_entry) return;
    
    const char *text = gtk_entry_get_text(GTK_ENTRY(app->find_entry));
    if (strlen(text) == 0) return;

    GtkTextIter start, match_start, match_end;
    /* Common crash point if editor_buffer is NULL value */
    GtkTextMark *mark = gtk_text_buffer_get_insert(app->editor_buffer);
    gtk_text_buffer_get_iter_at_mark(app->editor_buffer, &start, mark);

    GtkTextSearchFlags flags = GTK_TEXT_SEARCH_VISIBLE_ONLY;
    if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(app->case_check))) {
        flags |= GTK_TEXT_SEARCH_CASE_INSENSITIVE;
    }

    gboolean found = FALSE;
    if (forward) {
        found = gtk_text_iter_forward_search(&start, text, flags, &match_start, &match_end, NULL);
    } else {
        found = gtk_text_iter_backward_search(&start, text, flags, &match_start, &match_end, NULL);
    }

    if (found) {
        /* Check for Whole Word if requested */
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(app->word_check))) {
            if (!gtk_text_iter_starts_word(&match_start) || !gtk_text_iter_ends_word(&match_end)) {
                /* Not a whole word, recursively call to find the next match */
                gtk_text_buffer_place_cursor(app->editor_buffer, forward ? &match_end : &match_start);
                logic_find_text(app, forward);
                return;
            }
        }

        gtk_text_buffer_select_range(app->editor_buffer, &match_start, &match_end);
        gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(app->editor_view), &match_start, 0.0, FALSE, 0, 0);
    }
}

static void cb_on_find_next(GtkWidget *widget, gpointer data) { logic_find_text((AppState*)data, TRUE); }
static void cb_on_find_prev(GtkWidget *widget, gpointer data) { logic_find_text((AppState*)data, FALSE); }

static void ui_show_goto_dialog(AppState *app) {
    if (!app->editor_buffer) return; /* Safeguard for null point crash */

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Go to Line",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Go", GTK_RESPONSE_OK,
        NULL);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);
    
    GtkWidget *label = gtk_label_new("Line number:");
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE); /* Enter key triggers 'Go' */

    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(content_area), hbox);
    
    gtk_widget_show_all(dialog);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *line_text = gtk_entry_get_text(GTK_ENTRY(entry));
        logic_goto_line(app, line_text);
    }
    logic_update_dialog_closed_message(app);
    gtk_widget_destroy(dialog);
}

static void logic_goto_line(AppState *app, const char *line_str) {
    int line_num = atoi(line_str);
    if (line_num <= 0) return;

    GtkTextIter iter;
    /* GTK lines are 0-indexed, so we subtract 1 from user input */
    gtk_text_buffer_get_iter_at_line(app->editor_buffer, &iter, line_num - 1);

    /* Move the insertion point (cursor) to the start of that line */
    gtk_text_buffer_place_cursor(app->editor_buffer, &iter);

    /* Scroll the view so the line is in the center of the screen */
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(app->editor_view), &iter, 0.0, TRUE, 0.5, 0.5);
    
    /* Regain focus on the editor */
    gtk_widget_grab_focus(app->editor_view);
}

/* UI Status bar at bottom of editor classic */
static void ui_update_statusbar(GtkTextBuffer *buffer, gpointer user_data) {
    /*g_print("Debug: user_data pointer is %p\n", user_data);*/ 
    
    if (!user_data) return;
    
    AppState *app = (AppState *)user_data;

    /* Robust safety check: verify 'app' and the statusbar widget itself */
    if (!app || !app->statusbar || !GTK_IS_STATUSBAR(app->statusbar)) {
        return; 
    }

    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_mark(buffer, &iter, gtk_text_buffer_get_insert(buffer));

    int row = gtk_text_iter_get_line(&iter) + 1;
    int col = gtk_text_iter_get_line_offset(&iter) + 1;
    
    const char *display_name = app->current_filename ? app->current_filename : "No File Open";
    char *msg = g_strdup_printf("Line: %d, Col: %d | %s", row, col, display_name);
     
    gtk_statusbar_pop(GTK_STATUSBAR(app->statusbar), app->status_context_id);
    gtk_statusbar_push(GTK_STATUSBAR(app->statusbar), app->status_context_id, msg); 
    g_free(msg);
}

/* --- Implementation of a very base metal audio streamer --- */
static void ui_open_basic_audio_player(AppState *app, const char *url) {
    
    app->audio_popup = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(app->audio_popup), 755, 250); 
    gtk_window_set_transient_for(GTK_WINDOW(app->audio_popup), GTK_WINDOW(app->window));
    gtk_window_set_resizable(GTK_WINDOW(app->audio_popup), FALSE);
    app->audio_webview = webkit_web_view_new();
    gtk_container_add(GTK_CONTAINER(app->audio_popup), app->audio_webview);

    /* NULL Pointer Verification */
    if (!app || !app->window) {
        g_printerr("Error: Invalid application state in ui_open_audio_player.\n");
        return;
    }

    if (!url || strlen(url) == 0) {
        ui_set_status_alert(app, "Error: Invalid or empty audio stream URL.", TRUE);
        return;
    }         

char html_player[4096]; 
    snprintf(html_player, sizeof(html_player),
        "<html><head><style>"
        "body { background: #000; color: #00ff00; font-family: monospace; padding: 15px; margin: 0; }"
        "#stats { font-size: 11px; margin-top: 15px; text-transform: uppercase; }"
        "button { background: #000; border: 2px solid #00ff00; padding: 5px 15px; cursor: pointer; "
        "         font-family: monospace; margin-right: 5px; color: #00ff00; transition: 0.2s; }"
        "button:hover { background: #002200; color: #00EFFF; border-color: #00EFFF; }"
        "input[type=range] { accent-color: #00ff00; width: 100px; vertical-align: middle; }"
        "/* Meter Container with inline Text at the End */"
        ".meter-wrapper { display: inline-flex; align-items: center; gap: 10px; width: 75%%; vertical-align: middle; }"
        ".meter-container { flex-grow: 1; height: 12px; background: #111; border: 1px solid #00ff00; overflow: hidden; position: relative; }"
        ".meter-bar { height: 100%%; width: 0%%; background: #00ff00; transition: width 0.3s; }"
        ".meter-bar.active { background: linear-gradient(90deg, #005500, #00ff00, #00EFFF); animation: bufferPulse 1.5s infinite linear; width: 100%% !important; }"
        "#dataCounter { font-size: 11px; font-weight: bold; color: #00EFFF; min-width: 90px; }"
        "@keyframes bufferPulse { 0%% { opacity: 0.3; } 50%% { opacity: 1.0; } 100%% { opacity: 0.3; } }"
        "</style></head><body>"
        "<h1><u>Audio Stream Player</u></h1>"
        "<audio id='player' preload='auto'><source src='%s' type='audio/mpeg'></audio>"
        "  <button onclick=\"audio.play()\">Play</button>"
        "  <button onclick=\"audio.pause()\">Pause</button>"
        "  <button onclick=\"audio.pause(); audio.currentTime=0;\">Stop</button>"
        "  <label>Vol:</label> "
        "  <input type='range' id='volSlider' min='0' max='1' step='0.05' value='1' "
        "         oninput=\"audio.volume = this.value\">"
        "  <br/><br/>"
        "  <label>STREAM STATUS: </label>"
        "  <div class='meter-wrapper'>"
        "    <div class='meter-container'><div id='bufferBar' class='meter-bar'></div></div>"
        "    <div id='dataCounter'>0 KB</div>"
        "  </div>"
        "  <div id='stats'>Status: Initializing...</div>"
        "  <script>"            
        "  const audio = document.getElementById('player');"
        "  const stats = document.getElementById('stats');" 
        "  const bar = document.getElementById('bufferBar');"
        "  const dataCounter = document.getElementById('dataCounter');"
        "  const readyStateMap = ['EMPTY', 'METADATA', 'CURRENT_DATA', 'FUTURE_DATA', 'ENOUGH_DATA'];"
        "  const networkStateMap = ['EMPTY', 'IDLE', 'LOADING', 'NO_SOURCE'];"
        "  "
        "  /* Estimated stream transfer speed: 128 kbps (16 KB/sec) */"
        "  const BYTES_PER_SEC = 16 * 1024;"
        "  "
        "  const formatBytes = (bytes) => {"
        "    if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + ' KB';"
        "    return (bytes / (1024 * 1024)).toFixed(2) + ' MB';"
        "  };"
        "  "
        "  const updateStats = () => {"
        "    const cur = Math.floor(audio.currentTime);"
        "    const isPlaying = !audio.paused && audio.readyState >= 2;"
        "    "
        "    if (isPlaying) {"
        "      bar.classList.add('active');"
        "      /* Calculate total payload data received */"
        "      const estimatedBytes = cur * BYTES_PER_SEC;"
        "      dataCounter.textContent = formatBytes(estimatedBytes);"
        "    } else {"
        "      bar.classList.remove('active');"
        "      bar.style.width = '0%%';"
        "      if (audio.paused && cur === 0) dataCounter.textContent = '0 KB';"
        "    }"
        "    "
        "    stats.innerHTML = `<div style='display: grid; grid-template-columns: 1fr 1fr; gap: 6px;'>"
        "      <span>Status: ${audio.paused ? 'PAUSED' : 'PLAYING'}</span>"
        "      <span>Ready: ${readyStateMap[audio.readyState] || 'UNKNOWN'}</span>"
        "      <span>Time Elapsed: ${cur}s</span>"
        "      <span>Network: ${networkStateMap[audio.networkState] || 'UNKNOWN'}</span>"
        "      <span>Volume: ${Math.round(audio.volume * 100)}%%</span>"
        "    </div>`;"
        "  };"
        "  "
        "  audio.ontimeupdate = updateStats;"
        "  audio.onwaiting = () => { bar.classList.remove('active'); updateStats(); };"
        "  audio.onplaying = updateStats;"
        "  audio.onpause = updateStats;"
        "  audio.onprogress = updateStats;"
        "  audio.onvolumechange = () => { document.getElementById('volSlider').value = audio.volume; updateStats(); };"
        "  </script></body></html>", url);
    webkit_web_view_load_html(WEBKIT_WEB_VIEW(app->audio_webview), html_player, NULL);
    gtk_widget_show_all(app->audio_popup);
}

/* --- Implementation version 3 of my audio streamer --- */
static void ui_open_audio_player(AppState *app, const char *url) {
    if (!app || !app->window) return;
    if (!url || strlen(url) == 0) return; 
    /* Ensure the parent container widget exists and is valid */
    if (!app->sidebar_notebook || !GTK_IS_CONTAINER(app->sidebar_notebook)) {
        g_warning("DEBUG: ui_open_audio_player: sidebar_notebook object is NULL or not a valid GtkContainer.");
        return;
    }
    
    gboolean is_active = FALSE;
    if (app->audio_mode_item) {
        is_active = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(app->audio_mode_item));
    }
    
    if (is_active) {
    if (!app->embedded_audio_view) {
        app->embedded_audio_view = WEBKIT_WEB_VIEW(webkit_web_view_new());

        WebKitSettings *settings = webkit_web_view_get_settings(app->embedded_audio_view);
        webkit_settings_set_enable_media(settings, TRUE);
        webkit_settings_set_enable_webaudio(settings, TRUE);
        webkit_settings_set_media_playback_requires_user_gesture(settings, TRUE);

        /* Safely append to notebook */
        gtk_notebook_append_page(GTK_NOTEBOOK(app->sidebar_notebook),
                                 GTK_WIDGET(app->embedded_audio_view),
                                 gtk_label_new("Audio Stream"));
    gtk_widget_show(GTK_WIDGET(app->embedded_audio_view));
    webkit_web_view_load_uri(app->embedded_audio_view, url);
    }
   
   /* 2. Show child widget and its tab label */
        GtkWidget *child = GTK_WIDGET(app->embedded_audio_view);
        gtk_widget_show(child);

        GtkWidget *tab_label = gtk_notebook_get_tab_label(GTK_NOTEBOOK(app->sidebar_notebook), child);
        if (tab_label) {
            gtk_widget_show(tab_label);
        }

        /* 3. Load stream and focus page */
        webkit_web_view_load_uri(app->embedded_audio_view, "https://somafm.com/player/#/gslive");
        
        gint page_num = gtk_notebook_page_num(GTK_NOTEBOOK(app->sidebar_notebook), child);
        if (page_num != -1) {
            gtk_notebook_set_current_page(GTK_NOTEBOOK(app->sidebar_notebook), page_num);
        }

    } else {
        /* Hide child widget, tab label, and pause audio when toggled OFF */
        if (app->embedded_audio_view) {
            GtkWidget *child = GTK_WIDGET(app->embedded_audio_view);


            /* Hide widget */
            gtk_widget_hide(child);

            /* Hide tab header */
            GtkWidget *tab_label = gtk_notebook_get_tab_label(GTK_NOTEBOOK(app->sidebar_notebook), child);
            if (tab_label) {
                gtk_widget_hide(tab_label);
            }
        }
    }
    
    app->audio_popup = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(app->audio_popup), 755, 380); 
    gtk_window_set_transient_for(GTK_WINDOW(app->audio_popup), GTK_WINDOW(app->window));
    gtk_window_set_resizable(GTK_WINDOW(app->audio_popup), FALSE);
        
    app->audio_webview = webkit_web_view_new();
    gtk_container_add(GTK_CONTAINER(app->audio_popup), app->audio_webview);

    char html_player[8192]; 
    snprintf(html_player, sizeof(html_player),
        "<!DOCTYPE html><html><head><style>"
        "body { background: #000; color: #00ff00; font-family: monospace; padding: 15px; margin: 0; }"
        "#stats { font-size: 11px; margin-top: 10px; margin-bottom: 12px; text-transform: uppercase; }"
        "button { background: #000; border: 2px solid #00ff00; padding: 5px 15px; cursor: pointer; "
        "         font-family: monospace; margin-right: 5px; color: #00ff00; transition: 0.2s; }"
        "button:hover { background: #002200; color: #00EFFF; border-color: #00EFFF; }"
        "input[type=range] { accent-color: #00ff00; width: 100px; vertical-align: middle; }"
        ".meter-wrapper { display: inline-flex; align-items: center; gap: 10px; width: 75%%; vertical-align: middle; }"
        ".meter-container { flex-grow: 1; height: 12px; background: #111; border: 1px solid #00ff00; overflow: hidden; position: relative; }"
        ".meter-bar { height: 100%%; width: 0%%; background: #00ff00; transition: width 0.3s; }"
        ".meter-bar.active { background: linear-gradient(90deg, #005500, #00ff00, #00EFFF); animation: bufferPulse 1.5s infinite linear; width: 100%% !important; }"
        "#dataCounter { font-size: 11px; font-weight: bold; color: #00EFFF; min-width: 90px; }"
        "@keyframes bufferPulse { 0%% { opacity: 0.3; } 50%% { opacity: 1.0; } 100%% { opacity: 0.3; } }"
        
        "/* Custom-colored divider line */"
        ".viz-section { margin-top: 10px; border-top: 2px solid #00EFFF; box-shadow: 0 -2px 6px rgba(0, 239, 255, 0.3); padding-top: 10px; }"
        ".viz-header { font-size: 10px; color: #00EFFF; margin-bottom: 5px; letter-spacing: 1px; font-weight: bold; }"
        ".viz-container { display: flex; gap: 12px; align-items: center; }"
        "#waveformCanvas { background: #050505; border: 1px solid #00ff00; flex-grow: 1; height: 75px; }"
        ".peak-wrapper { display: flex; flex-direction: column; align-items: center; gap: 4px; }"
        ".peak-meter-frame { width: 18px; height: 75px; background: #050505; border: 1px solid #00ff00; position: relative; display: flex; align-items: flex-end; }"
        "#peakBar { width: 100%%; height: 0%%; background: linear-gradient(0deg, #00ff00 60%%, #ffff00 85%%, #ff0000 100%%); transition: height 0.05s ease-out; }"
        "#peakValue { font-size: 9px; color: #00ff00; width: 28px; text-align: center; }"
        "</style></head><body>"
        
        "<h1><u>Audio Stream Player</u></h1>"
        "<audio id='player' crossorigin='anonymous' preload='auto'><source src='%s' type='audio/mpeg'></audio>"
        "  <button onclick='startPlay()'>Play</button>"
        "  <button onclick='audio.pause()'>Pause</button>"
        "  <button onclick='audio.pause(); audio.currentTime=0;'>Stop</button>"
        "  <label>Vol:</label> "
        "  <input type='range' id='volSlider' min='0' max='1' step='0.05' value='1' "
        "         oninput='audio.volume = this.value'>"
        "  <br/><br/>"
        "  <label>STREAM STATUS: </label>"
        "  <div class='meter-wrapper'>"
        "    <div class='meter-container'><div id='bufferBar' class='meter-bar'></div></div>"
        "    <div id='dataCounter'>0 KB</div>"
        "  </div>"
        "  <div id='stats'>Status: Initializing...</div>"

        "  <div class='viz-section'>"
        "    <div class='viz-header'>DSP AUDIO ANALYSIS ENGINE</div>"
        "    <div class='viz-container'>"
        "      <canvas id='waveformCanvas' width='620' height='75'></canvas>"
        "      <div class='peak-wrapper'>"
        "        <div class='peak-meter-frame'><div id='peakBar'></div></div>"
        "        <div id='peakValue'>0dB</div>"
        "      </div>"
        "    </div>"
        "  </div>"

        "  <script>"            
        "  const audio = document.getElementById('player');"
        "  const stats = document.getElementById('stats');" 
        "  const bar = document.getElementById('bufferBar');"
        "  const dataCounter = document.getElementById('dataCounter');"
        "  const canvas = document.getElementById('waveformCanvas');"
        "  const canvasCtx = canvas.getContext('2d');"
        "  const peakBar = document.getElementById('peakBar');"
        "  const peakValue = document.getElementById('peakValue');"
        
        "  const readyStateMap = ['EMPTY', 'METADATA', 'CURRENT_DATA', 'FUTURE_DATA', 'ENOUGH_DATA'];"
        "  const networkStateMap = ['EMPTY', 'IDLE', 'LOADING', 'NO_SOURCE'];"
        "  const BYTES_PER_SEC = 16 * 1024;"

        "  let audioCtx, analyser, source, dataArray;"
        "  let isAudioEngineInit = false;"

        "  function initAudioEngine() {"
        "    if (isAudioEngineInit) return;"
        "    try {"
        "      audioCtx = new (window.AudioContext || window.webkitAudioContext)();"
        "      analyser = audioCtx.createAnalyser();"
        "      analyser.fftSize = 512;"
        "      source = audioCtx.createMediaElementSource(audio);"
        "      source.connect(analyser);"
        "      analyser.connect(audioCtx.destination);"
        "      dataArray = new Uint8Array(analyser.frequencyBinCount);"
        "      isAudioEngineInit = true;"
        "      renderVisualizers();"
        "    } catch(e) {"
        "      console.log('Web Audio CORS/Init Warning:', e);"
        "    }"
        "  }"

        "  function startPlay() {"
        "    initAudioEngine();"
        "    if (audioCtx && audioCtx.state === 'suspended') audioCtx.resume();"
        "    audio.play();"
        "  }"

        "  function renderVisualizers() {"
        "    requestAnimationFrame(renderVisualizers);"
        "    if (!isAudioEngineInit || audio.paused) return;"

        "    analyser.getByteTimeDomainData(dataArray);"
        "    canvasCtx.fillStyle = '#050505';"
        "    canvasCtx.fillRect(0, 0, canvas.width, canvas.height);"
        "    canvasCtx.lineWidth = 1.5;"
        "    canvasCtx.strokeStyle = '#00ff00';"
        "    canvasCtx.beginPath();"

        "    let sliceWidth = canvas.width * 1.0 / dataArray.length;"
        "    let x = 0;"
        "    let maxPeak = 0;"

        "    for (let i = 0; i < dataArray.length; i++) {"
        "      let v = dataArray[i] / 128.0;"
        "      let y = v * (canvas.height / 2);"

        "      let absVal = Math.abs(dataArray[i] - 128);"
        "      if (absVal > maxPeak) maxPeak = absVal;"

        "      if (i === 0) canvasCtx.moveTo(x, y);"
        "      else canvasCtx.lineTo(x, y);"
        "      x += sliceWidth;"
        "    }"
        "    canvasCtx.lineTo(canvas.width, canvas.height / 2);"
        "    canvasCtx.stroke();"

        "    let peakPct = Math.min(100, Math.round((maxPeak / 128.0) * 100));"
        "    peakBar.style.height = peakPct + '%%';"
        "    let dB = peakPct > 0 ? Math.round(20 * Math.log10(peakPct / 100)) : -60;"
        "    peakValue.textContent = dB + 'dB';"
        "  }"

        "  const formatBytes = (bytes) => {"
        "    if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + ' KB';"
        "    return (bytes / (1024 * 1024)).toFixed(2) + ' MB';"
        "  };"

        "  const updateStats = () => {"
        "    const cur = Math.floor(audio.currentTime);"
        "    const isPlaying = !audio.paused && audio.readyState >= 2;"
        "    "
        "    if (isPlaying) {"
        "      bar.classList.add('active');"
        "      const estimatedBytes = cur * BYTES_PER_SEC;"
        "      dataCounter.textContent = formatBytes(estimatedBytes);"
        "    } else {"
        "      bar.classList.remove('active');"
        "      bar.style.width = '0%%';"
        "      if (audio.paused && cur === 0) dataCounter.textContent = '0 KB';"
        "    }"
        "    "
        "    stats.innerHTML = `<div style=\"display: grid; grid-template-columns: 1fr 1fr; gap: 6px;\">"
        "      <span>Status: ${audio.paused ? 'PAUSED' : 'PLAYING'}</span>"
        "      <span>Ready: ${readyStateMap[audio.readyState] || 'UNKNOWN'}</span>"
        "      <span>Time Elapsed: ${cur}s</span>"
        "      <span>Network: ${networkStateMap[audio.networkState] || 'UNKNOWN'}</span>"
        "      <span>Volume: ${Math.round(audio.volume * 100)}%%</span>"
        "    </div>`;"
        "  };"

        "  audio.ontimeupdate = updateStats;"
        "  audio.onwaiting = () => { bar.classList.remove('active'); updateStats(); };"
        "  audio.onplaying = updateStats;"
        "  audio.onpause = updateStats;"
        "  audio.onprogress = updateStats;"
        "  audio.onvolumechange = () => { document.getElementById('volSlider').value = audio.volume; updateStats(); };"
        "  </script></body></html>", url);

    webkit_web_view_load_html(WEBKIT_WEB_VIEW(app->audio_webview), html_player, NULL);
    
    gtk_widget_show_all(app->audio_popup);
}

static void ui_apply_styles(void) {
    GtkCssProvider *provider;
    GdkDisplay *display;
    GdkScreen *screen;

    /* Define CSS directly in code for portability */
  const gchar *css_data = 
    "window { background-color: #000000; }"
        "treeview { background-color: #0d0d0d; color: #00ff00; border-right: 1px solid #333; }"
        
        /* Editor styling */
        "textview text { background-color: #000000; color: #dcdcdc; font-family: 'Monospace'; font-size: 11pt; }"
        "gutter { background-color: #000000; color: #666; padding-right: 8px; border-right: 1px solid #222; }"
        "textview text cursor-line { background-color: #2a2a2a; color: #00ff00; }"
        "textview gutter cursor-line { background-color: #2a2a2a; }"
        
        /* --- Pure White Labels --- */
        /* Targeting the label inside the menuitem is the standard way to change text color 
        "menubar > menuitem label { color: #ffffff; font-weight: bold; padding: 0 4px; }"
        "menu menuitem label { color: #ffffff; }"
        */ 
        
        "menubar > menuitem label { color: #00ff00; font-weight: bold; padding: 0 4px; }"
        "menu menuitem label { color: #2a3b4c; }"
    
        /* Clean and highly legible hover state for all menu / submenu items */
        "menu menuitem:hover { background-color: #2a3b4c; border-left: 3px solid #5294e2; }"
        "menu menuitem:hover label { color: #00ff00; font-weight: bold; }"
        
        /* Hover Effects */
        "menubar > menuitem:hover { background-color: #2a3b4c; }"
        "menubar > menuitem:hover label { color: #00ff0a; }"
        
        /* Targeting Nested Sub-menus specifically */
        "menu > menuitem > menu > menuitem { background-color: #333; border: 1px solid #00ff0a; }"
        "menu > menuitem > menu > menuitem label { color: #39ff14; font-weight: bold; }"
        
        "menubar { background-color: #111; border-bottom: 1px solid #333; }"
        "menu { background-color: #100; border: 1px solid #888; }"
        

        /* Hover state for the sub-menu items */     
        "menu > menuitem > menu menuitem:hover { background-color: #333; }" 
        "menu > menuitem > menu menuitem:hover label { color: #00ff0a; }"

        "menu menuitem:hover { background-color: #2a3b4c; }"
        "menu menuitem:hover label { color: #399f14; }"

        /* Notebook & Tabs Styling */
        "notebook header { background-color: #3D3846; border-bottom: 1px solid #FFFFFF; border: 1px solid #FFFFFF; }"
        "notebook header tab { background-color: #0d0d0d; color: #666; border: 1px solid #222;  border-bottom: 1px solid #FFFFFF; padding: 4px 10px; }"
        "notebook header tab:hover { background-color: #1a1a1a; color: #00ff00; }"
        "notebook header tab:checked { background-color: #000000; color: #00ff00; border-top: 2px solid #00ff00; border-bottom: 1px solid #FFFFFF; }"
        "notebook header tab:not(:checked) { background-color: #0d0d0d; color: #666666; border: 1px solid #222222; border-bottom: 1px solid #FFFFFF; padding: 4px 10px; }"        
        "notebook header tab:not(:checked):hover { background-color: #1a1a1a; color: #00ff00; }"

        /* Buttons and Entries */
        "entry { background-color: #1a1a1a; color: #00ff00; border: 1px solid #333; }"
        "button { background-color: #222; color: #eee; border: 1px solid #444; }";

    provider = gtk_css_provider_new();
    display = gdk_display_get_default();
    screen = gdk_display_get_default_screen(display);

    gtk_css_provider_load_from_data(provider, css_data, -1, NULL);

    /* Apply to the entire screen so all widgets inherit the style */
    gtk_style_context_add_provider_for_screen(screen, 
        GTK_STYLE_PROVIDER(provider), 
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    g_object_unref(provider);
}

/* basic Color picker bialog box - Your basic html editor lookup tool */
static void cb_on_color_picker(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    GtkWidget *dialog = gtk_color_chooser_dialog_new("Select a Color", GTK_WINDOW(app->window));
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));    
    if (response == GTK_RESPONSE_OK) {
        GdkRGBA color;
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(dialog), &color);
        int r = (int)(color.red * 255);
        int g = (int)(color.green * 255);
        int b = (int)(color.blue * 255);
        char hex_code[32];
        snprintf(hex_code, sizeof(hex_code), "#%02X%02X%02X", r, g, b);
        if (app->editor_buffer != NULL) {
            gtk_text_buffer_insert_at_cursor(app->editor_buffer, hex_code, -1);
        }
        GtkWidget *msg_dialog = gtk_message_dialog_new(GTK_WINDOW(app->window),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_OK,
            NULL);
            
        char *markup_text = g_strdup_printf(
            "Color Code Inserted!\n\n<b><span size='large' foreground='%s'>%s</span></b>",
            hex_code, hex_code);
            
        gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(msg_dialog), markup_text);
        g_free(markup_text);
            
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(msg_dialog), 
            "RGB Values: rgb(%d, %d, %d)\nInserted directly into your document.", r, g, b);
        char *status_msg = g_strdup_printf("Inserted color: %s", hex_code);
        ui_set_status_alert(app, status_msg, FALSE);
        g_free(status_msg);
        gtk_dialog_run(GTK_DIALOG(msg_dialog));
        gtk_widget_destroy(msg_dialog);
    }
    logic_update_dialog_closed_message(app);
    gtk_widget_destroy(dialog);
}

/* HTML TOOLS for this editor */
static void cb_on_wrap_tag_dialog(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    if (!app->editor_buffer) return;

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Wrap in Tag",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Insert", GTK_RESPONSE_OK,
        NULL);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);
    
    GtkWidget *label = gtk_label_new("HTML Tag (e.g. div, p, strong):");
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), "div");
    gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);

    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(content_area), hbox);
    gtk_widget_show_all(dialog);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *tag = gtk_entry_get_text(GTK_ENTRY(entry));
        if (strlen(tag) > 0) {
            GtkTextIter start, end;            
            if (gtk_text_buffer_get_selection_bounds(app->editor_buffer, &start, &end)) {
                char *selected_text = gtk_text_buffer_get_text(app->editor_buffer, &start, &end, FALSE);
                char *wrapped = g_strdup_printf("<%s>%s</%s>", tag, selected_text, tag);
                
                gtk_text_buffer_begin_user_action(app->editor_buffer);
                gtk_text_buffer_delete(app->editor_buffer, &start, &end);
                gtk_text_buffer_insert(app->editor_buffer, &start, wrapped, -1);
                gtk_text_buffer_end_user_action(app->editor_buffer);
                
                g_free(selected_text);
                g_free(wrapped);
            } else {
                char *template = g_strdup_printf("<%s></%s>", tag, tag);
                gtk_text_buffer_insert_at_cursor(app->editor_buffer, template, -1);
                GtkTextMark *insert_mark = gtk_text_buffer_get_insert(app->editor_buffer);
                gtk_text_buffer_get_iter_at_mark(app->editor_buffer, &start, insert_mark);
                gtk_text_iter_backward_chars(&start, strlen(tag) + 3); 
                gtk_text_buffer_place_cursor(app->editor_buffer, &start);
                g_free(template);
            }
            ui_set_status_alert(app, "HTML Tag wrapped/inserted", FALSE);
        }
    }
    logic_update_dialog_closed_message(app);
    gtk_widget_destroy(dialog);
}

static void cb_on_html_escape(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    if (!app->editor_buffer) return;

    GtkTextIter start, end;
    if (!gtk_text_buffer_get_selection_bounds(app->editor_buffer, &start, &end)) {
        ui_set_status_alert(app, "Please select text to escape first.", TRUE);
        return;
    }

    char *selected_text = gtk_text_buffer_get_text(app->editor_buffer, &start, &end, FALSE);
    /* g_markup_escape_text converts <, >, &, ', " instantly into valid entities */
    char *escaped_text = g_markup_escape_text(selected_text, -1);

    gtk_text_buffer_begin_user_action(app->editor_buffer);
    gtk_text_buffer_delete(app->editor_buffer, &start, &end);
    gtk_text_buffer_insert(app->editor_buffer, &start, escaped_text, -1);
    gtk_text_buffer_end_user_action(app->editor_buffer);

    g_free(selected_text);
    g_free(escaped_text);
    ui_set_status_alert(app, "Text converted to secure HTML entities.", FALSE);
}

static void cb_on_insert_link_dialog(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    if (!app->editor_buffer) return;

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Insert Hyperlink",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Insert Link", GTK_RESPONSE_OK,
        NULL);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    
    GtkWidget *label = gtk_label_new("Target URL:");
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), "https://");
    gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);

    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), entry, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);
    gtk_widget_show_all(dialog);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *url = gtk_entry_get_text(GTK_ENTRY(entry));
        GtkTextIter start, end;
        
        if (gtk_text_buffer_get_selection_bounds(app->editor_buffer, &start, &end)) {
            char *text = gtk_text_buffer_get_text(app->editor_buffer, &start, &end, FALSE);
            char *anchor = g_strdup_printf("<a href=\"%s\">%s</a>", url, text);
            
            gtk_text_buffer_begin_user_action(app->editor_buffer);
            gtk_text_buffer_delete(app->editor_buffer, &start, &end);
            gtk_text_buffer_insert(app->editor_buffer, &start, anchor, -1);
            gtk_text_buffer_end_user_action(app->editor_buffer);
            
            g_free(text);
            g_free(anchor);
        } else {
            char *anchor = g_strdup_printf("<a href=\"%s\">Link Text</a>", url);
            gtk_text_buffer_insert_at_cursor(app->editor_buffer, anchor, -1);
            g_free(anchor);
        }
        ui_set_status_alert(app, "Hyperlink inserted successfully.", FALSE);
    }
    logic_update_dialog_closed_message(app);
    gtk_widget_destroy(dialog);
}

static void cb_on_insert_meta_boilerplate(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    if (!app->editor_buffer) return;
    GDateTime *now = g_date_time_new_now_local();
    /* Format it as an ISO 8601 string (e.g., 2026-05-22T01:30:00) */ 
    char *time_str = g_date_time_format(now, "%Y-%m-%dT%H:%M:%S");
    /* Construct the clean template, safely providing exactly 1 argument for %s */
    char *boilerplate = g_strdup_printf(
        "<meta charset=\"UTF-8\">\n"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        "<meta property=\"article:published_time\" content=\"%s\">\n",
        time_str);
    /* Inject it into the editor right at the cursor position */
    gtk_text_buffer_insert_at_cursor(app->editor_buffer, boilerplate, -1);    
    /* Clean up memory allocations to prevent leaks */
    g_free(time_str);
    g_date_time_unref(now);
    g_free(boilerplate);    
    ui_set_status_alert(app, "Meta layout with timestamp appended.", FALSE);
}

static void cb_on_html_unescape(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    if (!app->editor_buffer) return;

    GtkTextIter start, end;
    if (!gtk_text_buffer_get_selection_bounds(app->editor_buffer, &start, &end)) {
        ui_set_status_alert(app, "Please select entity text to decode first.", TRUE);
        return;
    }

    char *selected_text = gtk_text_buffer_get_text(app->editor_buffer, &start, &end, FALSE);
    
    /* Reverse g_markup_escape_text mappings manually or safely parsing entities
     * Let's perform standard manual string translation for entities 
     */
    GString *decoded = g_string_new(selected_text);
    g_string_replace(decoded, "&lt;", "<", 0);
    g_string_replace(decoded, "&gt;", ">", 0);
    g_string_replace(decoded, "&amp;", "&", 0);
    g_string_replace(decoded, "&quot;", "\"", 0);
    g_string_replace(decoded, "&apos;", "'", 0);

    gtk_text_buffer_begin_user_action(app->editor_buffer);
    gtk_text_buffer_delete(app->editor_buffer, &start, &end);
    gtk_text_buffer_insert(app->editor_buffer, &start, decoded->str, -1);
    gtk_text_buffer_end_user_action(app->editor_buffer);

    g_string_free(decoded, TRUE);
    g_free(selected_text);
    ui_set_status_alert(app, "HTML Entities converted back to raw symbols.", FALSE);
}

static void cb_on_insert_image_dialog(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    if (!app->editor_buffer) return;

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Insert Image Element",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Insert Tag", GTK_RESPONSE_OK,
        NULL);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);

    GtkWidget *entry_src = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_src), "image-source.jpg");
    GtkWidget *entry_alt = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_alt), "Describe the image");

    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("Source URL / Path (src):"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), entry_src, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("Alternative Text (alt):"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), entry_alt, FALSE, FALSE, 5);

    gtk_container_add(GTK_CONTAINER(content_area), vbox);
    gtk_widget_show_all(dialog);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *src = gtk_entry_get_text(GTK_ENTRY(entry_src));
        const char *alt = gtk_entry_get_text(GTK_ENTRY(entry_alt));
        char *img_tag = g_strdup_printf("<img src=\"%s\" alt=\"%s\" loading=\"lazy\">\n", 
                                        strlen(src) > 0 ? src : "placeholder.jpg", alt);
        
        gtk_text_buffer_insert_at_cursor(app->editor_buffer, img_tag, -1);
        g_free(img_tag);
        ui_set_status_alert(app, "Inserted <img> tag skeleton.", FALSE);
    }
    logic_update_dialog_closed_message(app);
    gtk_widget_destroy(dialog);
}

static void cb_on_insert_lorem_ipsum(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    if (!app->editor_buffer) return;

    const char *lorem = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, "
                        "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
                        "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris "
                        "nisi ut aliquip ex ea commodo consequat.";

    gtk_text_buffer_insert_at_cursor(app->editor_buffer, lorem, -1);
    ui_set_status_alert(app, "Lorem Ipsum dummy text inserted.", FALSE);
}

static void cb_on_insert_table_dialog(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    if (!app->editor_buffer) return;

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Create HTML Table",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Generate Table", GTK_RESPONSE_OK,
        NULL);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 12);

    GtkWidget *spin_rows = gtk_spin_button_new_with_range(1, 20, 1);
    GtkWidget *spin_cols = gtk_spin_button_new_with_range(1, 10, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_rows), 3);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_cols), 3);

    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Rows Count:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), spin_rows, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Columns Count:"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), spin_cols, 1, 1, 1, 1);

    gtk_container_add(GTK_CONTAINER(content_area), grid);
    gtk_widget_show_all(dialog);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        int rows = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_rows));
        int cols = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_cols));

        GString *table_str = g_string_new("<table>\n");
        g_string_append(table_str, "  <thead>\n    <tr>\n");
        for (int c = 0; c < cols; c++) {
            g_string_append_printf(table_str, "      <th>Header %d</th>\n", c + 1);
        }
        g_string_append(table_str, "    </tr>\n  </thead>\n  <tbody>\n");
        for (int r = 0; r < rows; r++) {
            g_string_append(table_str, "    <tr>\n");
            for (int c = 0; c < cols; c++) {
                g_string_append(table_str, "      <td>Data Cell</td>\n");
            }
            g_string_append(table_str, "    </tr>\n");
        }
        g_string_append(table_str, "  </tbody>\n</table>\n");

        gtk_text_buffer_insert_at_cursor(app->editor_buffer, table_str->str, -1);
        g_string_free(table_str, TRUE);
        ui_set_status_alert(app, "HTML Grid Table Blueprint appended.", FALSE);
    }
    gtk_widget_destroy(dialog);
}

static void cb_on_insert_full_html_page(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    if (!app->editor_buffer) return;
    GDateTime *now = g_date_time_new_now_local();
    char *time_str = g_date_time_format(now, "%Y-%m-%dT%H:%M:%S");
    char *html_template = g_strdup_printf(
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "    <meta charset=\"UTF-8\">\n"
        "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        "    <title>Web Document</title>\n"
        "    \n"
        "    \n"
        "    <meta property=\"article:published_time\" content=\"%s\">\n"
        "    <meta name=\"description\" content=\"Document created in Crystalcat MDE\">\n"
        "    \n"
        "    \n"
        "    <style>\n"
        "        :root {\n"
        "            --primary-color: #2c3e50;\n"
        "            --accent-color: #3498db;\n"
        "            --bg-color: #f8f9fa;\n"
        "            --text-color: #333333;\n"
        "        }\n"
        "        body {\n"
        "            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;\n"
        "            line-height: 1.6;\n"
        "            background-color: var(--bg-color);\n"
        "            color: var(--text-color);\n"
        "            max-width: 800px;\n"
        "            margin: 40px auto;\n"
        "            padding: 0 20px;\n"
        "        }\n"
        "        h1 {\n"
        "            color: var(--primary-color);\n"
        "            border-bottom: 2px solid var(--accent-color);\n"
        "            padding-bottom: 10px;\n"
        "        }\n"
        "        footer {\n"
        "            margin-top: 50px;\n"
        "            font-size: 0.85em;\n"
        "            color: #7f8c8d;\n"
        "            text-align: center;\n"
        "        }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <header>\n"
        "        <h1>Welcome to </h1>\n"
        "    </header>\n"
        "    \n"
        "    <main>\n"
        "        <p> </p>\n"
        "    </main>\n"
        "    \n"
        "    <footer>\n"
        "        <p>&copy; Document generated natively via Crystalcat Editor</p>\n"
        "    </footer>\n"
        "</body>\n"
        "</html>\n",
        time_str);
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(app->editor_buffer, &start, &end);
    
    if (gtk_text_iter_equal(&start, &end)) {
        gtk_text_buffer_set_text(app->editor_buffer, html_template, -1);
        ui_set_status_alert(app, "Full HTML5 Document generated into workspace.", FALSE);
    } else {
        gtk_text_buffer_insert_at_cursor(app->editor_buffer, html_template, -1);
        ui_set_status_alert(app, "HTML5 Document Template injected at cursor point.", FALSE);
    }

    g_free(time_str);
    g_date_time_unref(now);
    g_free(html_template);
}

static void cb_on_wrap_html_comment(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    if (!app->editor_buffer) return;

    GtkTextIter start, end;
    if (gtk_text_buffer_get_selection_bounds(app->editor_buffer, &start, &end)) {
        char *text = gtk_text_buffer_get_text(app->editor_buffer, &start, &end, FALSE);
        char *commented = g_strdup_printf("// %s", text);

        gtk_text_buffer_begin_user_action(app->editor_buffer);
        gtk_text_buffer_delete(app->editor_buffer, &start, &end);
        gtk_text_buffer_insert(app->editor_buffer, &start, commented, -1);
        gtk_text_buffer_end_user_action(app->editor_buffer);

        g_free(text);
        g_free(commented);
        ui_set_status_alert(app, "Selection hidden inside HTML comment block.", FALSE);
    } else {
        ui_set_status_alert(app, "Select text to comment it out.", TRUE);
    }
}

static void cb_on_wrap_unordered_list(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    if (!app->editor_buffer) return;

    GtkTextIter start, end;
    if (gtk_text_buffer_get_selection_bounds(app->editor_buffer, &start, &end)) {
        char *text = gtk_text_buffer_get_text(app->editor_buffer, &start, &end, FALSE);
        
        /* Split selection by line returns to create individual list items */
        char **lines = g_strsplit(text, "\n", -1);
        GString *list_builder = g_string_new("<ul>\n");
        
        for (int i = 0; lines[i] != NULL; i++) {
            if (strlen(g_strstrip(lines[i])) > 0) { /* skip empty space strings */ 
                g_string_append_printf(list_builder, "    <li>%s</li>\n", lines[i]);
            }
        }
        g_string_append(list_builder, "</ul>\n");

        gtk_text_buffer_begin_user_action(app->editor_buffer);
        gtk_text_buffer_delete(app->editor_buffer, &start, &end);
        gtk_text_buffer_insert(app->editor_buffer, &start, list_builder->str, -1);
        gtk_text_buffer_end_user_action(app->editor_buffer);

        g_strfreev(lines);
        g_string_free(list_builder, TRUE);
        g_free(text);
        ui_set_status_alert(app, "Lines wrapped into Unordered List list-items.", FALSE);
    } else {
        /* No selection: Insert an empty standard list structure skeleton */
        gtk_text_buffer_insert_at_cursor(app->editor_buffer, "<ul>\n    <li>Item 1</li>\n    <li>Item 2</li>\n</ul>\n", -1);
        ui_set_status_alert(app, "Inserted generic Bullet List skeletal markup.", FALSE);
    }
}

/* Wrapper to handle the 4 arguments of the mark-set signal */
static void cb_mark_set(GtkTextBuffer *buffer, GtkTextIter *location, GtkTextMark *mark, gpointer user_data) {
    ui_update_statusbar(buffer, user_data);
}
/* status ui alert test */ 
static gboolean cb_clear_status_alert(gpointer data) {
    AppState *app = (AppState *)data;

    if (app && app->statusbar) {
        gtk_statusbar_pop(GTK_STATUSBAR(app->statusbar), app->status_context_id);
    }

    /* * CRITICAL: Returning G_SOURCE_REMOVE (or FALSE) tells 
     * the timer to stop. If you return TRUE, it will loop forever! 
     */
    return G_SOURCE_REMOVE;
}

static void ui_set_status_alert(AppState *app, const char *msg, gboolean is_error) {
    if (!app->statusbar) return;

    /* Clear previous message */
    gtk_statusbar_pop(GTK_STATUSBAR(app->statusbar), app->status_context_id);
        
    char *formatted_msg;
    if (is_error) {
        formatted_msg = g_strdup_printf("!! ALERT: %s !!", msg);
    } else {
        formatted_msg = g_strdup_printf("> %s", msg);
    }

    gtk_statusbar_push(GTK_STATUSBAR(app->statusbar), app->status_context_id, formatted_msg);
    app->status_context_id = g_timeout_add(19000, (GSourceFunc)cb_clear_status_alert, app); 
    g_timeout_add(8000, (GSourceFunc)cb_clear_status_alert, app); 
    g_free(formatted_msg);
}

/* --- UI Construction Modules --- */
static void ui_build_layout(AppState *app) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    
    gtk_container_add(GTK_CONTAINER(app->window), vbox);
    /* List out all my steps so i get this correct, 
     1. Create ALL your widgets first */ 
     
    app->statusbar = gtk_statusbar_new();
    
    /* CRITICAL: Set the name so the CSS ID selector (#status-text-area) works*/
    gtk_widget_set_name(app->statusbar, "status-text-area");
    
    app->status_context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(app->statusbar), "info");
    gtk_statusbar_push(GTK_STATUSBAR(app->statusbar), app->status_context_id, "Ready.");
    app->color_toggle = 0; 
    ui_set_status_alert(app, " MDE SYSTEM LOADING..........", FALSE); 
    GtkWidget *menu = ui_create_menu(app);
    GtkWidget *search_section = ui_create_search_section(app);
    GtkWidget *workspace = ui_create_workspace(app); 
    GtkWidget *console_panel = ui_create_console_panel(app);
    
    /* 2. Pack them all into the layout */

    gtk_box_pack_start(GTK_BOX(vbox), menu, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), search_section, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), workspace, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), app->statusbar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), console_panel, FALSE, FALSE, 0);
    
    ui_build_assistant_panel(vbox, app);

    /* 3. Connect signals */
    if (app->editor_buffer) {
        g_signal_connect(app->editor_buffer, "changed", G_CALLBACK(ui_update_statusbar), app);
        g_signal_connect(app->editor_buffer, "mark-set", G_CALLBACK(cb_mark_set), app);
        g_signal_connect(app->editor_buffer, "changed", G_CALLBACK(cb_on_editor_buffer_changed), app);
    }

    if (app->console_view) {
        g_signal_connect(app->console_view, "key-press-event", G_CALLBACK(cb_console_key_press), app);
    }
    
    g_signal_connect(app->window, "destroy", G_CALLBACK(cb_on_destroy), app);   
    
    /* 4. Setup the Smooth Transition Provider */
    app->status_css_provider = gtk_css_provider_new();
    
    /* Change PRIORITY_APPLICATION to PRIORITY_USER to ensure it overrides global styles */
gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                              GTK_STYLE_PROVIDER(app->status_css_provider), 
                                              GTK_STYLE_PROVIDER_PRIORITY_USER);
     app->color_toggle = 0;                             
    g_timeout_add(599, cb_rotate_status_color, app); 
    update_rabbit_animation_state(app);   
    gtk_widget_show_all(app->window);
    gtk_window_present(GTK_WINDOW(app->window));
    logic_spawn_terminal(app);
}

static GtkWidget* ui_create_menu(AppState *app) {
    GtkWidget *bar;
    GtkWidget *file_item, *file_menu;
    GtkWidget *edit_item, *edit_menu;
    GtkWidget *search_menu_item, *search_menu, *ddg_item;  
    GtkWidget *open_item, *save_item, *save_as_item, *sep, *quit_item;  
    GtkWidget *edit_sep;
    GtkWidget *outline_item, *snippet_item;

    /* Create the Accelerator Group */
    GtkAccelGroup *accel_group = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(app->window), accel_group);
    
    bar = gtk_menu_bar_new();

    /* --- File Menu --- */
    file_item = gtk_menu_item_new_with_mnemonic("_File");
    file_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_item), file_menu);
   
    open_item = gtk_menu_item_new_with_label("Open");

    /* File -> Open Local HTML File*/
    GtkWidget *open_html_item = gtk_menu_item_new_with_mnemonic("Open Local _HTML...");
    g_signal_connect(open_html_item, "activate", G_CALLBACK(cb_on_open_local_html), app);
    
    GtkWidget *quick_save_item = gtk_menu_item_new_with_label("Quick Save");
    save_item = gtk_menu_item_new_with_label("Save");
    save_as_item = gtk_menu_item_new_with_label("Save As...");
        
    GtkWidget *new_session_item = gtk_menu_item_new_with_label("New");
    g_signal_connect(new_session_item, "activate", G_CALLBACK(cb_on_new_session_save_prompt), app);

    
    sep = gtk_separator_menu_item_new();
    quit_item = gtk_menu_item_new_with_label("Quit");

    /* Assign Shortcuts to File Items */
    gtk_widget_add_accelerator(open_item, "activate", accel_group, 
                               GDK_KEY_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(save_item, "activate", accel_group, 
                               GDK_KEY_s, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(save_as_item, "activate", accel_group, 
                               GDK_KEY_s, GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(quit_item, "activate", accel_group, 
                               GDK_KEY_q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(new_session_item, "activate", accel_group, 
                               GDK_KEY_n, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(quick_save_item, "activate", accel_group, 
                               GDK_KEY_w, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
                          
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), open_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), open_html_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), new_session_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), quick_save_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), save_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), save_as_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), sep);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), quit_item);

    /* --- Edit Menu --- */
    edit_item = gtk_menu_item_new_with_mnemonic("_Edit");
    edit_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(edit_item), edit_menu);
    
    outline_item = gtk_menu_item_new_with_label("Generate Code Outline");      
    snippet_item = gtk_menu_item_new_with_label("Insert Snippet");

    /* Assign Shortcuts to Edit Items */
    gtk_widget_add_accelerator(outline_item, "activate", accel_group, 
                               GDK_KEY_r, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(snippet_item, "activate", accel_group, 
                               GDK_KEY_j, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE); 

    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), outline_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), snippet_item);


    edit_sep = gtk_separator_menu_item_new();
    GtkWidget *find_item = gtk_menu_item_new_with_label("Find...");
    GtkWidget *goto_item = gtk_menu_item_new_with_label("Go to Line...");
    GtkWidget *cookie_mgr_item = gtk_menu_item_new_with_label("Manage Cookies...");
    GtkWidget *color_item = gtk_menu_item_new_with_label("Color Picker...");
    
    /* Add Accelerators (Shortcuts) */
    gtk_widget_add_accelerator(find_item, "activate", accel_group, GDK_KEY_f, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(goto_item, "activate", accel_group, GDK_KEY_g, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    
    /* HTML TOOLS  */
    GtkWidget *html_menu_item = gtk_menu_item_new_with_label("HTML Tools");
    GtkWidget *html_submenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(html_menu_item), html_submenu);

    GtkWidget *blueprint_item = gtk_menu_item_new_with_label("New HTML5 Layout Blueprint (with CSS)");
    g_signal_connect(blueprint_item, "activate", G_CALLBACK(cb_on_insert_full_html_page), app);
    gtk_menu_shell_prepend(GTK_MENU_SHELL(html_submenu), blueprint_item); // places at the very top

    /* Inject a divider bar below the full template item for professional formatting layout */
     gtk_menu_shell_append(GTK_MENU_SHELL(html_submenu), gtk_separator_menu_item_new());

    /* Add List Wrapper */
    GtkWidget *list_item = gtk_menu_item_new_with_label("Convert Lines to Bulleted List (<ul>)");
    g_signal_connect(list_item, "activate", G_CALLBACK(cb_on_wrap_unordered_list), app);
    gtk_menu_shell_append(GTK_MENU_SHELL(html_submenu), list_item);

    /* Add Comment Block */
    GtkWidget *comment_item = gtk_menu_item_new_with_label("Comment Out Current Selection ()");
    g_signal_connect(comment_item, "activate", G_CALLBACK(cb_on_wrap_html_comment), app);
    gtk_menu_shell_append(GTK_MENU_SHELL(html_submenu), comment_item);

    /*  Wrap Tag Item */
    GtkWidget *wrap_item = gtk_menu_item_new_with_label("Wrap in Custom Tag");
    g_signal_connect(wrap_item, "activate", G_CALLBACK(cb_on_wrap_tag_dialog), app);
    gtk_menu_shell_append(GTK_MENU_SHELL(html_submenu), wrap_item);

    /* Entity Escape Item */
    GtkWidget *escape_item = gtk_menu_item_new_with_label("Escape Selection to Entities");
    g_signal_connect(escape_item, "activate", G_CALLBACK(cb_on_html_escape), app);
    gtk_menu_shell_append(GTK_MENU_SHELL(html_submenu), escape_item);

    /* Anchor Link Item */
    GtkWidget *link_item = gtk_menu_item_new_with_label("Insert Hyperlink Anchor");
    g_signal_connect(link_item, "activate", G_CALLBACK(cb_on_insert_link_dialog), app);
    gtk_menu_shell_append(GTK_MENU_SHELL(html_submenu), link_item);

   /* Meta Template Item */
   GtkWidget *meta_item = gtk_menu_item_new_with_label("Insert Meta Boilerplate");
   g_signal_connect(meta_item, "activate", G_CALLBACK(cb_on_insert_meta_boilerplate), app);
   gtk_menu_shell_append(GTK_MENU_SHELL(html_submenu), meta_item);

   /* Unescape Selection */
   GtkWidget *unescape_item = gtk_menu_item_new_with_label("Decode HTML Entities");
   g_signal_connect(unescape_item, "activate", G_CALLBACK(cb_on_html_unescape), app);
   gtk_menu_shell_append(GTK_MENU_SHELL(html_submenu), unescape_item);

   /* Insert Image Element */
   GtkWidget *img_item = gtk_menu_item_new_with_label("Insert Image Tag");
   g_signal_connect(img_item, "activate", G_CALLBACK(cb_on_insert_image_dialog), app);
   gtk_menu_shell_append(GTK_MENU_SHELL(html_submenu), img_item);

   /* Placeholder Generator 
    * It's actually a scrambled piece of 1st-century BC classical Latin 
    * literature written by Cicero (De Finibus Bonorum et Malorum). 
    * Typesetters in the 1500s scrambled the words to make a specimen book, 
    * and it became the absolute gold standard for the printing and digital industries. */
   GtkWidget *lorem_item = gtk_menu_item_new_with_label("Insert Lorem Ipsum Paragraph");
   g_signal_connect(lorem_item, "activate", G_CALLBACK(cb_on_insert_lorem_ipsum), app);
   gtk_menu_shell_append(GTK_MENU_SHELL(html_submenu), lorem_item);

   /* Dynamic Table Wizard */
   GtkWidget *table_item = gtk_menu_item_new_with_label("Generate Layout Table...");
   g_signal_connect(table_item, "activate", G_CALLBACK(cb_on_insert_table_dialog), app);
   gtk_menu_shell_append(GTK_MENU_SHELL(html_submenu), table_item);
   
    GtkWidget *calc_item = gtk_menu_item_new_with_mnemonic("Calculator");
    g_signal_connect(calc_item, "activate", G_CALLBACK(on_calculator_menu_activate), app);

     /* Add Font Size Option under Edit Menu */
     GtkWidget *font_size_item = gtk_menu_item_new_with_label("Font Size...");
     g_signal_connect(G_OBJECT(font_size_item), "activate", 
                 G_CALLBACK(cb_on_change_font_size), app);
    
    /* Font Type Selection Action */
    GtkWidget *font_type_item = gtk_menu_item_new_with_label("Select Font Type...");
    g_signal_connect(font_type_item, "activate", G_CALLBACK(cb_on_select_font_type), app);           
 
    /* Toggle Comment */
    GtkWidget *comment_item_1 = gtk_menu_item_new_with_label("Toggle Comment");
    gtk_widget_add_accelerator(comment_item_1, "activate", accel_group, GDK_KEY_slash, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    g_signal_connect(comment_item_1, "activate", G_CALLBACK(cb_on_toggle_comment), app);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), comment_item_1);

    /* Duplicate Line */
    GtkWidget *dup_item = gtk_menu_item_new_with_label("Duplicate Line");
    gtk_widget_add_accelerator(dup_item, "activate", accel_group, GDK_KEY_d, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    g_signal_connect(dup_item, "activate", G_CALLBACK(cb_on_duplicate_line), app);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), dup_item);

    /* Move Line Up */
    GtkWidget *move_up_item = gtk_menu_item_new_with_label("Move Line Up");
    gtk_widget_add_accelerator(move_up_item, "activate", accel_group, GDK_KEY_Up, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
    g_signal_connect(move_up_item, "activate", G_CALLBACK(cb_on_move_line_up), app);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), move_up_item);

    /* Move Line Down */
    GtkWidget *move_down_item = gtk_menu_item_new_with_label("Move Line Down");
    gtk_widget_add_accelerator(move_down_item, "activate", accel_group, GDK_KEY_Down, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
    g_signal_connect(move_down_item, "activate", G_CALLBACK(cb_on_move_line_down), app);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), move_down_item);

    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), gtk_separator_menu_item_new());

    /* Transform Case Submenu */
    GtkWidget *case_item = gtk_menu_item_new_with_label("Transform Case");
    GtkWidget *case_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(case_item), case_menu);

    GtkWidget *upper_item = gtk_menu_item_new_with_label("UPPERCASE");
    g_object_set_data(G_OBJECT(upper_item), "to_upper", GINT_TO_POINTER(TRUE));
    g_signal_connect(upper_item, "activate", G_CALLBACK(cb_on_transform_case), app);
    gtk_menu_shell_append(GTK_MENU_SHELL(case_menu), upper_item);

    GtkWidget *lower_item = gtk_menu_item_new_with_label("lowercase");
    g_object_set_data(G_OBJECT(lower_item), "to_upper", GINT_TO_POINTER(FALSE));
    g_signal_connect(lower_item, "activate", G_CALLBACK(cb_on_transform_case), app);
    gtk_menu_shell_append(GTK_MENU_SHELL(case_menu), lower_item);

    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), case_item);

    /* Line Settings Submenu */
    GtkWidget *settings_item = gtk_menu_item_new_with_label("Line Settings");
    GtkWidget *settings_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(settings_item), settings_menu);

    GtkWidget *line_nums_item = gtk_check_menu_item_new_with_label("Toggle Line Numbers");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(line_nums_item), TRUE);
    g_signal_connect(line_nums_item, "toggled", G_CALLBACK(cb_on_toggle_line_numbers), app);
    gtk_menu_shell_append(GTK_MENU_SHELL(settings_menu), line_nums_item);

    GtkWidget *word_wrap_item = gtk_check_menu_item_new_with_label("Toggle Word Wrap");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(word_wrap_item), FALSE);
    g_signal_connect(word_wrap_item, "toggled", G_CALLBACK(cb_on_toggle_word_wrap), app);
    gtk_menu_shell_append(GTK_MENU_SHELL(settings_menu), word_wrap_item);

    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), settings_item);     

    /* Append to Edit Menu */
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), html_menu_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), font_size_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), font_type_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), edit_sep);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), find_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), goto_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), cookie_mgr_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), color_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), calc_item);
    /* --- Search Menu --- */
    search_menu_item = gtk_menu_item_new_with_mnemonic("_Search G-dorking");
    search_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(search_menu_item), search_menu);

    /* Duckduckgo's search engine  */  
    GtkWidget *engines_sub_item = gtk_menu_item_new_with_label("Search Engine (DuckDuckGo) ");
    GtkWidget *engines_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(engines_sub_item), engines_menu);
    ddg_item = gtk_menu_item_new_with_label("DuckDuckGo");
    gtk_menu_shell_append(GTK_MENU_SHELL(engines_menu), ddg_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(search_menu), engines_sub_item);
    
    /* Advanced search dialog idea */
    GtkWidget *adv_search_item = gtk_menu_item_new_with_label("Advanced Search...");
    gtk_menu_shell_append(GTK_MENU_SHELL(search_menu), adv_search_item);
    
    /* menu Separator bar */
    GtkWidget *separator = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(search_menu), separator);
   

    /* List of predefined searches */ 
    const char *topics[] = { 
		"inurl:home.htm intitle:1766", 
		"inurl:cgi-bin/koha", 
		"site:edu filetype:xls 'root' database", 
		"intitle:index of /etc/ssh", 
		"intitle:Index of '/etc/network' | '/etc/cni/net.d'",
		"intext:'user' filetype:php intext:'account' inurl:/admin",
		"intitle:'Index of' intext:'php' site:*.com.*", 
		"intitle:'phpinfo' site:*.com.* intext:'HTTP_HOST'",
		"intitle:'Index of' intext:'config' site:*.com.*", 
		"site:.edu intext'robotics' inurl:/research",
		"intitle:'index of' '*robots.txt' site:.edu", 
		"inurl:robots filetype:txt", 
		"intitle:index.of /email /robots.txt", 
		"inurl: /wp-content/uploads/ inurl:'robots.txt' 'Disallow:' filetype:txt",
		"inurl:ftp://ftp robots.txt"
		};
    for (int i = 0; i < 15; i++) {
        GtkWidget *item = gtk_menu_item_new_with_label(topics[i]);
        
        /* Attach the specific search string to the widget object */
        g_object_set_data(G_OBJECT(item), "search_query", (gpointer)topics[i]);
        
        g_signal_connect(item, "activate", G_CALLBACK(cb_on_search_dor), app);
        gtk_menu_shell_append(GTK_MENU_SHELL(search_menu), item);
    }
    
    
    /* Audio Streaming */
    GtkWidget *media_item, *media_menu, *stream_item;

    media_item = gtk_menu_item_new_with_mnemonic("_Media Stream");
    media_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(media_item), media_menu);

    stream_item = gtk_menu_item_new_with_label("Open Stream...");
    gtk_menu_shell_append(GTK_MENU_SHELL(media_menu), stream_item);
    GtkWidget *dubstep_item = gtk_menu_item_new_with_label("Play Dub Step Beyond (Advanced Player)");
    gtk_menu_shell_append(GTK_MENU_SHELL(media_menu), dubstep_item); 
   
    GtkWidget *audio_item = gtk_menu_item_new_with_label("Play Dub Step Beyond (Basic player)");
    g_signal_connect(audio_item, "activate", G_CALLBACK(cb_on_open_basic_audio_player), app);
    gtk_menu_shell_append(GTK_MENU_SHELL(media_menu), audio_item);
 
 
    /* Other Search engines */ 
    const char *engine_names[] = {
	"Boardreader.com Forum Search (content aggeregator public - Zero trust Censored)",	
	"Core.ac.uk (Academic Aggregator Censored by CIA, UK interpol ncog groups)",	
	"Wiby.me (Independent Crawler Classic HTML, hobbyist sites, minimalist blogs)",
	"Marginalia (Independent Crawler non-commercial, text-heavy small web blogs)", 	
	"duckduck.go Search (Gateway to the darkweb/badlands)", 
	"Google Search (Zero trust censored by google inc)",	
    "Brave Search (Fully independent index Zero trust Cencered)", 
    "Bing Search (Sencored by microsoft)", 
    "Yahoo Search (Legacy index - Censorship by foreign governments, editorial gatekeeping, and content moderation.)", 
    "Ecosia Search (Privacy-friendly and plants trees)", 
    "Startpage Search (Googles resutls without tracking Cencered obfuscation only)",  
    "Mojeek Search (independent UK-based crawler Cencered by OSA-2023 / Ofcom)",
    "Swisscows Search (Semantic search; strictly fanily-frendly)",
    "Qwant Search (European-based; strict GDPR compliance)",
    "MetaGer Search (German metasearchl focus on transparency)", 
    "SearXNG Search (Open-source metasearch)", 
    "Gibiru Search (focuses on uncensored and unfiltered results)", 
    "Phind Search (AI search engine optimized specifically for developers)", 
    "Perplexity Search (Concersational AI search with direct citations)",
    "You.com Search (Customizable search with built-in AI apps)", 
    "Exhale Search (Minimalist, high-speed privacy search)",
    "Ahmia Search (Clear-web portal for security and hidden service research)",
    "Baidu (China zero trust censored by Russia, USA)",
    "Yandex (Russia zero trust censored by USA networks)",
    "Naver (South Korea zero trust censored blacklisted)", 
    "Seznam (Czech Republic blacklisted)", 
    "WolframAlpha (Academic / Computational)",
    "Google Scholar (Zero trust censored)",
    "BASE search Protected by Anubis From Techaro",
    "RefSeek  (students and researchers censored by FBI/NSA) "
    };
    const char *engine_urls[] = {
    "https://boardreader.com/s/",
	"https://core.ac.uk/search/?q=",	
	"https://wiby.me/?q=", 	
	"https://marginalia-search.com/search?query=",
	"https://duckduckgo.com/?q=",	 
	"https://www.google.com/search?q=",	
    "https://search.brave.com/search?q=", 
    "https://www.bing.com/search?q=", 
    "https://search.yahoo.com/search?p=", 
    "https://www.ecosia.org/search?q=",
    "https://www.startpage.com/sp/search?q=", 
    "https://www.mojeek.com/search?q=", 
    "https://swisscows.com/web?query=", 
    "https://www.qwant.com/?q=",
    "https://metager.org/meta/meta.ger3?eingabe=", 
    "https://searx.be/search?q=", 
    "https://gibiru.com/results.html?q=", 
    "https://www.phind.com/search?q=", 
    "https://preplexity.ai/search?q=", 
    "https://you.com/search?q=", 
    "https://exhale.it/search?q=",
    "https://ahmia.fi/search/?q=",
    "https://www.baidu.com/s?ie=utf-8&f=8&rsv_bp=1&rsv_idx=1&tn=baidu&wd=",
    "https://yandex.com/?text=", 
    "https://search.naver.com/search.naver?where=nexearch&sm=top_hty&fbm=0&ie=utf8&query=",
    "https://www.seznam.cz/?q=",
    "https://www.wolframalpha.com/input?i=",
    "https://scholar.google.com/scholar?hl=en&as_sdt=0%2C27&q=", 
    "https://www.base-search.net/Search/Results?lookfor=",
    "https://www.refseek.com/search?q="
    };
    GtkWidget *engines_item = gtk_menu_item_new_with_mnemonic("_Search engines");
    GtkWidget *engines_menu_sub = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(engines_item), engines_menu_sub);

    for (int i = 0; i < G_N_ELEMENTS(engine_names); i++) {
        GtkWidget *item = gtk_menu_item_new_with_label(engine_names[i]);
        
        /* Store the base URL in the widget object */
        g_object_set_data(G_OBJECT(item), "engine_url", (gpointer)engine_urls[i]);
        
        g_signal_connect(item, "activate", G_CALLBACK(cb_execute_engine_search), app);
        gtk_menu_shell_append(GTK_MENU_SHELL(engines_menu_sub), item);
    }
   
   /* HELP MENU */
    GtkWidget *help_item = gtk_menu_item_new_with_mnemonic("_Help");
    GtkWidget *help_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(help_item), help_menu);

    GtkWidget *view_help = gtk_menu_item_new_with_label("Manual / Shortcuts");
    
    gtk_menu_shell_append(GTK_MENU_SHELL(help_menu), view_help);

    /* Add FAQ Menu Item */
    GtkWidget *faq_item = gtk_menu_item_new_with_label("FAQ & Documentation");
    gtk_menu_shell_append(GTK_MENU_SHELL(help_menu), faq_item);
          
    /* Create the toggle directly as a top-level menu item */
    GtkWidget *js_toggle_mi = gtk_check_menu_item_new_with_label("Java Script");
    /* Set initial state to match your default (JS OFF) */ 
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(js_toggle_mi), FALSE);
    gtk_widget_add_accelerator(js_toggle_mi, "activate", accel_group, GDK_KEY_1, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    
    /* Privacy menu  */ 
    GtkWidget *privacy_item = gtk_menu_item_new_with_mnemonic("_Privacy");
    GtkWidget *privacy_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(privacy_item), privacy_menu);
    GtkWidget *js_toggle = gtk_check_menu_item_new_with_label("Enable JavaScript");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(js_toggle), TRUE); // Default ON
    GtkWidget *dnt_toggle = gtk_check_menu_item_new_with_label("Enable Do Not Track");                     
    gtk_menu_shell_append(GTK_MENU_SHELL(privacy_menu), js_toggle);
    gtk_menu_shell_append(GTK_MENU_SHELL(privacy_menu), dnt_toggle);
    GtkWidget *hardened_toggle = gtk_check_menu_item_new_with_label("Enable Hardened Privacy");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(hardened_toggle), FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(privacy_menu), hardened_toggle);
    /* Add Clear Data option */
    GtkWidget *clear_data_item = gtk_menu_item_new_with_label("Clear Browsing Cache & Storage");
    g_signal_connect(clear_data_item, "activate", G_CALLBACK(cb_on_clear_browsing_data), app);
    gtk_menu_shell_append(GTK_MENU_SHELL(privacy_menu), clear_data_item);

    /* Block hostile Script Protection */ 
    GtkWidget *menu_item_defusal = gtk_check_menu_item_new_with_label("Block Hostile Scripts (Script Defusal)");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item_defusal), TRUE);
    g_signal_connect(G_OBJECT(menu_item_defusal), "toggled", G_CALLBACK(on_toggle_script_defusal), app);
    gtk_menu_shell_append(GTK_MENU_SHELL(privacy_menu), menu_item_defusal);
    
    /* Create 'Spoof User-Agent' Parent Submenu Item */
    GtkWidget *ua_menu_item = gtk_menu_item_new_with_label("Spoof User-Agent");
    GtkWidget *ua_submenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(ua_menu_item), ua_submenu);
    GSList *ua_group = NULL;
    /* Helper array to construct options cleanly */
    struct {
    const char *label;
    const char *ua_val;
    } ua_options[] = {
    {"Default (Crystalcat)", UA_DEFAULT},
    {"Firefox (Linux)",      UA_FIREFOX},
    {"Chrome (Linux)",       UA_CHROME},
    {"Edge (Linux)",         UA_EDGE},
    {"Safari (iOS Mobile)",  UA_MOBILE},
	{"Windows NT (10 /11)", UA_WINDOWS},
	{"MacOS (Macintosh)", UA_MACOS},
	{"Android (Chrome Mobile)", UA_ANDROID}
    };
	
    for (size_t i = 0; i < sizeof(ua_options) / sizeof(ua_options[0]); i++) {
    GtkWidget *mi = gtk_radio_menu_item_new_with_label(ua_group, ua_options[i].label);
    ua_group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(mi));

    /* Attach UA string pointer to menu item object */
    g_object_set_data(G_OBJECT(mi), "ua-string", (gpointer)ua_options[i].ua_val);

    g_signal_connect(mi, "toggled", G_CALLBACK(cb_on_ua_selected), app);
    gtk_menu_shell_append(GTK_MENU_SHELL(ua_submenu), mi);
   }
   /* Append submenu into Privacy Menu */
   gtk_menu_shell_append(GTK_MENU_SHELL(privacy_menu), ua_menu_item);
   
   GtkWidget *reject_cookies_item = gtk_menu_item_new_with_label("Reject All Cookies");
   g_signal_connect(G_OBJECT(reject_cookies_item), "activate",
                 G_CALLBACK(on_reject_all_cookies_activate), app);
   gtk_menu_shell_append(GTK_MENU_SHELL(privacy_menu), reject_cookies_item);

   GtkWidget *alter_cookie_menu_item = gtk_menu_item_new_with_label("Alter Cookie...");
   g_signal_connect(G_OBJECT(alter_cookie_menu_item), "activate", 
                 G_CALLBACK(on_menu_alter_cookie_activate), app);
   gtk_menu_shell_append(GTK_MENU_SHELL(privacy_menu), alter_cookie_menu_item);

   GtkWidget *spoof_menu_item = ui_create_cookie_spoof_menu_item(app);
   gtk_menu_shell_append(GTK_MENU_SHELL(privacy_menu), spoof_menu_item);

    /* Add "Randomize Geolocation" Item */
    GtkWidget *geo_item = gtk_menu_item_new_with_label("Randomize Geolocation");   
    /* "Randomize Geolocation" Connect click event */
    g_signal_connect(geo_item, "activate", 
                     G_CALLBACK(on_randomize_location_activate), 
                     app);

    gtk_menu_shell_append(GTK_MENU_SHELL(privacy_menu), geo_item);


    /* Cromebook / cromeOS system configureation options at local level. */
    const char *flag_names[] = { 
        "Main Flags (chrome://flags)", 
        "GPU Internals (chrome://gpu)", 
        "Network Exports (chrome://net-export)", 
        "Device Log (chrome://device-log)", 
        "Version Information (chrome://version)"
    };

    const char *flag_urls[] = { 
        "chrome://flags", 
        "chrome://gpu", 
        "chrome://net-export", 
        "chrome://device-log", 
        "chrome://version"
    };
    
    /* Create the top-level menu item with a unique mnemonic (_C) */
    GtkWidget *flags_root_item = gtk_menu_item_new_with_mnemonic("_ChromeOs");
    GtkWidget *flags_sub_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(flags_root_item), flags_sub_menu);

    /* Loop through and populate the sub-menu */
    for (int i = 0; i < G_N_ELEMENTS(flag_names); i++) {
        GtkWidget *item = gtk_menu_item_new_with_label(flag_names[i]);
        
        /* Attach the specific URL to this specific menu item widget */
        g_object_set_data(G_OBJECT(item), "system_uri", (gpointer)flag_urls[i]);
        
        /* Connect to our new callback */
        g_signal_connect(item, "activate", G_CALLBACK(cb_execute_flag_direct), app);
        
        gtk_menu_shell_append(GTK_MENU_SHELL(flags_sub_menu), item);
    }
    
    /* Create the Menu Bar and Items */
    GtkWidget *filters_menu_item = gtk_menu_item_new_with_label("Filters Adblock");
	GtkWidget *filters_menu = gtk_menu_new();
	GtkWidget *add_pattern_item = gtk_menu_item_new_with_label("Add New Pattern");
    
    /* Create menu entry to open the dialog */
    GtkWidget *dialog_item = gtk_menu_item_new_with_label("Article & Image Isolation Settings...");
    g_signal_connect(dialog_item, "activate", 
                         G_CALLBACK(ui_show_article_isolation_dialog), app);
                         
    /* Or add the direct checkbox toggle item directly to the menu */
   GtkWidget *toggle_item = ui_create_article_isolation_menu_item(app);
   gtk_menu_shell_append(GTK_MENU_SHELL(filters_menu), dialog_item); 
   gtk_menu_shell_append(GTK_MENU_SHELL(filters_menu), toggle_item);
       
    /* --- Primary Tools Menu --- */
    GtkWidget *tools_menu = gtk_menu_new();
    GtkWidget *tools_item = gtk_menu_item_new_with_label("Tools");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(tools_item), tools_menu);
    
    /* Directory Operations */
    GtkWidget *mkdir_item  = gtk_menu_item_new_with_label("Create Directory...");
    g_signal_connect(mkdir_item, "activate", G_CALLBACK(cb_on_create_directory), app);
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), mkdir_item);
    
    GtkWidget *sep_file = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), sep_file);

    /* --- Panel & View Toggles --- */
    GtkWidget *toggle_time_item = gtk_check_menu_item_new_with_label("Show Menu Time");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(toggle_time_item), TRUE);
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), toggle_time_item);
    g_signal_connect(G_OBJECT(toggle_time_item), "toggled", G_CALLBACK(cb_on_toggle_menu_time), app);
     
    GtkWidget *toggle_sidebar_item = gtk_check_menu_item_new_with_label("Show Sidebar (Recent & Functions)");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(toggle_sidebar_item), TRUE);
    g_signal_connect(G_OBJECT(toggle_sidebar_item), "toggled", G_CALLBACK(cb_on_toggle_sidebar_panel), app);
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), toggle_sidebar_item);

    GtkWidget *toggle_console_item = gtk_check_menu_item_new_with_label("Show Bash Shell Console");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(toggle_console_item), TRUE);
    g_signal_connect(G_OBJECT(toggle_console_item), "toggled", 
                     G_CALLBACK(cb_on_toggle_console_panel), app);
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), toggle_console_item);

    GtkWidget *toggle_editor_item = gtk_check_menu_item_new_with_label("Focus Browser Mode (Hide/show Editor)");
    gtk_menu_item_set_accel_path(GTK_MENU_ITEM(toggle_editor_item), "<App>/View/ToggleEditor");
    gtk_accel_map_add_entry("<App>/View/ToggleEditor", GDK_KEY_6, GDK_CONTROL_MASK | GDK_SHIFT_MASK);
    gtk_widget_add_accelerator(toggle_editor_item, "activate", accel_group, GDK_KEY_asciicircum, GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE);                                
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(toggle_editor_item), FALSE);
    g_signal_connect(G_OBJECT(toggle_editor_item), "toggled", G_CALLBACK(cb_on_toggle_editor_focus), app);

    /* Add Audio Program Mode check item under View menu */
    GtkWidget *audio_mode_item = gtk_check_menu_item_new_with_label("Audio Streaming Mode");
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), audio_mode_item);
    g_signal_connect(G_OBJECT(audio_mode_item), "toggled", 
                 G_CALLBACK(cb_on_toggle_audio_mode), app);
                 gtk_widget_show(audio_mode_item);

     /* Append to the Tools sub-menu */
     gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), toggle_editor_item);
     gtk_widget_show(toggle_editor_item);
     
     /* Create the Toggle Compile Button check menu item */
     GtkWidget *toggle_compile_item = gtk_check_menu_item_new_with_label("Show Compile Button");
     gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(toggle_compile_item), TRUE);
     g_signal_connect(G_OBJECT(toggle_compile_item), "toggled",
                 G_CALLBACK(cb_on_toggle_compile_button), app);
     gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), toggle_compile_item);
     gtk_widget_show(toggle_compile_item);
     
     GtkWidget *sep_toggles = gtk_separator_menu_item_new();
     gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), sep_toggles);
     
    /* --- Utilities & Add-ons --- */
    GtkWidget *calculator_item = gtk_menu_item_new_with_label("Calculator");
    g_signal_connect(G_OBJECT(calculator_item), "activate", G_CALLBACK(on_calculator_menu_activate), app);
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), calculator_item);

    GtkWidget *filters_item = gtk_menu_item_new_with_label("Manage Web Filters...");
    g_signal_connect(G_OBJECT(filters_item), "activate", G_CALLBACK(on_add_filter_menu_activated), app);
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), filters_item);
    
    GtkWidget *manage_cookies_item = gtk_menu_item_new_with_label("Manage Cookies...");
    g_signal_connect_swapped(manage_cookies_item, "activate", G_CALLBACK(ui_show_cookie_dialog), app);
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), manage_cookies_item);

    GtkWidget *webkit_console_item = gtk_menu_item_new_with_label("WebKit JavaScript Console Logs...");
    g_signal_connect(webkit_console_item, "activate", G_CALLBACK(ui_show_webkit_console_dialog), app);
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), webkit_console_item);

    /* Create the Article Search menu entry */
    GtkWidget *article_search_item = gtk_menu_item_new_with_label("Advanced Article Search");
    gtk_container_add(GTK_CONTAINER(tools_menu), article_search_item);
    g_signal_connect(article_search_item, "activate", G_CALLBACK(on_tools_article_search_activated), app);

    /* Handy Helpful Research Links */
    const char *research_names[] = {
        "Archive.org (Wayback Machine - Web History)",
        "Archive.today (Webpage Snapshot / Paywall Bypass)",
        "Wikipedia (Global Crowdsourced Encyclopedia)",
        "Wikidata (Structured Knowledge Database)",
        "Wikisource (Free Content Library of Source Texts)",
        "Wikimedia Commons (Free Media Repository)",
        "Internet Archive Books (Digital Public Library)",
        "Open Library (Borrowable Public eBook Catalog)",
        "Project Gutenberg (Public Domain eBooks)",
        "LibGen / Library Genesis (Scientific Papers & Books)",
        "Sci-Hub (Academic Research Article Unlocker)",
        "PubMed (Biomedical & Life Sciences Literature)",
        "Europe PMC (Open Access Life Sciences Articles)",
        "SSRN (Social Science Research Network)",
        "arXiv (Physics, Math, Computer Science Preprints)",
        "bioRxiv (Biology Research Preprints)",
        "medRxiv (Health Sciences Research Preprints)",
        "OSF Preprints (Open Science Framework)",
        "Directory of Open Access Journals (DOAJ)",
        "WorldCat (Global Library Catalog)",
        "Stanford Encyclopedia of Philosophy (SEP)",
        "Internet Encyclopedia of Philosophy (IEP)",
        "DART-Europe (European Research Theses Portal)",
        "Networked Digital Library of Theses and Dissertations",
        "PhilPapers (Philosophy Research Aggregator)",
        "SSRN Papers (Social Science Research Papers)",
        "Data.gov (US Government Open Data)",
        "European Union Open Data Portal",
        "UN Data (United Nations Statistics)",
        "World Bank Open Data",
        "4chan.org (Home of ANONYMOUS)"  
    };

    const char *research_urls[] = {
        "https://web.archive.org/web/*/",
        "https://archive.is/",
        "https://en.wikipedia.org/w/index.php?search=",
        "https://www.wikidata.org/w/index.php?search=",
        "https://en.wikisource.org/w/index.php?search=",
        "https://commons.wikimedia.org/w/index.php?search=",
        "https://archive.org/details/books?tab=collection&query=",
        "https://openlibrary.org/search?q=",
        "https://www.gutenberg.org/ebooks/search/?query=",
        "https://libgen.is/search.php?req=",
        "https://sci-hub.se/",
        "https://pubmed.ncbi.nlm.nih.gov/?term=",
        "https://europepmc.org/search?query=",
        "https://papers.ssrn.com/sol3/results.cfm?txtKeyWords=",
        "https://arxiv.org/search/?query=",
        "https://www.biorxiv.org/search/",
        "https://www.medrxiv.org/search/",
        "https://osf.io/preprints/discover?q=",
        "https://doaj.org/search/articles?source=%7B%22query%22%3A%7B%22query_string%22%3A%7B%22query%22%3A%22",
        "https://www.worldcat.org/search?q=",
        "https://plato.stanford.edu/search/searcher.py?query=",
        "https://iep.utm.edu/?s=",
        "https://www.dart-europe.org/basic-search.php?q=",
        "https://ndltd.org/search/?q=",
        "https://philpapers.org/s/",
        "https://papers.ssrn.com/sol3/results.cfm?txtKeyWords=",
        "https://catalog.data.gov/dataset?q=",
        "https://data.europa.eu/data/datasets?query=",
        "https://data.un.org/Search.aspx?q=",
        "https://data.worldbank.org/indicator?tab=all&q=",
        "https://www.4chan.org/"
    };

    GtkWidget *research_item = gtk_menu_item_new_with_mnemonic("_Research");
    GtkWidget *research_menu_sub = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(research_item), research_menu_sub);

    for (int i = 0; i < G_N_ELEMENTS(research_names); i++) {
        GtkWidget *item = gtk_menu_item_new_with_label(research_names[i]);
        
        /* Store the base URL in the widget object */
        g_object_set_data(G_OBJECT(item), "engine_url", (gpointer)research_urls[i]);
        
        g_signal_connect(item, "activate", G_CALLBACK(cb_execute_engine_search), app);
        gtk_menu_shell_append(GTK_MENU_SHELL(research_menu_sub), item);
    }
    

 
    /* --- Signals --- */
    g_signal_connect(quick_save_item, "activate", G_CALLBACK(cb_on_quick_save), app);
    g_signal_connect(adv_search_item, "activate", G_CALLBACK(ui_show_advanced_search_dialog), app);
    g_signal_connect(add_pattern_item, "activate", G_CALLBACK(on_add_filter_menu_activated), app);
    g_signal_connect(color_item, "activate", G_CALLBACK(cb_on_color_picker), app);
    g_signal_connect(hardened_toggle, "activate", G_CALLBACK(on_toggle_hardened_mode), app);
    g_signal_connect(js_toggle, "activate", G_CALLBACK(on_toggle_js), app);
    g_signal_connect(dnt_toggle, "activate", G_CALLBACK(on_toggle_do_not_track), app);
    g_signal_connect_swapped(cookie_mgr_item, "activate", G_CALLBACK(ui_show_cookie_dialog), app);
    g_signal_connect(js_toggle_mi, "toggled", G_CALLBACK(on_toggle_js), app);
    g_signal_connect(view_help, "activate", G_CALLBACK(cb_on_help_page), app);
    g_signal_connect_swapped(find_item, "activate", G_CALLBACK(ui_show_find_dialog), app);
    g_signal_connect_swapped(goto_item, "activate", G_CALLBACK(ui_show_goto_dialog), app);
    g_signal_connect(ddg_item, "activate", G_CALLBACK(cb_execute_ddg_search), app);
    g_signal_connect(dubstep_item, "activate", G_CALLBACK(cb_play_dubstep_beyond), app);
    g_signal_connect(stream_item, "activate", G_CALLBACK(cb_on_stream_request), app);
    g_signal_connect(outline_item, "activate", G_CALLBACK(cb_on_generate_outline), app);
    g_signal_connect(open_item, "activate", G_CALLBACK(cb_on_open), app);
    g_signal_connect(save_item, "activate", G_CALLBACK(cb_on_save), app);
    g_signal_connect(save_as_item, "activate", G_CALLBACK(cb_on_save_as), app);
    g_signal_connect(quit_item, "activate", G_CALLBACK(cb_on_destroy), app);
    g_signal_connect(snippet_item, "activate", G_CALLBACK(cb_on_snippet_insert), app);
    g_signal_connect(G_OBJECT(faq_item), "activate", G_CALLBACK(on_menu_help_faq_activate), app->window);
                 
    gtk_menu_shell_append(GTK_MENU_SHELL(bar), file_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(bar), edit_item);  
    gtk_menu_shell_append(GTK_MENU_SHELL(bar), tools_item);    
    gtk_menu_shell_append(GTK_MENU_SHELL(bar), search_menu_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(bar), media_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(bar), engines_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(bar), research_item);  
    gtk_menu_shell_append(GTK_MENU_SHELL(bar), js_toggle_mi); 
    gtk_menu_shell_append(GTK_MENU_SHELL(bar), privacy_item);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(filters_menu_item), filters_menu);
	gtk_menu_shell_append(GTK_MENU_SHELL(filters_menu), add_pattern_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(bar), filters_menu_item);    
    gtk_menu_shell_append(GTK_MENU_SHELL(bar), flags_root_item);    
    gtk_menu_shell_append(GTK_MENU_SHELL(bar), help_item);  
    
    GtkWidget *time_item = gtk_menu_item_new();
    app->menu_time_label = gtk_label_new("00:00:00");
    gtk_container_add(GTK_CONTAINER(time_item), app->menu_time_label);
    gtk_widget_set_sensitive(time_item, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(bar), time_item);
    g_timeout_add(1000, cb_update_menu_bar_time, app);

    return bar;
}

static GtkWidget* ui_create_search_section(AppState *app) {
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *btn = gtk_button_new_with_label("Browser Search");

    app->search_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app->search_entry), "Search C documentation...");
     
    gtk_box_pack_start(GTK_BOX(hbox), app->search_entry, TRUE, TRUE, 10);
    gtk_box_pack_start(GTK_BOX(hbox), btn, FALSE, FALSE, 10);
    /* Trigger search when user is clicking button */
    g_signal_connect(btn, "clicked", G_CALLBACK(cb_on_search), app);
    /* Trigger lazy reflex search when pressing Enter in the entry box */
    g_signal_connect(app->search_entry, "activate", G_CALLBACK(cb_on_search), app);
    
    return hbox;
}

static GtkWidget* ui_create_workspace(AppState *app) {
    GtkWidget *h_paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    GtkWidget *v_paned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    GtkWidget *scroll_tree = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *scroll_recent = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *scroll_editor = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *tree_view;

    /* --- Initialize Buffer First --- */
    GtkSourceBuffer *source_buf = gtk_source_buffer_new(NULL);
    app->editor_buffer = GTK_TEXT_BUFFER(source_buf);

    /* Configure buffer settings */
    gtk_source_buffer_set_highlight_syntax(source_buf, TRUE);
    gtk_source_buffer_set_max_undo_levels(source_buf, 100);

    /* --- Initialize View using that Buffer --- */
    app->editor_view = gtk_source_view_new_with_buffer(source_buf);

    /* --- Apply Style Scheme & Language --- */
    GtkSourceStyleSchemeManager *scheme_manager = gtk_source_style_scheme_manager_get_default();
    GtkSourceStyleScheme *scheme = gtk_source_style_scheme_manager_get_scheme(scheme_manager, "oblivion");
    gtk_source_buffer_set_style_scheme(source_buf, scheme);

    GtkSourceLanguageManager *lang_manager = gtk_source_language_manager_get_default();
    GtkSourceLanguage *lang = gtk_source_language_manager_get_language(lang_manager, "c");
    gtk_source_buffer_set_language(source_buf, lang);

    /* --- Configure View Settings --- */
    gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(app->editor_view), TRUE);
    gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(app->editor_view), TRUE);

    /* --- Rest of UI Assembly --- */
    gtk_container_add(GTK_CONTAINER(scroll_editor), app->editor_view);

    /* =========================================================
     * SIDEBAR NOTEBOOK CREATION
     * ========================================================= */
    app->sidebar_notebook = gtk_notebook_new();
    gtk_widget_set_size_request(app->sidebar_notebook, 200, 40);

    /* --- Sidebar Tab 1: Functions Tree --- */
    app->tree_store = gtk_tree_store_new(1, G_TYPE_STRING);
    tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(app->tree_store));
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view),
        gtk_tree_view_column_new_with_attributes("Jump to Functions:", gtk_cell_renderer_text_new(), "text", 0, NULL));
    gtk_container_add(GTK_CONTAINER(scroll_tree), tree_view);
    g_signal_connect(tree_view, "row-activated", G_CALLBACK(cb_on_tree_row_activated), app);

    gtk_notebook_append_page(GTK_NOTEBOOK(app->sidebar_notebook), scroll_tree, gtk_label_new("Program functions"));

    /* --- Sidebar Tab 2: Recent Files Tree --- */
    app->recent_tree_store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
    app->recent_tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(app->recent_tree_store));

    GtkCellRenderer *renderer_recent = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *col_recent = gtk_tree_view_column_new_with_attributes("Reopen recent files:", renderer_recent, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(app->recent_tree_view), col_recent);

    g_signal_connect(app->recent_tree_view, "row-activated", G_CALLBACK(cb_on_recent_row_activated), app);

    gtk_container_add(GTK_CONTAINER(scroll_recent), app->recent_tree_view);
    gtk_notebook_append_page(GTK_NOTEBOOK(app->sidebar_notebook), scroll_recent, gtk_label_new("Recent"));

    /* Populate recent files list on creation */
    logic_refresh_recent_files_tree(app);

    /* --- Browser View Setup --- */
    app->browser_view = webkit_web_view_new();
   
    /* Lock that shit down & Catch webcam/mic access attempts */
    g_signal_connect(app->browser_view, "permission-request", G_CALLBACK(on_permission_request), app);
    /* Connect signal to update the assistant message when the URI changes */
    g_signal_connect(app->browser_view, "notify::uri", G_CALLBACK(on_browser_uri_notify), app);
   
    WebKitUserContentManager *console_manager =
    webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(app->browser_view));
    if (console_manager) {
    if (webkit_user_content_manager_register_script_message_handler(
            console_manager, "crystalcatConsole")) {

        g_signal_connect(
            console_manager,
            "script-message-received::crystalcatConsole",
            G_CALLBACK(cb_webkit_console_message),
            app);

        WebKitUserScript *console_script =
            webkit_user_script_new(
                webkit_console_capture_script,
                WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
                WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
                NULL, NULL);

        webkit_user_content_manager_add_script(
            console_manager, console_script);

        webkit_user_script_unref(console_script);
     }
    }
    apply_ad_filters_2(WEBKIT_WEB_VIEW(app->browser_view), app->db);
    apply_css_element_blocking_2(WEBKIT_WEB_VIEW(app->browser_view), app->db);  
    webkit_web_view_load_html(WEBKIT_WEB_VIEW(app->browser_view),
    "<html><head>"
    "<title>CrystalCat Editor</title>"
    "</head>"
    "<body style='background:#121212; color:#00ff00;font-family:sans-serif; padding:20px;'>"
    "<h1>Crystalcat MDE</h1><br/><hr/>"
    "<p style='color:#11ff00'>A high-performance modular development environment for ANSI C.</p>"
    "</body></html>", NULL);

    WebKitSettings *settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(app->browser_view));
    webkit_settings_set_enable_javascript_markup(settings, FALSE);
    webkit_settings_set_enable_mediasource(settings, TRUE);
    webkit_settings_set_allow_universal_access_from_file_urls(settings, TRUE);
    webkit_settings_set_allow_file_access_from_file_urls(settings, TRUE);

    g_object_set(G_OBJECT(settings), "enable-developer-extras", TRUE, NULL);

    #if WEBKIT_CHECK_VERSION(2, 30, 0)
    #else
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    webkit_settings_set_enable_hyperlink_auditing(settings, FALSE);
    #pragma GCC diagnostic pop
    #endif

    /* --- Final Layout Assembly --- */
    /* Top pane: Editor directly */
    gtk_paned_add1(GTK_PANED(v_paned), scroll_editor);

    /* Bottom pane: Web Browser View */
    gtk_widget_set_size_request(app->browser_view, -1, 200);
    gtk_paned_add2(GTK_PANED(v_paned), app->browser_view);
    gtk_paned_set_position(GTK_PANED(v_paned), 450);

    /* Left pane: Sidebar Notebook */
    gtk_paned_add1(GTK_PANED(h_paned), app->sidebar_notebook);
    gtk_paned_add2(GTK_PANED(h_paned), v_paned);
    gtk_paned_set_position(GTK_PANED(h_paned), 220);

    return h_paned;
}

static GtkWidget* ui_create_console_panel(AppState *app) {
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    /* AppState this is already used above,
     *  GtkWidget *btn_box = gtk_button_box_new(GTK_ORIENTATION_VERTICAL); */ 
    GtkWidget *btn_comp = gtk_button_new_with_label("Compile");

    /* Create the view and buffer first */
    app->console_view = gtk_text_view_new();
    app->console_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->console_view));
    gtk_text_view_set_editable(GTK_TEXT_VIEW(app->console_view), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(app->console_view), TRUE);

    /* Set the height for the terminal area */
    gtk_widget_set_size_request(scroll, -1, 150);
    
    /* Add to containers ONCE */
    gtk_container_add(GTK_CONTAINER(scroll), app->console_view);
    gtk_box_pack_start(GTK_BOX(hbox), scroll, TRUE, TRUE, 0);
    
    app->btn_box = gtk_button_box_new(GTK_ORIENTATION_VERTICAL);
    gtk_container_add(GTK_CONTAINER(app->btn_box), btn_comp);
    gtk_box_pack_start(GTK_BOX(hbox), app->btn_box, FALSE, FALSE, 5);
    
    g_signal_connect(btn_comp, "clicked", G_CALLBACK(cb_on_compile), app);

    return hbox;
}

/* --- Callback & Logic Modules --- */
static void cb_execute_engine_search(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    const char *base_url = g_object_get_data(G_OBJECT(widget), "engine_url");
    const char *query = gtk_entry_get_text(GTK_ENTRY(app->search_entry));

   if (!query || strlen(query) == 0) {
        ui_set_status_alert(app, "Please enter keywords into the search box first.", TRUE);
        /* Convenience alternative: Load a default start page or return early */
        webkit_web_view_load_uri(WEBKIT_WEB_VIEW(app->browser_view), "about:blank");     
        return;
    }
    /* Clear the status alert since the user provided a valid search query */
    ui_set_status_alert(app, "", FALSE);
    if (query && strlen(query) > 0) {
        char *escaped_query = g_uri_escape_string(query, NULL, TRUE);
        char *full_url = g_strdup_printf("%s%s", base_url, escaped_query);

        webkit_web_view_load_uri(WEBKIT_WEB_VIEW(app->browser_view), full_url);
        
        g_free(escaped_query);
        g_free(full_url);
    }
}

/*Audio Streaming callback */
static void cb_on_stream_request(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    if (!app) {
        g_printerr("cb_on_stream_request: AppState is NULL!\n");
        return;
    }

    /* Build and Run the URL Input Dialog */
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Open Stream",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_OK", GTK_RESPONSE_ACCEPT,
        "_Cancel", GTK_RESPONSE_REJECT,
        NULL);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content_area), 15);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_pack_start(GTK_BOX(content_area), vbox, TRUE, TRUE, 0);

    /* Instructions label */
    GtkWidget *info_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(info_label), 
        "<b>Enter a direct audio stream URL:</b>\n"
        "<small>Do not use .m3u or .pls playlist URLs. Use the raw stream link instead.</small>");
    gtk_label_set_xalign(GTK_LABEL(info_label), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), info_label, FALSE, FALSE, 0);

    /* Text entry */
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "e.g., http://ice1.somafm.com/deepspaceone-128-mp3");
    gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);

    /* Examples label */
    GtkWidget *examples_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(examples_label),
        "<span foreground='#888888'><b>Common SomaFM Direct Streams:</b>\n"
        "• http://ice1.somafm.com/deepspaceone-128-mp3\n"
        "• http://ice1.somafm.com/groovesalad-256-mp3\n"
        "• http://ice1.somafm.com/dronezone-128-mp3\n"
        "• http://ice1.somafm.com/beatblender-128-mp3</span>");
    gtk_label_set_xalign(GTK_LABEL(examples_label), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), examples_label, FALSE, FALSE, 5);

    gtk_widget_show_all(dialog);

    /* Process Response */
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        const char *url = gtk_entry_get_text(GTK_ENTRY(entry));

        if (url && strlen(url) > 0) {
            
            /* Safely check and reuse or create the popup player window */
            if (!app->audio_popup || !GTK_IS_WIDGET(app->audio_popup)) {
                app->audio_popup = gtk_window_new(GTK_WINDOW_TOPLEVEL);
                gtk_window_set_title(GTK_WINDOW(app->audio_popup), "Audio Stream Player");
                gtk_window_set_default_size(GTK_WINDOW(app->audio_popup), 755, 250);
                
                /* Weak pointer automatically sets app->audio_popup to NULL when closed */ 
                g_object_add_weak_pointer(G_OBJECT(app->audio_popup), (gpointer*)&app->audio_popup);
                app->audio_webview = NULL; /* Reset child pointer reference */ 
            }

            if (!app->audio_webview || !WEBKIT_IS_WEB_VIEW(app->audio_webview)) {
                app->audio_webview = webkit_web_view_new();
                gtk_container_add(GTK_CONTAINER(app->audio_popup), app->audio_webview);

                /* Configure WebKit settings to allow autoplay */
                WebKitSettings *settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(app->audio_webview));
                webkit_settings_set_media_playback_requires_user_gesture(settings, FALSE);
                webkit_settings_set_enable_webaudio(settings, TRUE);

                /* Weak pointer automatically sets app->audio_webview to NULL when destroyed*/ 
                g_object_add_weak_pointer(G_OBJECT(app->audio_webview), (gpointer*)&app->audio_webview);
            }

            /* Prepare and load HTML content */
            char html_player[4096];
            snprintf(html_player, sizeof(html_player),
                     "<html>"
                     "<head>"
                     "  <style>"
                     "    body { background:#121212; color:#00ff00; font-family:monospace; margin:0; "
                     "           display:flex; flex-direction:column; justify-content:center; align-items:center; height:100vh; }"
                     "    .container { width: 85%%; display: flex; flex-direction: column; gap: 15px; }"
                     "    .header { font-size: 1.1em; color: #ffff0E; border-bottom: 1px dashed #333; padding-bottom: 8px; margin: 0; }"
                     "    .stats-panel { background: #1a1a1a; border: 1px solid #333; padding: 10px; font-size: 0.85em; "
                     "                   display: grid; grid-template-columns: 1fr 1fr; gap: 5px; color: #888888; }"
                     "    .stat-val { color: #00ff00; }"
                     "    audio { width: 100%%; }"
                     "  </style>"
                     "</head>"
                     "<body>"
                     "  <div class='container'>"
                     "    <h3 class='header'>Streaming: <span style='color:#00ff00;'>%s</span></h3>"
                     "    <div class='stats-panel'>"
                     "      <div>Status: <span id='stat-status' class='stat-val'>Connecting...</span></div>"
                     "      <div>Session Time: <span id='stat-time' class='stat-val'>00:00:00</span></div>"
                     "      <div>Stream Type: <span id='stat-type' class='stat-val'>Live Audio Feed</span></div>"
                     "      <div>Volume: <span id='stat-volume' class='stat-val'>100%%</span></div>"
                     "    </div>"
                     "    <audio id='player' src='%s' controls autoplay></audio>"
                     "  </div>"
                     "  <script>"
                     "    var audio = document.getElementById('player');"
                     "    var secondsElapsed = 0;"
                     "    audio.addEventListener('canplay', function() {"
                     "        audio.play().catch(e => console.log('Autoplay blocked: ', e));"
                     "    });"
                     "    function formatTime(totalSeconds) {"
                     "        var h = Math.floor(totalSeconds / 3600).toString().padStart(2, '0');"
                     "        var m = Math.floor((totalSeconds %% 3600) / 60).toString().padStart(2, '0');"
                     "        var s = Math.floor(totalSeconds %% 60).toString().padStart(2, '0');"
                     "        return h + ':' + m + ':' + s;"
                     "    }"
                     "    setInterval(function() {"
                     "        var status = 'Playing';"
                     "        if (audio.paused) {"
                     "            status = 'Paused';"
                     "        } else if (audio.seeking || audio.readyState < 3) {"
                     "            status = 'Buffering...';"
                     "        }"
                     "        document.getElementById('stat-status').innerText = status;"
                     "        var vol = Math.round(audio.volume * 100);"
                     "        if (audio.muted) vol = 0;"
                     "        document.getElementById('stat-volume').innerText = vol + '%%';"
                     "    }, 250);"
                     "    setInterval(function() {"
                     "        if (!audio.paused && audio.readyState >= 3 && !audio.seeking) {"
                     "            secondsElapsed++;"
                     "            document.getElementById('stat-time').innerText = formatTime(secondsElapsed);"
                     "        }"
                     "    }, 1000);"
                     "  </script>"
                     "</body>"
                     "</html>", url, url);

            webkit_web_view_load_html(WEBKIT_WEB_VIEW(app->audio_webview), html_player, NULL);
            gtk_widget_show_all(app->audio_popup);
            gtk_window_present(GTK_WINDOW(app->audio_popup));
        }
    }

    gtk_widget_destroy(dialog);
}
static void cb_play_dubstep_beyond(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    const char *dubstep_url = "https://ice1.somafm.com/dubstep-128-mp3";
    ui_open_audio_player(app, dubstep_url);
}

static void cb_on_open_basic_audio_player(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    /* Opens the basic audio player stream/popup */
    ui_open_basic_audio_player(app, "https://www.soundhelix.com/examples/mp3/SoundHelix-Song-1.mp3");
}

/* Open file callback */
static void cb_on_open(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Open File", GTK_WINDOW(app->window), GTK_FILE_CHOOSER_ACTION_OPEN, "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        
        /* Store the filename in AppState */
        if (app->current_filename) { 
        g_free(app->current_filename);
	    }
        app->current_filename = g_strdup(filename);
        
        char *content;
        gsize length;
        if (g_file_get_contents(filename, &content, &length, NULL)) {
            gtk_text_buffer_set_text(app->editor_buffer, content, length);
            logic_refresh_function_tree(app);
            g_free(content);
        }
        /* Record to SQLite database & update UI tree */
        logic_add_recent_file(app, filename);
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

static void cb_on_tree_row_activated(GtkTreeView *view, GtkTreePath *path, 
                                     GtkTreeViewColumn *col, gpointer data) {
    AppState *app = (AppState *)data;
    GtkTreeModel *model = gtk_tree_view_get_model(view);
    GtkTreeIter iter;
    gchar *func_name;
    GtkTextIter start, f_start, f_end;

    if (gtk_tree_model_get_iter(model, &iter, path)) {
        gtk_tree_model_get(model, &iter, 0, &func_name, -1);
        gtk_text_buffer_get_start_iter(app->editor_buffer, &start);

        /* Search for the function name */
        if (gtk_text_iter_forward_search(&start, func_name, 
            GTK_TEXT_SEARCH_VISIBLE_ONLY, &f_start, &f_end, NULL)) {
            
            /* Move cursor and scroll to line */
            gtk_text_buffer_place_cursor(app->editor_buffer, &f_start);
            gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(app->editor_view), 
                                         &f_start, 0.0, TRUE, 0.0, 0.5);
        }
        g_free(func_name);
    }
}

static void logic_refresh_function_tree(AppState *app) {
    GtkTextIter start, end;
    GtkTreeIter tree_iter;
    gchar *text, *line;
    gtk_tree_store_clear(app->tree_store);
    gtk_text_buffer_get_bounds(app->editor_buffer, &start, &end);
    text = gtk_text_buffer_get_text(app->editor_buffer, &start, &end, FALSE);
    line = strtok(text, "\n");
    while (line != NULL) {
        if (strstr(line, ")") && strstr(line, "{") && !strstr(line, ";")) {
            gtk_tree_store_append(app->tree_store, &tree_iter, NULL);
            gtk_tree_store_set(app->tree_store, &tree_iter, 0, line, -1);
        }
        line = strtok(NULL, "\n");
    }
    g_free(text);
}

/* --- Edit Menu / New Session Callback --- */
static void cb_on_new_session_save_prompt(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    if (!app || !app->editor_buffer) return;

    /*  Prompt user to save active document if modified */
    if (gtk_text_buffer_get_modified(app->editor_buffer)) {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(app->window),
            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_NONE,
            "Do you want to save current changes before starting a new editable session?"
        );

        gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                               "_Save", GTK_RESPONSE_YES,
                               "_Discard", GTK_RESPONSE_NO,
                               "_Cancel", GTK_RESPONSE_CANCEL,
                               NULL);

        gint response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        if (response == GTK_RESPONSE_YES) {
            cb_on_save(NULL, app);
            /* If the user cancels out of the save file dialog, abort creating new session */
            if (gtk_text_buffer_get_modified(app->editor_buffer)) {
                return;
            }
        } else if (response == GTK_RESPONSE_CANCEL || response == GTK_RESPONSE_DELETE_EVENT) {
            return;
        }
    }

    /* Clear current editor buffer text and open filename reference */
    gtk_text_buffer_set_text(app->editor_buffer, "", 0);
    gtk_text_buffer_set_modified(app->editor_buffer, FALSE);

    if (app->current_filename) {
        g_free(app->current_filename);
        app->current_filename = NULL;
    }

    /*  Clear recent/open files wrappers & UI elements */
    for (int i = 0; i < g_recent_wrapper.count; i++) {
        free(g_recent_wrapper.items[i].filepath);
        free(g_recent_wrapper.items[i].display_name);
        g_recent_wrapper.items[i].filepath = NULL;
        g_recent_wrapper.items[i].display_name = NULL;
    }
    g_recent_wrapper.count = 0;
    recent_files_wrapper_rebuild_ui();

    if (app->recent_tree_store) {
        gtk_tree_store_clear(app->recent_tree_store);
    }
    if (app->tree_store) {
        gtk_tree_store_clear(app->tree_store);
    }
    /* Reload all recent files from database back into Tab 2 */
    const char *sql = "SELECT file_path FROM recent_files ORDER BY last_opened DESC LIMIT 15;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(app->db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        GtkTreeIter iter;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *full_path = (const char *)sqlite3_column_text(stmt, 0);
            if (full_path) {
                char *base_name = g_path_get_basename(full_path);
                gtk_tree_store_append(app->recent_tree_store, &iter, NULL);
                gtk_tree_store_set(app->recent_tree_store, &iter,
                                   0, base_name,   /* Visible label */
                                   1, full_path,   /* Hidden path */
                                   -1);
                g_free(base_name);
            }
        }
        sqlite3_finalize(stmt);
    } else {
        g_printerr("Failed to fetch recent files: %s\n", sqlite3_errmsg(app->db));
    }

    /* Rebuild UI / populate GtkTreeStore with fresh items */ 
    recent_files_wrapper_rebuild_ui();

    /* Notify status bar and set focus on clean editor canvas */
    ui_set_status_alert(app, "Open files cleared. New editable session started.", FALSE);
    if (app->editor_view) {
        gtk_widget_grab_focus(app->editor_view);
    }
   /*  We can also flip this back to index Tab 1 = 0 or Tab 2 = 1   */ 
    gtk_notebook_set_current_page(GTK_NOTEBOOK(app->sidebar_notebook), 1);
}

static void cb_on_save_as(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Save As", GTK_WINDOW(app->window), GTK_FILE_CHOOSER_ACTION_SAVE, "_Cancel", GTK_RESPONSE_CANCEL, "_Save", GTK_RESPONSE_ACCEPT, NULL);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        GtkTextIter start, end;
        char *content;
        gtk_text_buffer_get_bounds(app->editor_buffer, &start, &end);
        content = gtk_text_buffer_get_text(app->editor_buffer, &start, &end, FALSE);
        g_file_set_contents(filename, content, -1, NULL);
        logic_refresh_function_tree(app);
        g_free(content);
        g_free(filename);
    } else if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    
    if (app->current_filename) g_free(app->current_filename);
    app->current_filename = g_strdup(filename);

    /* Perform the write using the quick save callback */
    cb_on_quick_save(widget, data);
    g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

static gboolean cb_on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    AppState *app = (AppState *)data; 

    if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_w || event->keyval == GDK_KEY_W)) {
        if (!app || !app->editor_buffer) return FALSE;

        /* Check if the user has actually made edits since the last save */
        gboolean is_modified = gtk_text_buffer_get_modified(app->editor_buffer);

        if (!is_modified) {
            ui_set_status_alert(app, "No changes detected. Save skipped.", FALSE);
            return TRUE;
        }
        /* Safeguard: Confirm before overwriting an existing file */
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(app->window),
            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_OK_CANCEL,
            "Are you sure you want to overwrite '%s'?", app->current_filename
        );
        
        gtk_window_set_title(GTK_WINDOW(dialog), "Confirm Overwrite");

        gint response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        if (response == GTK_RESPONSE_OK) {
            cb_on_quick_save(widget, data);
            /* Reset the modified flag after a successful save */
            gtk_text_buffer_set_modified(app->editor_buffer, FALSE);
        } else {
            ui_set_status_alert(app, "Save cancelled.", FALSE);
        }
        return TRUE; 
    }
       
    return FALSE;
}

/* GTK3 code works as well,
static void cb_on_save(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    if (app->current_filename == NULL || app->current_filename[0] == '\0') {
        GtkFileChooserNative *native = gtk_file_chooser_native_new(
            "Save File",
            GTK_WINDOW(app->window), 
            GTK_FILE_CHOOSER_ACTION_SAVE,
            "_Save",
            "_Cancel"
        );        
        gint response = gtk_native_dialog_run(GTK_NATIVE_DIALOG(native));
        
        if (response == GTK_RESPONSE_ACCEPT) {
            char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(native));
            if (filename != NULL) {
                g_free(app->current_filename);
                app->current_filename = filename;
            }
        }       
        g_object_unref(native);       
        if (app->current_filename == NULL || app->current_filename[0] == '\0') {
            return; 
        }
    }
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(app->editor_buffer, &start, &end);
    char *text = gtk_text_buffer_get_text(app->editor_buffer, &start, &end, FALSE);
    
    GError *error = NULL;
    gboolean success = g_file_set_contents(app->current_filename, text, -1, &error);  
    
    if (success) {
        gtk_text_buffer_set_modified(app->editor_buffer, FALSE); 
        char *msg = g_strdup_printf("Saved: %s", app->current_filename);
        ui_set_status_alert(app, msg, FALSE);
        g_free(msg);
    } else {
        char *err_msg = g_strdup_printf("Save Failed: %s", error->message);
        ui_set_status_alert(app, err_msg, TRUE);
        g_free(err_msg);
        g_error_free(error);    
    }    
    g_free(text); 
}

* But I will Roll with GTK4 for the save dialog, 
*/
 
static void on_save_dialog_response(GtkNativeDialog *native, gint response_id, gpointer data) {
    AppState *app = (AppState *)data;

    if (response_id == GTK_RESPONSE_ACCEPT) {
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(native);
        char *filename = gtk_file_chooser_get_filename(chooser);
        
        if (filename != NULL) {
            g_free(app->current_filename);
            app->current_filename = filename;
            
            // Resume the saving process now that a filename has been chosen!
            cb_on_save(NULL, app);
        }
    }

    // Clean up the native dialog memory in GTK4
    g_object_unref(native);
}

static void cb_on_save(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    
    /* 1. If there is no filename, show a non-blocking GTK4 native dialog */
    if (app->current_filename == NULL || app->current_filename[0] == '\0') {
        GtkFileChooserNative *native = gtk_file_chooser_native_new(
            "Save File",
            GTK_WINDOW(app->window), /* Ensure app->window points to main GtkWindow */ 
            GTK_FILE_CHOOSER_ACTION_SAVE,
            "_Save",
            "_Cancel"
        );        
        
        // Connect the response signal to our callback handler
        g_signal_connect(native, "response", G_CALLBACK(on_save_dialog_response), app);
        
        // Show the dialog non-blockingly and return immediately
        gtk_native_dialog_show(GTK_NATIVE_DIALOG(native));
        return; 
    }
    
    /* 2. Execution flows straight down to this code block automatically 
     * either when saving an already-named file, or returning from the callback above. */
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(app->editor_buffer, &start, &end);
    char *text = gtk_text_buffer_get_text(app->editor_buffer, &start, &end, FALSE);
    
    GError *error = NULL;
    gboolean success = g_file_set_contents(app->current_filename, text, -1, &error);  
    
    if (success) {
        gtk_text_buffer_set_modified(app->editor_buffer, FALSE); 
        char *msg = g_strdup_printf("Saved: %s", app->current_filename);
        ui_set_status_alert(app, msg, FALSE);
        g_free(msg);
    } else {
        char *err_msg = g_strdup_printf("Save Failed: %s", error->message);
        ui_set_status_alert(app, err_msg, TRUE);
        g_free(err_msg);
        g_error_free(error);    
    }   
    
    g_free(text); 
}

static void cb_on_quick_save(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    
    if (!app->editor_buffer) return;

    /* If no file is currently associated, fall back to "Save As" */
    if (!app->current_filename || strlen(app->current_filename) == 0) {
        cb_on_save_as(widget, data);
        return;
    }

    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(app->editor_buffer, &start, &end);
    char *contents = gtk_text_buffer_get_text(app->editor_buffer, &start, &end, FALSE);

    GError *error = NULL;
    if (g_file_set_contents(app->current_filename, contents, -1, &error)) {
        char *msg = g_strdup_printf("Quick Saved: %s", app->current_filename);
        ui_set_status_alert(app, msg, FALSE);
        g_free(msg);
    } else {
        char *err_msg = g_strdup_printf("Save Failed: %s", error->message);
        ui_set_status_alert(app, err_msg, TRUE);
        g_free(err_msg);
        g_error_free(error);
    }

    g_free(contents);
}


static void cb_on_snippet_insert(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    /* Example: Inserting an ANSI C boilerplate snippet */
    const char *snippet = "#include <stdio.h>\n\nint main(int argc, char *argv[]) {\n\treturn 0;\n}\n";
    gtk_text_buffer_insert_at_cursor(app->editor_buffer, snippet, -1);
}

static void cb_on_search(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    const char *query = gtk_entry_get_text(GTK_ENTRY(app->search_entry));
    char url[1024];
    /* Check if text field is empty or contains only whitespace */
    if (!query || strlen(g_strstrip(g_strdup(query))) == 0) {
        ui_set_status_alert(app, "Search field is empty. Please enter a search query.", TRUE);
        webkit_web_view_load_uri(WEBKIT_WEB_VIEW(app->browser_view), "about:blank");
        return;
    } else { 
    ui_set_status_alert(app, "", FALSE);	
    sprintf(url, "https://www.google.com/search?q=%s", query);
    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(app->browser_view), url);
    }
}

static void cb_on_search_dor(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    const char *query = NULL;
    
    /* Check if this is a predefined search from a menu item */
    query = g_object_get_data(G_OBJECT(widget), "search_query");
    
    /* If no attached data, fall back to the text in the search entry */
    if (!query) {
        query = gtk_entry_get_text(GTK_ENTRY(app->search_entry));
    }
    if (!query || strlen(query) == 0) {
	ui_set_status_alert(app, "Please enter keywords into the search box first.", TRUE);
	webkit_web_view_load_uri(WEBKIT_WEB_VIEW(app->browser_view), "about:blank");
	}
    if (query && strlen(query) > 0) {
        char url[2048];
        /* Use g_ascii_strdown or similar if you want to normalize, 
           otherwise just format the Google search URL */
        snprintf(url, sizeof(url), "https://www.google.com/search?q=%s", query);
        webkit_web_view_load_uri(WEBKIT_WEB_VIEW(app->browser_view), url);
        
        /* Optional: Update the search entry text to show what was searched */
        gtk_entry_set_text(GTK_ENTRY(app->search_entry), query);
    }
}

static void cb_execute_ddg_search(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    const char *query = gtk_entry_get_text(GTK_ENTRY(app->search_entry));

    if (query && strlen(query) > 0) {
        char *escaped_query = g_uri_escape_string(query, NULL, TRUE);

      /*  char *search_url = g_strdup_printf("https://duckduckgo.com/?q=%s", escaped_query); */
      char *search_url = g_strdup_printf("https://duckduckgo.com/?q=%s&kae=d&k7=#000000&kj=#000000", escaped_query);  

        webkit_web_view_load_uri(WEBKIT_WEB_VIEW(app->browser_view), search_url);
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "Browsing: %s\n", search_url);
        gtk_text_buffer_insert_at_cursor(app->console_buffer, log_msg, -1);

        g_free(escaped_query);
        g_free(search_url);
    }
}

void cb_execute_google_search(GtkEntry *entry, AppState *app) {
    const char *text = gtk_entry_get_text(GTK_ENTRY(app->search_entry));
    if (text && strlen(text) > 0) {
        char *encoded_query = g_uri_escape_string(text, NULL, TRUE);
        char *url = g_strdup_printf("https://www.google.com/search?q=%s&kae=d&k7=#000000&kj=#000000", encoded_query);        
        webkit_web_view_load_uri(WEBKIT_WEB_VIEW(app->browser_view), url);
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "Searching...... %s\n", url);
        gtk_text_buffer_insert_at_cursor(app->console_buffer, log_msg, -1);        
        g_free(encoded_query);
        g_free(url);
    }
}

static void cb_on_generate_outline(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    /* Call the logic that scans the buffer for functions */
    logic_refresh_function_tree(app);    
    /* Provide feedback in the console */
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(app->console_buffer, &end);
    gtk_text_buffer_insert(app->console_buffer, &end, "\n[INFO] Code outline updated.", -1);
}

/* GCC compiler dialog options and functions */
static void logic_run_gcc_advanced(AppState *app, const char *src, const char *out, const char *flags) {
    char command[4096];
        /* Core compiler definitions flags matching your project architecture*/
    const char *mandatory = "-ansi -Wall -D_DEFAULT_SOURCE "
                            "`pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.0 sqlite3 gtksourceview-3.0`";

    /* Safety Layer: Sync active changes down to raw storage if buffer is tracking */
    if (app->editor_buffer) {
        GtkTextIter start, end;
        gtk_text_buffer_get_bounds(app->editor_buffer, &start, &end);
        char *content = gtk_text_buffer_get_text(app->editor_buffer, &start, &end, FALSE);
        g_file_set_contents(app->current_filename, content, -1, NULL);
        g_free(content);
    }

    /* Compile instruction targeting customized inputs (-o user_defined_binary) */
    snprintf(command, sizeof(command), "gcc %s %s \"%s\" -o \"%s\"\n", 
             mandatory, flags, src, out);

    /* Write clear trace directly into console buffer terminal panel layout */
    GtkTextIter end_iter;
    gtk_text_buffer_get_end_iter(app->console_buffer, &end_iter);
    gtk_text_buffer_insert(app->console_buffer, &end_iter, "\n> Executing Pipeline Builder:\n", -1);
    gtk_text_buffer_insert(app->console_buffer, &end_iter, command, -1);

    /* Route direct execution to live shell descriptor thread or standard fallback loop */
    if (app->master_fd > 0) {
        write(app->master_fd, command, strlen(command));
    } else {
        int ret = system(command);
        if (ret != 0) {
             ui_set_status_alert(app, "Compilation error identified within thread context", TRUE);
        }
    }
}

static void cb_on_compile(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    current_mood = MOOD_COMPILING;     
    /* Guard Layer: Verify file context path state before proceeding */
    if (app->current_filename == NULL) {
        ui_set_status_alert(app, "Error: Save file to disk before compilation configuration.", TRUE);
        return;
    }

    /* Modal dialogue initialization block matching theme footprint */
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Compiler Matrix Pipeline", 
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_REJECT,
        "BUILD ENGINE", GTK_RESPONSE_ACCEPT,
        NULL
    );

    gtk_window_set_default_size(GTK_WINDOW(dialog), 460, 260);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content_area), 15);

    /* Apply exact Neon Matrix CSS classes to dialog objects */ 
    GtkCssProvider *dialog_css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(dialog_css,
        "dialog, window { background-color: #000000; border: 2px solid #39ff14; }\n"
        "label { color: #39ff14; font-family: 'Monospace'; font-weight: bold; font-size: 11px; }\n"
        "entry { background-color: #111111; color: #00ffff; border: 1px solid #39ff14; caret-color: #00ffff; font-family: 'Monospace'; padding: 4px; }\n"
        "combobox { background-color: #111111; color: #00ffff; border: 1px solid #39ff14; }\n"
        "button { background: #111111; color: #39ff14; border: 1px solid #39ff14; font-family: 'Monospace'; padding: 5px; }\n"
        "button:hover { background: #39ff14; color: #000000; }", -1, NULL);
    gtk_style_context_add_provider(gtk_widget_get_style_context(dialog),
        GTK_STYLE_PROVIDER(dialog_css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_box_pack_start(GTK_BOX(content_area), grid, TRUE, TRUE, 5);
    GtkWidget *in_label = gtk_label_new("Source Input:");
    gtk_widget_set_halign(in_label, GTK_ALIGN_START);
    GtkWidget *in_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(in_entry), app->current_filename);
    gtk_grid_attach(GTK_GRID(grid), in_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), in_entry, 1, 0, 1, 1);
    gtk_widget_set_hexpand(in_entry, TRUE);
    GtkWidget *out_label = gtk_label_new("Target Output:");
    gtk_widget_set_halign(out_label, GTK_ALIGN_START);
    GtkWidget *out_entry = gtk_entry_new();
    char default_out[512];
    g_strlcpy(default_out, app->current_filename, sizeof(default_out));
    char *dot = strrchr(default_out, '.');
    if (dot && strcmp(dot, ".c") == 0) {
        *dot = '\0'; 
    } else {
        g_strlcat(default_out, ".out", sizeof(default_out));
    }
    gtk_entry_set_text(GTK_ENTRY(out_entry), default_out);
    gtk_grid_attach(GTK_GRID(grid), out_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), out_entry, 1, 1, 1, 1);
    GtkWidget *opt_label = gtk_label_new("Optimization:");
    gtk_widget_set_halign(opt_label, GTK_ALIGN_START);
    GtkWidget *opt_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(opt_combo), "-O0 (Debug Mode / No Optimization)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(opt_combo), "-O2 (Standard Production)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(opt_combo), "-Os (Size Optimized Profile)");
    gtk_combo_box_set_active(GTK_COMBO_BOX(opt_combo), 0);
    gtk_grid_attach(GTK_GRID(grid), opt_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), opt_combo, 1, 2, 1, 1);
    GtkWidget *flag_label = gtk_label_new("Extra Flags:");
    gtk_widget_set_halign(flag_label, GTK_ALIGN_START);
    GtkWidget *flag_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(flag_entry), "e.g., -ggdb3 -DDEBUG_MODE");
    gtk_entry_set_activates_default(GTK_ENTRY(flag_entry), TRUE); 
    gtk_grid_attach(GTK_GRID(grid), flag_label, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), flag_entry, 1, 3, 1, 1);
    gtk_widget_show_all(content_area);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        const char *input_src = gtk_entry_get_text(GTK_ENTRY(in_entry));
        const char *output_bin = gtk_entry_get_text(GTK_ENTRY(out_entry));
        
        int active_profile = gtk_combo_box_get_active(GTK_COMBO_BOX(opt_combo));
        const char *profile_flag = "-O0";
        if (active_profile == 1) profile_flag = "-O2";
        else if (active_profile == 2) profile_flag = "-Os";

        const char *user_flags = gtk_entry_get_text(GTK_ENTRY(flag_entry));
        
        char flag_buffer[512];
        snprintf(flag_buffer, sizeof(flag_buffer), "%s %s", profile_flag, user_flags);

        ui_set_status_alert(app, "Deploying compiler pipeline architecture...", FALSE);
        logic_run_gcc_advanced(app, input_src, output_bin, flag_buffer);
    }

    g_object_unref(dialog_css);
    gtk_widget_destroy(dialog);
}


/* This function is for Console bash functions to handle output. 
 * (Shadow tweaking of shell commands allowed) Note shadow not alias,
 * hacking of this custom dumb terminal is up to the end user. 
 * Handle output from Bash -> Display in GTK 
 * */
static gboolean cb_terminal_out(GIOChannel *source, GIOCondition condition, gpointer data) {
    AppState *app = (AppState *)data;
    char buf[4096];
    gsize bytes_read;
    GtkTextIter start, end;

    if (g_io_channel_read_chars(source, buf, sizeof(buf) - 1, &bytes_read, NULL) == G_IO_STATUS_NORMAL) {
        buf[bytes_read] = '\0';
        /* Check if the buffer contains the ANSI clear sequence, \033[H = Move cursor to home, \033[2J = Clear entire screen */ 
        /* If user enters a custom trigger command or Easter egg */
       if (strstr(buf, "\033[H") || strstr(buf, "\033[2J")) {
            gtk_text_buffer_get_bounds(app->console_buffer, &start, &end);
            gtk_text_buffer_delete(app->console_buffer, &start, &end);
            return TRUE;
        } else if (strstr(buf, "cls")) {
		    gtk_text_buffer_get_bounds(app->console_buffer, &start, &end);
            gtk_text_buffer_delete(app->console_buffer, &start, &end);
            gtk_text_buffer_set_text(app->console_buffer, "", 0);
            if (app->master_fd >= 0) {
             write(app->master_fd, "\n", 1);
            }
            return TRUE;
		} else if (strstr(buf, "clr")) {
    		 trigger_cat_animation(app); 
		    gtk_text_buffer_get_bounds(app->console_buffer, &start, &end);
            gtk_text_buffer_delete(app->console_buffer, &start, &end);
            gtk_text_buffer_set_text(app->console_buffer, "", 0);
            if (app->master_fd >= 0) {
				write(app->master_fd, "echo \n", 7); 
			}
            return TRUE;
		}
         
        gtk_text_buffer_get_end_iter(app->console_buffer, &end);
        gtk_text_buffer_insert(app->console_buffer, &end, buf, -1);
        
        /* Scroll to bottom */
        gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(app->console_view), &end, 0.0, FALSE, 0, 0);
    }
    return TRUE;
}
/* Inline html help page */
static void cb_on_help_page(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    if (!app || !app->browser_view) return;

    /* Using a single string literal with implicit concatenation (lines in quotes) */
    const char *html_content = 
        "<html><head><style>"
        "body { background-color: #1e1e1e; background-image: linear-gradient(rgba(255,255,255,.05) 1px, transparent 1px), linear-gradient(90deg, rgba(255,255,255,.05) 1px, transparent 1px); background-size: 20px 20px; color: #d1d1d1; font-family: 'Monospace', 'Courier New', monospace; padding: 40px; line-height: 1.6; max-width: 800px; margin: auto; }"
        ".header-container { display: flex; align-items: center; justify-content: space-between; border-bottom: 2px solid #00ff00; padding-bottom: 10px; margin-bottom: 5px; }"
        "h1 { color: #00ff00; margin: 0; text-transform: uppercase; letter-spacing: 2px; }"
        ".xbm-avatar { width: 32px; height: 32px; background-color: #00ff00; transform: scale(1.5); image-rendering: pixelated; display: inline-block; -webkit-mask-repeat: no-repeat; mask-repeat: no-repeat; }"
        ".subtitle { color: #888; font-style: italic; margin-bottom: 30px; display: block; }"
        "h2 { color: #ff00ff; margin-top: 35px; border-left: 4px solid #ff00ff; padding-left: 10px; }"
        "p { margin-bottom: 15px; }"
        "code { background: #2d2d2d; color: #39ff14; padding: 2px 6px; border-radius: 4px; border: 1px solid #444; }"
        "ul { list-style-type: none; padding-left: 0; }"
        "li { margin-bottom: 10px; padding: 8px; background: rgba(0,0,0,0.3); border-radius: 4px; }"
        ".shortcut { color: #fff; font-weight: bold; min-width: 100px; display: inline-block; }"
        ".forward { background: rgba(57, 255, 20, 0.05); padding: 20px; border-radius: 8px; border: 1px dashed #333; margin-top: 20px; font-size: 0.95em; }"
    
        "</style></head><body>"
        "<h1>MDE Crystalcat</h1>"
        "<span class='subtitle'>A high-performance modular development environment for ANSI C.</span>"
        "<h2>Forward</h2>"
        "<div class='forward'>"
        "<p>Crystalcat was engineered as a high-performance workspace that transcends the limits of a standard C editor."
        "It is a comprehensive <b>Modular Development Environment (MDE)</b> designed for modern web and systems architecture."
        "By integrating private, decentralized search protocols and an open-source SQLite backbone, Crystalcat ensures "
        "your research remains unfiltered and your data remains your own offline. <br><br>"
        "Fully tested with Docker container and Virtual Machines (VMs) - My goal for this program was lightweight, standalone, and executable package of software that includes"
        "everything needed to run an application: code, runtime, system tools, system libraries, and settings all in (Crostini) with minimulist library setup dependency design.<br><br>"
        "<h2>Notes:</h2>"
        "The Chromebook Linux environment (penguin) is neutered, referring to the strict security limitations imposed by ChromeOS, which restrict direct hardware access, lock down the root file system, and isolate the container to protect the core operating system."
        "Google intentionally built the Linux development environment (Crostini) this way for several security and stability reasons As in cheaping out on SSD storage size and short-term based software support cycles that brick the cheap device. Also they work for the Big Iron NSA and other NGO'S the source on cromebooks are closed loop systems and are very low trust based platforms."        
        "</p>"
        "</div>"
        "<h2>Database Engine</h2>"
        "<ul>"
        "<li><b>Integrated SQLite:</b> Crystalcat features a native <code>sqlite3</code> backbone.</li>"
        "<li><b>Ad-Block & Filtering:</b> The database-driven filtering system ensures a clean research environment.</li>"
        "</ul>"
        "<h2>Core Systems Architecture</h2>"
        "<ol>"
        "<li><b>SQLite Backbone:</b> Powers persistent memory and internal states.</li>"
        "<li><b>DUMB PTY/TTY Terminal:</b> A direct forked process for zero-latency shell execution.</li>"
        "<li><b>WebKit Engine:</b> Renders documentation with custom privacy headers.</li>"
        "<li><b>GTK 3.0 UI:</b> Native Linux toolkit implementation.</li>"
        "</ol>"
        "<h2>Language Support</h2>"
        "<ol type='I'>"
        "<li>ANSI C / C++ With a focus on GCC and open source.</li>"
        "<li>HTML5 ideal for website testing.</li>"
        "<li>XHTML</li><li>Python</li>"
        "<li>ASP</li><li>You can write and run assembly language code on a Chromebook by enabling the Linux Development Environment and installing an assembler like NASM or GCC.</li>"
        "<li>CSS</li>"
        "</ol>"
        "<h2>Main keyboard Shortcuts</h2>"
        "<p>See FAQ for full keyboard shortcuts list in a table.</p>"
        "<ul>"
        "<li><span class='shortcut'>Shift + Ctrl + ^</span> Focus browser mode (show/hide editor)</li>"
        "<li><span class='shortcut'>Ctrl + /</span> Toggle Comment</li>"
        "<li><span class='shortcut'>Ctrl + D</span> Duplicate Line</li>"
        "<li><span class='shortcut'>Alt + Up</span> Move Line Up</li>" 
        "<li><span class='shortcut'>Alt + Down</span> Move Line Down</li>" 
        "<li><span class='shortcut'>CTRL + N:</span> New Session</li>"
        "<li><span class='shortcut'>CTRL + W:</span> Quick Save</li>"
        "<li><span class='shortcut'>CTRL + O:</span> Open File</li>"
        "<li><span class='shortcut'>CTRL + S:</span> Save File</li>"
        "<li><span class='shortcut'>CTRL + L:</span> Clear Terminal Console</li>"
        "<li><span class='shortcut'>CTRL + R:</span> Refresh Code Outline</li>"
        "<li><span class='shortcut'>CTRL + 1:</span> Fast toggle JavaScript [ON/OFF]</li>"
        "<li><span class='shortcut'>Ctrl + C:</span> Copy</li>"
        "<li><span class='shortcut'>Ctrl + X:</span> Cut</li>"
        "<li><span class='shortcut'>Ctrl + V:</span> Paste</li>"
        "<li><span class='shortcut'>Ctrl + Z:</span> Undo</li>"
        "<li><span class='shortcut'>Ctrl + A:</span> Select All</li>"
        "<li><span class='shortcut'>Ctrl + Shift + S:</span> Save As</li>"
        "<li><span class='shortcut'>Ctrl + Q:</span> Quit</li>"
        "<li><span class='shortcut'>Ctrl + J:</span> Insert Snippet</li>"
        "<li><span class='shortcut'>Ctrl + F:</span> Find</li>"
        "<li><span class='shortcut'>Ctrl + G:</span> Go to line</li>"
        "</ul>"
        "<h2>Integrated Features</h2>"
        "<ul>"
        "<li><b>Docs Browser:</b> Access search engines and C documentation.</li>"
        "<li><b>Memory Management:</b> Designed with strict C89/C90 compliance.</li>"
        "<li><b>Terminal Emulator:</b> Direct shell access via forked PTY process.</li>"
        "<li>"
        "<h3><i><u>Rabbit & Cat Companion</i></u></h3>" 
        "<dl>"
        "<dt><strong>What are the rabbit and cat characters?</strong></dt>"
        "<dd>These are animated, terminal-style pets integrated into the application interface."
        " The rabbit serves as the primary assistant, while the cat appears to be a secondary character that interacts through a 'cat-chase' logic."
        "</dd>"        
        "<dt><br/><strong>How do these characters behave?</strong></dt>"
        "<dd>"
        "    <ul>"
        "        <li><strong>The Rabbit:</strong> Cycles through animation frames (idle, ear twitch, blinking) and displays different colors based on the assistant's mood (e.g., IDLE, TYPING, COMPILING, ERROR). It also occasionally displays 'idle thoughts' to the user.</li>"
        "        <li><strong>The Cat:</strong> Has its own animation sequence and an 'active' state that triggers a cat-chase scenario. Its activity is determined by random probability within the code.</li>"
        "    </ul>"
        "</dd>"        
        "<dt><strong>How are these characters rendered?</strong></dt>"
        "<dd>The characters are rendered using inline 1-bit XBM data (a format for monochrome bitmap images) that is expanded into 8-bit alpha data for rendering via Cairo, a 2D graphics library.</dd>"        
        "<br/><dt><strong>What does the rabbit's color indicate?</strong></dt>"
        "<dd>The rabbit's color changes according to the system's current state:"
        "    <ul>"
        "        <li><strong>Green:</strong> Default/Idle mood.</li>"
        "        <li><strong>Blue:</strong> Compiling.</li>"
        "        <li><strong>Red:</strong> Error state.</li>"
        "        <li><strong>Random:</strong> When the assistant is in the typing mood, the color is randomized.</li>"
        "    </ul>"
        "</dd>"
        "</dl>"
        "</li>"
        "</ul>"
        "<h2>Companion Feature: The crystal cat </h2>"
        "<p>"
        "In addition to your primary rabbit assistant glitch, you might notice an autonomous cat character appearing in the workspace." 
        "Here are some frequently asked questions regarding this feature:"
        "</p><dl>"
        "<dt><strong>How do I command the cat?</strong></dt>"
        "<dd>"
        "<subtext>So in the text editor just type,</subtext>"
        "<ul>"
        "<ol><li><b>//cat joke</b> shows a random joke</li>"
        "<li><b>//cat clean</b> removes old 'a.out' junk files on the fly</li>"
        "</ol><ul>"
        "</dd>"
        "<dt><strong>When will the cat appear?</strong></dt>"
        "<dd>"
        "  The cat’s activity is determined by random probability within the system logic. When the internal application counter reaches its threshold, there is a 37% chance the cat will trigger an 'active' state and appear on your canvas."
        "</dd>"
        "<dt><strong>What does the cat do?</strong></dt>"
        "<dd>"
        "    The cat face is a visual animation element 1st and background auto macro 2nd. Once active, it loops through specific movement frames (Idle, Eyes blink) before automatically resetting and disappearing from the screen."
        "But the cat is always active hunting the ribbit in the background for code errors etc." 
        "</dd></dl>"        
        "</body></html>";

    webkit_web_view_load_html(WEBKIT_WEB_VIEW(app->browser_view), html_content, "about:blank");
}

/* FAQ Dialog inline the total html file into our program. */
static const char *INLINE_FAQ_HTML = 
"<!DOCTYPE html>"
"<html lang=\"en\">"
"<head>"
"    <meta charset=\"UTF-8\">"
"    <title>Crystalcat MDE — Help & FAQ</title>"
"    <style>"
"        :root {"
"            --bg-color: #0f0f13;"
"            --card-bg: #181820;"
"            --text-color: #e0e0e0;"
"            --accent-green: #39ff14;"
"            --accent-cyan: #00e5ff;"
"            --accent-red: #ff3333;"
"            --border-color: #2a2a38;"
"            --code-bg: #111116;"
"        }"
"        #skew {"
"              position: absolute;"
"              right: 9px; "
"              top: 259px;"
"              z-index: -1;"
"              color: var(--accent-cyan);" 
"              font-size: 2.4em;"
"              text-align: right;"
"              white-space: nowrap; "
"              opacity: 0.3;"
"              transform-origin: top right;"
"              transform: translate(-20px, 0px) scale(0.5) rotate(87deg) skew(-8deg, -5deg);"
"             }"
"        body {"
"            background-image: linear-gradient(rgba(255,255,255,.02) 1px, transparent 1px), linear-gradient(90deg, rgba(255,255,255,.04) 1px, transparent 1px);" 
"            background-size: 20px 20px; color: #d1d1d1;" 
"            max-width: 800px;"
"            font-family: 'Courier New', Courier, monospace, sans-serif;"
"            background-color: var(--bg-color);"
"            color: var(--text-color);"
"            line-height: 1.6;"
"            margin: auto;"
"            padding: 20px;"
"        }"
"        .container { max-width: 900px; margin: 0 auto; }"
"        header { border-bottom: 2px  double var(--accent-green); padding-bottom: 15px; margin-bottom: 30px; }"
"        h1 { "
"               color: var(--accent-green);" 
"               margin: 1px 1px 10px 0;" 
"               font-size: 3em; "
"               text-transform: uppercase;" 
"               letter-spacing: 0.04px; "
"               text-shadow: 2px 2px 0 #4074b5, 2px -2px 0 #4074b5, "
"                -2px 2px 0 #4074b5, -2px -2px 0 #4074b5, "
"                2px 0px 0 #4074b5, 0px 2px 0 #4074b5, -2px 0px 0 #4074b5," 
"                0px -2px 0 #4074b5, 8px -2px 1px rgba(44,217,15,0);"
"               }"
"        .subtitle { color: var(--accent-cyan); font-size: 0.95em; }"
"        h2 { color: var(--accent-cyan); border-bottom: 1px solid var(--border-color); padding-bottom: 5px; margin-top: 40px; text-transform: uppercase; font-size: 1.3em; }"
"        .faq-item {"
"            background-color: var(--card-bg);"
"            border: 1px solid var(--border-color);"
"            border-left: 4px solid var(--accent-green);"
"            border-radius: 4px;"
"            padding: 15px 20px;"
"            margin-bottom: 15px;"
"        }"
"        .question { font-weight: bold; color: #ffffff; margin-bottom: 8px; font-size: 1.05em; }"
"        .answer { color: #cccccc; font-size: 0.95em; }"
"        .answer ul { margin: 8px 0; padding-left: 20px; }"
"        .answer li { margin-bottom: 4px; }"
"        code {"
"            background-color: var(--code-bg);"
"            color: var(--accent-green);"
"            padding: 2px 6px;"
"            border-radius: 3px;"
"            font-family: 'Courier New', Courier, monospace;"
"            font-size: 0.9em;"
"            border: 1px solid #222;"
"        }"
"        .status-idle { color: var(--accent-green); font-weight: bold; }"
"        .status-typing { color: #ffff00; font-weight: bold; }"
"        .status-compile { color: var(--accent-cyan); font-weight: bold; }"
"        .status-error { color: var(--accent-red); font-weight: bold; }"
"        footer { margin-top: 50px; text-align: center; font-size: 0.85em; color: #666; border-top: 1px solid var(--border-color); padding-top: 20px; }"
"    </style>"
"</head>"
"<body>"
"<div class=\"container\">"
"    <header>"
"        <h1>Crystalcat MDE</h1>"
"        <div class=\"subtitle\">Modular Development Environment — Frequently Asked Questions & System Manual</div>"
"    </header>"
"    <h2>General & Architecture</h2>"
"    <div class=\"faq-item\">"
"        <div class=\"question\">Q: What is Crystalcat MDE?</div>"
"        <div class=\"answer\">"
"            <strong>Crystalcat MDE</strong> is a lightweight, Modular Development Environment built in C using GTK+ 3, WebKit2GTK, SQLite3, and GtkSourceView. It combines a code editor, an embedded web browser, an integrated terminal, and an animated digital assistant into a unified developer workspace."
"        </div>"
"    </div>"
"    <div class=\"faq-item\">"
"        <div class=\"question\">Q: What operating systems or environments is this program designed for?</div>"
"        <div class=\"answer\">"
"            The code is written in C and targets Linux / Unix-like POSIX environments (notably designed and tested for ChromeOS Linux / Crostini environments). It includes fallback systems and clipboard HUD bridges to work seamlessly within strict browser sandboxes like Google Chrome/Chromium."
"        </div>"
"    </div>"
"    <div class=\"faq-item\">"
"        <div class=\"question\">Q: What libraries and compiler flags are required to build Crystalcat MDE?</div>"
"        <div class=\"answer\">"
"            To compile the application, you need standard development headers for:"
"            <ul>"
"                <li><strong>GTK+ 3</strong> (<code>gtk+-3.0</code>)</li>"
"                <li><strong>WebKit2GTK</strong> (<code>webkit2gtk-4.0</code>)</li>"
"                <li><strong>GtkSourceView</strong> (<code>gtksourceview-3.0</code> or <code>gtksourceview-4.0</code>)</li>"
"                <li><strong>SQLite3</strong> (<code>sqlite3</code>)</li>"
"                <li>Standard math and POSIX C libraries (<code>-lm</code>)</li>"
"            </ul>"
"        </div>"
"    </div>"
"    <h2>Companion Assistant & UI Aesthetics</h2>"
"    <div class=\"faq-item\">"
"        <div class=\"question\">Q: What is the pixelated pet on the screen, and what does it do?</div>"
"        <div class=\"answer\">"
"            The assistant panel features a retro 1-bit bitmap character (a rabbit/cat rendered directly via Cairo masks from raw XBM bit arrays). The assistant serves multiple purposes:"
"            <ul>"
"                <li><span class=\"status-idle\">Idle:</span> Green (<code>#39FF14</code>), displaying random developer thoughts or reminders.</li>"
"                <li><span class=\"status-typing\">Typing:</span> Rapid multi-color shifts while actively editing code in the workspace.</li>"
"                <li><span class=\"status-compile\">Compiling:</span> Cyan/Blue cycling through frame animation states.</li>"
"                <li><span class=\"status-error\">Error:</span> Bright Red (<code>#FF3333</code>) static or twitching frame.</li>"
"                <li><strong>Cat-Chase Animation:</strong> Periodic random cat animations that chase or interact across the main canvas area.</li>"
"            </ul>"
"        </div>"
"    </div>"
"    <h2>Web browser content reader features</h2>"
"<div class=\"faq-item\">"
"    <div class=\"question\">Q: What is Article &amp; Image Isolation, and how do I use Reader mode?</div>"
"    <div class=\"answer\">"
"        A: Article &amp; Image Isolation lets you control which parts of a web page"
"        can load or display, helping create a cleaner reading experience and reduce"
"       unwanted page content."
"        <p><strong>Reader presets:</strong></p>"
"        <ul>"
"            <li><strong>Strict Reader:</strong> Applies stricter content restrictions"
"                for a simplified reading experience.</li>"
"            <li><strong>Balanced Reader:</strong> Provides a balance between"
"                readability and website functionality.</li>"
"            <li><strong>Images + Text:</strong> Allows article text and images"
"                while restricting other content.</li>"
"            <li><strong>Normal Browsing:</strong> Restores normal browsing behavior"
"                when you want the full website experience.</li>"
"        </ul>"
"        <p><strong>Custom settings:</strong> The Article &amp; Image Isolation"
"        dialog provides controls for JavaScript, page layout elements, fonts,"
"        media, raw resources, and popups. Adjust these options to customize"
"        what the page is allowed to display or load.</p>"
"        <p><strong>Applying your settings:</strong> Select your desired options"
"        and use <em>Apply</em> to apply them. Use <em>Reset Controls</em> to"
"        reset the dialog's controls, or <em>Restore Normal Browsing</em> to"
"        return to normal browsing.</p>"
"        <p><strong>Filter source:</strong> The source view displays the generated"
"        WebKit content-filter JSON associated with the current isolation settings."
"        This allows you to inspect the rules being used.</p>"
"        <p><strong>Note:</strong> Strict content restrictions may prevent some"
"        websites from working correctly. If a page loses functionality, try"
"        Balanced Reader or Restore Normal Browsing.</p>"
"    </div>"
"</div>"
"    <h2>Code Editor & Database Features</h2>"
"    <div class=\"faq-item\">"
"        <div class=\"question\">Q: How does the Recent Files history work?</div>"
"        <div class=\"answer\">"
"            Crystalcat MDE manages recent files using two parallel systems:"
"            <ul>"
"                <li><strong>In-Memory Ring Buffer:</strong> Stores up to 10 recent items dynamically rebuilt inside the application menus (<code>RecentFilesWrapper</code>).</li>"
"                <li><strong>SQLite Database Persistence:</strong> Tracks recent files inside <code>ide_context.db</code> in a table named <code>recent_files</code>. Selecting a file from the sidebar's \"Recent\" tab reloads it directly into the editor view.</li>"
"            </ul>"
"        </div>"
"    </div>"
"    <div class=\"faq-item\">"
"        <div class=\"question\">Q: What safety prompt triggers when switching files?</div>"
"        <div class=\"answer\">"
"            If you activate a file from the \"Recent\" tree while your current buffer contains unsaved modifications, a GTK modal prompt appears asking whether to <strong>Save</strong>, <strong>Discard</strong>, or <strong>Cancel</strong> before opening the target file."
"        </div>"
"    </div>"
"    <div class=\"faq-item\">"
"        <div class=\"question\">Q: Does the editor support search and line navigation?</div>"
"        <div class=\"answer\">"
"            Yes:"
"            <ul>"
"                <li><strong>Find Dialog:</strong> Supports forward/backward text matching with optional <em>Case Sensitive</em> and <em>Match Whole Word</em> flags.</li>"
"                <li><strong>Go to Line:</strong> Allows rapid jumping to specific line numbers by converting human 1-indexed line numbers to zero-indexed GtkTextIters.</li>"
"            </ul>"
"        </div>"
"    </div>"
"    <h2>Web Browser, Privacy & Security</h2>"
"    <div class=\"faq-item\">"
"        <div class=\"question\">Q: Does the embedded browser support Ad Blocking?</div>"
"        <div class=\"answer\">"
"            Yes. Crystalcat MDE features a custom SQLite-backed ad filter system:"
"            <ul>"
"                <li>Filters are stored in the SQLite table <code>ad_filters</code>.</li>"
"                <li>Patterns (e.g., <code>.*doubleclick.net.*</code>, <code>.*google-analytics.com.*</code>, tracker pixels, etc.) are compiled on the fly into JSON rules and passed to WebKit’s <code>WebKitUserContentFilterStore</code>.</li>"
"                <li>You can manage or append custom URL block patterns using the <strong>Manage Filters</strong> menu dialog.</li>"
"            </ul>"
"        </div>"
"    </div>"
"    <div class=\"faq-item\">"
"        <div class=\"question\">Q: What privacy and security features are included?</div>"
"        <div class=\"answer\">"
"            <ul>"
"                <li><strong>AI content stripping Just in time</strong> Allows instant disabling of AI html injection and agents.</li>"
"                <li><strong>JavaScript Toggle:</strong> Allows instant enabling/disabling of JavaScript execution across the browser.</li>"
"                <li><strong>Hardened Mode:</strong> Disables JavaScript, inline JS markup, and site-specific quirks for high-privacy browsing.</li>"
"                <li><strong>Do-Not-Track (DNT):</strong> Sends DNT signal headers via WebKit settings when supported.</li>"
"                <li><strong>Cookie Manager:</strong> Opens a dedicated \"Cookie Monster\" window to inspect active session cookies or flush all stored domain cookies at once.</li>"
"            </ul>"
"        </div>"
"    </div>"
"    <div class=\"faq-item\">"
"        <div class=\"question\">Q: What is the \"Security Bridge / Persistent HUD\"?</div>"
"        <div class=\"answer\">"
"            When opening specific system-level or internal Chrome URLs (<code>chrome://</code> or <code>webkit://</code>), ChromeOS sandboxes often block standard launch attempts. Crystalcat MDE intercepts these requests, automatically copies the URI to your system clipboard, and presents a borderless floating dark-mode HUD window. Once confirmed, it launches external focus so you can paste (<code>CTRL+V</code>) the URL directly into your main address bar."
"        </div>"
"    </div>"
"    <h2>Search Engine & HTML Utilities</h2>"
"    <div class=\"faq-item\">"
"        <div class=\"question\">Q: What search tools and HTML utilities are included?</div>"
"        <div class=\"answer\">"
"            <ul>"
"                <li><strong>Advanced Search Dialog:</strong> Allows structured search queries combining keywords, target domains (<code>site:</code>), locations, and date ranges directly launching external Google queries.</li>"
"                <li><strong>HTML Boilerplate & Insertions:</strong> Includes quick workflows for entity escaping/unescaping, template insertion, image tags, links, tables, comments, and Lorem Ipsum placeholder text.</li>"
"            </ul>"
"        </div>"
"    </div>"
"    <h2>Multimedia & Embedded Terminal</h2>"
"    <div class=\"faq-item\">"
"        <div class=\"question\">Q: How does the built-in Audio Stream Player work?</div>"
"        <div class=\"answer\">"
"            When opening an audio stream URL, Crystalcat MDE launches a dedicated compact window rendering an HTML5/CSS audio player powered by WebKit. It includes play/pause/stop controls, volume sliders, and real-time network and buffer health indicators (<code>buffered</code>, <code>networkState</code>, <code>readyState</code>)."
"            You also have the option of embedding the somafm streaming website within the app crystalcat."
"        </div>"
"    </div>"
"    <div class=\"faq-item\">"
"        <div class=\"question\">Q: Does the MDE include an integrated terminal or compilation tool?</div>"
"        <div class=\"answer\">"
"            Yes:"
"            <ul>"
"                <li><strong>Embedded Terminal:</strong> Spawns sub-shell processes using POSIX pseudo-terminals (<code>master_fd</code> via <code>openpty</code>/<code>fcntl</code>) hooked into a GTK console buffer.</li>"
"                <li><strong>GCC Compilation Engine:</strong> Includes internal routines (<code>logic_run_gcc_advanced</code>) to directly trigger system GCC compilation commands on open source files.</li>"
"            </ul>"
"        </div>"
"    </div>"
"    <div class=\"faq-item\">"
"        <div class=\"question\">Q: Why is the browser so paranoid?</div>"
"        <div class=\"answer\">"
"            Yes paranoid I am! :" 
"            <p>If it feels like the browser is acting paranoid — blocking scripts, popping up alerts, or breaking pages— it’s usually down to a few specific reasons built into its design:<br><br>"
"            <ul>"
"            <li>Hardened Defaults<br>"
"            By default, the browser is configured to assume that untrusted code could and will be malicious. Features like Hardened Privacy Mode and strict HTTP header enforcement lock down common attack vectors (like cross-site scripting or fingerprinting), but they can sometimes cause aggressive site-blocking this is A good thing. AI agents are always hacking code in real time!<br></li>"
"            <li> Aggressive Cookie & Tracker Blocking<br>"
"             With Intelligent Tracking Protection (ITP) and active database-level web filtering, the browser constantly scans incoming network requests. If a site tries to load third-party tracking scripts, persistent cookies, or telemetry pixels, the browser immediately cuts off the connection.<br></li>"
"            <li> JavaScript Restriction as default<br>"
"            If JavaScript is toggled off or heavily restricted via the menu, many modern websites (which rely on JS to render images, menus, and dynamic content) will instantly freeze or break. To the user, it feels like the browser is panicking, but it's actually just refusing to run unverified AI code. The Internet is a boneyard of broken dead code or DIT.<br></li>"
"            <li> Local Database Rule Enforcement<br>"
"            Because the browser checks incoming URLs against local SQLite filter lists, it acts as a local firewall. If a domain matches a pattern flagged for ad-tracking or invasive data collection, access is immediately blocked without asking for permission first by default.<br></li>"
" </ul> "
"        </div>"
"     </div>"
"  </div>"
"    <main>"
" <header>"
"        <h1>🛡️ Privacy Features FAQ</h1>"
"        <p class=\"subtitle\">Learn more about the built-in tracking protection and security controls in our browser.</p>"
"    </header>"
"        <article class=\"faq-item\">"
"            <h2 class=\"faq-question\">1. What new privacy protections have been added?</h2>"
"            <div class=\"faq-answer\">"
"                <p>Our latest release introduces several built-in privacy tools designed to give you complete control over your data and brick AI agents:</p>"
"                <ul>"
"                    <li><strong>Hardened Privacy Mode:</strong> A one-click toggle that enforces strict security rules across all sites.</li>"
"                    <li><strong>Smart Cookie Management:</strong> Enhanced tracking protection to manage, block, or modify how third-party cookies track you.</li>"
"                    <li><strong>Integrated Do Not Track (DNT) &amp; ITP:</strong> Native support for Intelligent Tracking Protection and standard DNT signals.</li>"
"                    <li><strong>Custom Web Filters:</strong> Built-in ad and domain filtering powered by a dedicated database.</li>"
"                    <li><strong>JavaScript Control:</strong> Global or site-by-site toggles to disable high-risk scripts when needed.</li>"
"                </ul>"
"            </div>"
"        </article>"
"        <article class=\"faq-item\">"
"            <h2 class=\"faq-question\">2. What is \"Hardened Privacy Mode\"?</h2>"
"            <div class=\"faq-answer\">"
"                <p>Hardened Privacy Mode is an all-in-one security setting. When enabled, it automatically applies maximum restriction settings—blocking aggressive fingerprinting techniques, restricting third-party tracking scripts, and enforcing strict HTTP headers to ensure your browsing identity stays private.</p>"
"            </div>"
"        </article>"
"        <article class=\"faq-item\">"
"            <h2 class=\"faq-question\">3. How does the Cookie Manager work?</h2>"
"            <div class=\"faq-answer\">"
"                <p>Our dynamic Cookie Manager puts you in charge of local data:</p>"
"                <ul>"
"                    <li><strong>Inspect:</strong> View stored tracking cookies per session or domain.</li>"
"                    <li><strong>Modify &amp; Alter:</strong> Selectively alter cookie attributes or expire invasive tracking tokens manually.</li>"
"                    <li><strong>Auto-Clear:</strong> Automatically purge non-essential tracking cookies when you close a session.</li>"
"                </ul>"
"            </div>"
"        </article>"
"        <article class=\"faq-item\">"
"            <h2 class=\"faq-question\">4. How do I enable or manage Web Filters?</h2>"
"            <div class=\"faq-answer\">"
"                <p>You can access web filtering directly from the menu bar:</p>"
"                <ol>"
"                    <li>Navigate to <span class=\"menu-path\">Tools &gt; Manage Web Filters...</span></li>"
"                    <li>Add custom URL patterns, domain targets, or blocklists.</li>"
"                    <li>Choose whether to block requests outright or alter headers to mask your digital footprint.</li>"
"                </ol>"
"                <p>Your filter list is stored locally on your device in a lightweight SQLite database, meaning your rules are never uploaded to our servers.</p>"
"            </div>"
"        </article>"
"        <article class=\"faq-item\">"
"            <h2 class=\"faq-question\">5. Will toggling off JavaScript break websites?</h2>"
"            <div class=\"faq-answer\">"
"                <p>Disabling JavaScript (via <span class=\"menu-path\">Browser &gt; Enable JavaScript</span>) significantly boosts speed and stops tracking scripts, but it may cause dynamic web applications (like video platforms or online editors) to render incorrectly. We recommend toggling JavaScript off for simple reading and toggling it back on for complex web apps.</p>"
"            </div>"
"        </article>"
"        <article class=\"faq-item\">"
"            <h2 class=\"faq-question\">6. How do I enable these features?</h2>"
"            <div class=\"faq-answer\">"
"                <p>Most privacy controls can be easily toggled from the top menu:</p>"
"                <ul>"
"                    <li><strong>To manage tracking signals:</strong> Open the <span class=\"menu-path\">Browser</span> menu and check <strong>Do Not Track / ITP</strong> or <strong>Hardened Privacy Mode</strong>.</li>"
"                    <li><strong>To manage cookies:</strong> Select <span class=\"menu-path\">Browser &gt; Cookie Manager...</span></li>"
"                    <li><strong>To perform private searches:</strong> Use the built-in DuckDuckGo search shortcut directly from the search bar.</li>"
"                </ul>"
"            </div>"
"        </article>"
"        <article class=\"faq-item\">"
"            <h2 class=\"faq-question\">7. What is \"Agent Spoofing\"?</h2>"
"            <div class=\"faq-answer\">"
"                <p>Agent Spoofing (or User-Agent spoofing) alters the information your browser sends to websites about your device and operating system. By disguising your browser identity as a generic or different device, it prevents websites and tracking networks from building a unique fingerprint based on your specific browser setup.</p>"
"            </div>"
"        </article>"
"        <article class=\"faq-item\">"
"            <h2 class=\"faq-question\">8. How does Geolocation Randomization work?</h2>"
"            <div class=\"faq-answer\">"
"                <p>Geolocation Randomization dynamically spoofs your browser's physical location API. Instead of revealing your true coordinates or relying on fixed locations, it injects random latitude and longitude points into the web environment for each session, preventing websites from using location data to track or pinpoint your geographic position. This is designed to be used in conjunction with a proxy and VPN for comprehensive location privacy. It even logs a funny message in the developer console to let you know your real location remains sudo-hidden within the layers of your browser.</p>"
"            </div>"
"        </article>"
"    </main>"
"    </div>"
"   <h2>Editor keyboard shortcuts</h2>"
"    <div class=\"faq-item\">"
"    <div class=\"question\">Q: What are the keyboard shortcuts for the editor?</div>" 
"    <div class=\"answer\">"
"<table style='border: 4px double #333; border-collapse: separate; border-spacing: 0; padding: 10px;'>"
"  <tr>"
"    <td style='border: 1px solid #ddd; padding: 10px;'>"
"      <ol start='1'>"
"        <li><span class='shortcut'>Ctrl + /</span> Toggle Comment</li>"
"        <li><span class='shortcut'>Ctrl + D</span> Duplicate Line</li>"
"        <li><span class='shortcut'>Alt + Up</span> Move Line Up</li>"
"      </ol>"
"    </td>"
"    <td style='border: 1px solid #ddd; padding: 8px;'>"
"      <ol start='4'>"
"        <li><span class='shortcut'>Alt + Down</span> Move Line Down</li>"
"        <li><span class='shortcut'>CTRL + N:</span> New Session</li>"
"        <li><span class='shortcut'>CTRL + W:</span> Quick Save</li>"
"      </ol>"
"    </td>"
"  </tr>"
"  <tr>"
"    <td style='border: 1px solid #ddd; padding: 8px;'>"
"      <ol start='7'>"
"        <li><span class='shortcut'>CTRL + O:</span> Open File</li>"
"        <li><span class='shortcut'>CTRL + S:</span> Save File</li>"
"        <li><span class='shortcut'>CTRL + L:</span> Clear Terminal</li>"
"      </ol>"
"    </td>"
"    <td style='border: 1px solid #ddd; padding: 8px;'>"
"      <ol start='10'>"
"        <li><span class='shortcut'>CTRL + R:</span> Refresh Code Outline</li>"
"        <li><span class='shortcut'>CTRL + 1:</span> Toggle JavaScript-[ON/OFF]</li>"
"        <li><span class='shortcut'>Ctrl + C:</span> Copy</li>"
"      </ol>"
"    </td>"
"  </tr>"
"  <tr>"
"    <td style='border: 1px solid #ddd; padding: 8px;'>"
"      <ol start='13'>"
"        <li><span class='shortcut'>Ctrl + X:</span> Cut</li>"
"        <li><span class='shortcut'>Ctrl + V:</span> Paste</li>"
"        <li><span class='shortcut'>Ctrl + Z:</span> Undo</li>"
"      </ol>"
"    </td>"
"    <td style='border: 1px solid #ddd; padding: 8px;'>"
"      <ol start='16'>"
"        <li><span class='shortcut'>Ctrl + A:</span> Select All</li>"
"        <li><span class='shortcut'>Ctrl + Shift + S:</span> Save As</li>"
"      </ol>"
"    </td>"
"  </tr>"
"  <tr>"
"    <td style='border: 1px solid #ddd; padding: 8px;'>"
"      <ol start='19'>"
"        <li><span class='shortcut'>Ctrl + Q:</span> Quit</li>"
"        <li><span class='shortcut'>Ctrl + J:</span> Insert Snippet</li>"
"      </ol>"
"    </td>"
"    <td style='border: 1px solid #ddd; padding: 8px;'>"
"      <ol start='21'>"
"        <li><span class='shortcut'>Ctrl + F:</span> Find</li>"
"        <li><span class='shortcut'>Ctrl + G:</span> Go to line</li>"
"        <li><apan class='shortcut'>Shift + Ctrl + 6:</span> Focus Browser mode."
"      </ol>"
"    </td>"
"  </tr>"
"</table>"
"   </div>"
"   </div>"
"    <footer>"
"        &bull; Crystalcat MDE Internal Documentation &bull; Built with GTK+ & WebKit2GTK &bull; </br> <u>Open Source for Liinux and Cromebook computing devices</u> .o*(<b>2026</b>)*o."
"    </footer>"
"</div>"
"<div id='skew'>By Johnny B Stroud </div>"
"</body>"
"</html>";

void on_menu_help_faq_activate(GtkMenuItem *menuitem, gpointer user_data) {
    GtkWidget *parent_window = GTK_WIDGET(user_data);

    GtkWidget *faq_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(faq_window), "Crystalcat MDE — Help & FAQ");
    gtk_window_set_default_size(GTK_WINDOW(faq_window), 950, 700);
    gtk_window_set_transient_for(GTK_WINDOW(faq_window), GTK_WINDOW(parent_window));
    gtk_window_set_position(GTK_WINDOW(faq_window), GTK_WIN_POS_CENTER_ON_PARENT);
    GtkWidget *web_view = webkit_web_view_new();
    gtk_container_add(GTK_CONTAINER(faq_window), web_view);
    webkit_web_view_load_html(WEBKIT_WEB_VIEW(web_view), INLINE_FAQ_HTML, NULL);
    gtk_widget_show_all(faq_window);
}

/* Rotote statusbar text color */ 
static gboolean cb_rotate_status_color(gpointer data) {
    AppState *app = (AppState *)data;
    if (!app || !app->statusbar || !app->status_css_provider) return TRUE;

    /* Static counter to track iterations across function calls */
    static int change_count = 0;
    char *color_str;

    if (change_count < 100) {
        /* Phase 1: Toggle between Neon Cyan and Neon Green */
        app->color_toggle = !app->color_toggle;
        const char *fixed_color = (app->color_toggle) ? "#00ffff" : "#39ff14";
        color_str = g_strdup(fixed_color);
        change_count++;
    } else {
        /* Phase 2: Generate Random Colors (Vibrant/Neon range) */
        int r = g_random_int_range(100, 256);
        int g = g_random_int_range(100, 256);
        int b = g_random_int_range(100, 256);
        color_str = g_strdup_printf("#%02x%02x%02x", r, g, b);
    }

    /* Target the ID and label node. */
    char *css = g_strdup_printf(
        "#status-text-area label { "
        "  color: %s; "
        "  transition: color 200ms ease-in-out; "
        "  font-weight: bold; "
        "  text-shadow: 0 0 3px %s; " 
        "}", color_str, color_str);

    /* Load the new CSS into the EXISTING provider */
    gtk_css_provider_load_from_data(app->status_css_provider, css, -1, NULL);

    /* Clean up */ 
    g_free(color_str);
    g_free(css);

    return TRUE; 
}

/* Handle keyboard input from GTK -> Send to Bash */
static gboolean cb_console_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    AppState *app = (AppState *)data;
    if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_l || event->keyval == GDK_KEY_L)) {
        GtkTextIter start, end;
        gtk_text_buffer_get_bounds(app->console_buffer, &start, &end);
        gtk_text_buffer_delete(app->console_buffer, &start, &end);
        return TRUE;
    }
    if (app->master_fd > 0) {
        if (event->keyval == GDK_KEY_Return || event->keyval == GDK_KEY_KP_Enter) {
            write(app->master_fd, "\n", 1);
            return TRUE;
        }
        write(app->master_fd, event->string, strlen(event->string));
        return TRUE; 
    }
    return FALSE;
}

/* Runtime buffer triggers for assistant dynamic */
static void logic_update_assistant_message(AppState *app, const char *msg) {
if (!app) return;

    /* Ensure the label widget is valid before updating text */
    if (app->assistant_label && GTK_IS_LABEL(app->assistant_label)) {
        gtk_label_set_text(GTK_LABEL(app->assistant_label), msg);
    }

    /* Ensure the container widget is valid before showing */
    if (app->assistant_box && GTK_IS_WIDGET(app->assistant_box)) {
        gtk_widget_show_all(app->assistant_box);
    }
    
    if (!msg || strlen(msg) == 0) {
        gtk_widget_hide(app->assistant_box);
    } else {
        gtk_label_set_text(GTK_LABEL(app->assistant_label), msg);
        gtk_widget_show_all(app->assistant_box);
    }
}

/* Event handler that hooks onto your GtkTextBuffer change signals */
static void cb_on_editor_buffer_changed(GtkTextBuffer *buffer, gpointer user_data) {
    AppState *app = (AppState *)user_data;
    
    current_mood = MOOD_TYPING;
    typing_cooldown_counter = 100; 
    
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    char *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    update_rabbit_animation_state(app);   
    /*Concrete helper rules:*/
    if (strstr(text,"/*")) {
	logic_update_assistant_message(app, "Remember to close your comment with */ .. "); 	
    current_mood = MOOD_TYPING;  
    } else if (strstr(text, "//cat time")) {
        time_t rawtime;
        struct tm *timeinfo;
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        
        char time_msg[128];
        snprintf(time_msg, sizeof(time_msg), "🕒 Local Matrix Time: %02d:%02d:%02d", 
                 timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
        logic_update_assistant_message(app, time_msg);
        
    } else if (strstr(text, "//cat joke")) {
        const char *jokes[] = {
            "Why do programmers wear glasses? Because they can't C#!",
            "There are 10 types of people: those who understand binary, and those who don't.",
            "A SQL query walks into a bar, walks up to two tables and asks, 'Can I join you?'",
            "So why don't scientists trust atoms? Because they make up everything.",
            "The future, the present, and the past walked into a bar. Things got a little tense.", 
            "I threw a boomerang a few years ago. I now live in constant fear.", 
            "Parallel lines have so much in common. It’s a shame they’ll never meet but match."
        };
        static int joke_index = 0;
        logic_update_assistant_message(app, jokes[joke_index]);
        joke_index = (joke_index + 1) % 7;
        
    } else if (strstr(text, "//cat clean")) {
        /* Trigger an actual system cleanup or file tracking task! */
        logic_update_assistant_message(app, "🧹 Clearing out temporary local workspace builds...");
        system("rm -f *.o a.out"); 
    } else if (strstr(text, "johnny")) {
    logic_update_assistant_message(app, "Rabbit eats carrots hint.");
    } else if (strstr(text, "buckallew")) {
	logic_update_assistant_message(app, "Feed me a carrot already johnny"); 	
    } else if (strstr(text, "stroud")) {
		const char *eggs[] = {
			"Are you going to type something?",
			"How am I doing boss?",
			"The author's 1st name is Johnny..",
			"I am here to help!",
			"It's coffee time.."
		};
		static int eggs_index = 0; 
	    logic_update_assistant_message(app, eggs[eggs_index]);
	    eggs_index = (eggs_index + 1) % 5; 
	/* If user enters a custom trigger command or Easter egg */  
	} else if (strstr(text, "🥕") || strstr(text, "carrot")) {
        logic_update_assistant_message(app, "🎉 *Nom nom nom*! Thank you for the carrot! Energy levels at 100%!");        
        /* If you implemented the mood states from earlier, you can trigger a happy state! */ 
        current_mood = MOOD_TYPING; 
    } else if (strstr(text, "TODO:") || strstr(text, "FIXME:") || strstr(text, "NOTES:")) {
        logic_update_assistant_message(app, "⚠️ Flagged critical notes inside current frame workspace.");
    } else if (strstr(text, "strcpy")) {
        logic_update_assistant_message(app, "🛡️ Warning: strcpy() is vulnerable to buffer overflows. Use strncpy()!");
    } else if (strstr(text, "malloc")) {
        logic_update_assistant_message(app, "💾 Memory requested from the heap. Don't forget to free() it later!");
    } else if (strstr(text, "goto")) {
        logic_update_assistant_message(app, "🦎 Uh oh, a goto statement! Somewhere, a computer science professor is crying.");
    } else if (strstr(text, "while(1)") || strstr(text, "while (1)")) {
        logic_update_assistant_message(app, "🌀 Safety check: Make sure your infinite loop has a break statement!");
    } else if (strstr(text, "pthread_create") && !strstr(text, "pthread_join")) {
        logic_update_assistant_message(app, "💡 Thread instantiation recognized. Ensure join conditions are explicitly mapped.");
    } else if (strstr(text, "gets(")) {
    logic_update_assistant_message(app, "🚫 Danger: gets() is deprecated and unsafe. Use fgets() instead!");
    } else if (strstr(text, "strcpy(")) {
    logic_update_assistant_message(app, "💡 Tip: strcpy() can cause overflows. Check if strncpy() or strlcpy() is better suited.");
    } else if (strstr(text, "gets(")) {
    logic_update_assistant_message(app, "🚫 Danger: gets() is deprecated and unsafe. Use fgets() instead!");
    } else if (strstr(text, "strcpy(")) {
    logic_update_assistant_message(app, "💡 Tip: strcpy() can cause overflows. Check if strncpy() or strlcpy() is better suited.");
    } else if (strstr(text, "malloc") && !strstr(text, "free")) {
    logic_update_assistant_message(app, "🔍 Memory check: Ensure every malloc has a corresponding free() to prevent leaks.");
    } else if (strstr(text, "password =") || strstr(text, "SECRET_KEY")) {
    logic_update_assistant_message(app, "🛡️ Privacy Alert: Don't hardcode credentials. Use environment variables instead!");
    } else if (strstr(text, "+") && strstr(text, "malloc") && strstr(text, "sizeof")) {
    logic_update_assistant_message(app, "⚠️ Overflow alert: Check if your size calculation (malloc + sizeof) could overflow integer bounds.");
    current_mood = MOOD_ERROR; 
    } else  if (strstr(text, "PASSWORD")) {
        logic_update_assistant_message(app, "🔐 Security Notice: Detected a potential password leak via printf. Make sure to hash your passwords!");
    } else if (strstr(text, "printf(") && strstr(text, "password")) {
    logic_update_assistant_message(app, "🔐 Security Notice: Detected a potential password leak via printf. Ensure this is only for debug!");
    } else if (strstr(text, "chmod") && strstr(text, "0777")) {
    logic_update_assistant_message(app, "🚫 Warning: Setting file permissions to 0777 (world-writable) is generally unsafe.");
    } else if (strstr(text, "fopen(") && !strstr(text, "if (") && !strstr(text, "!= NULL")) {
    logic_update_assistant_message(app, "🔍 Logic check: Did you check if the pointer returned by fopen() is NULL?");
    } else if (strstr(text, "scanf(\"%s\"")) {
    logic_update_assistant_message(app, "🚫 Security Risk: Unbounded scanf() detected. Use %Ns (where N is the buffer size) to prevent overflows.");
    } else if (strstr(text, "int ") && !strstr(text, "=") && strstr(text, ";")) {
    logic_update_assistant_message(app, "💡 Coding Tip: Uninitialized variable detected. Always set local variables to a default value.");
    } 
    else {
        /* Clear out or minimize when writing standard boilerplate logic*/ 
        logic_update_assistant_message(app, "Looking for source code errors in real time."); 
    }
    
    g_free(text);
}

/*The Spawn function - heavy lifting PTY and forking the shell process*/
static void logic_spawn_terminal(AppState *app) {
    int master_fd = open("/dev/ptmx", O_RDWR | O_NOCTTY);
    fcntl(master_fd, F_SETFL, O_NONBLOCK); 
    grantpt(master_fd);
    unlockpt(master_fd);

    char *slave_name = ptsname(master_fd);
    app->master_fd = master_fd;

if (fork() == 0) {
    setsid();
    int slave_fd = open(slave_name, O_RDWR);
    dup2(slave_fd, 0); 
    dup2(slave_fd, 1); 
    dup2(slave_fd, 2);
    close(slave_fd);
    close(master_fd);
    
    setenv("TERM", "DUMB", 1); 
    execl("/bin/bash", "/bin/bash", "--norc", "--noprofile", "-i", (char *)NULL);
  
    exit(0);
}

    GIOChannel *chan = g_io_channel_unix_new(app->master_fd);
    g_io_channel_set_encoding(chan, NULL, NULL);
    g_io_add_watch(chan, G_IO_IN | G_IO_HUP, cb_terminal_out, app);
    g_io_channel_unref(chan);
} 

/* Other Editor tools */ 
/* Internal state structure for the calculator dialog */
 
typedef struct {
    GtkWidget *entry;
    GtkWidget *dialog;
    GtkTextBuffer *target_buffer;
    double current_value;
    double pending_value;
    char pending_op;
    gboolean new_digit;
} CalcState;

/* Destroy callback to safely free CalcState without freeing widget memory */ 
static void on_calc_dialog_destroy(GtkWidget *widget, gpointer user_data) {
    CalcState *state = (CalcState *)user_data;
    if (state) {
        g_free(state); /* Safe: frees the heap-allocated CalcState struct */ 
    }
}

/* Callback for calculator button clicks */ 
static void on_calc_button_clicked(GtkWidget *button, gpointer user_data) {
    CalcState *state = (CalcState *)user_data;
    const char *label = gtk_button_get_label(GTK_BUTTON(button));
    const char *current_text = gtk_entry_get_text(GTK_ENTRY(state->entry));

    /* Handle Digits and Decimal Point */ 
    if ((label[0] >= '0' && label[0] <= '9') || label[0] == '.') {
        if (state->new_digit || strcmp(current_text, "0") == 0) {
            gtk_entry_set_text(GTK_ENTRY(state->entry), label);
            state->new_digit = FALSE;
        } else {
            /* Prevent multiple decimal points */ 
            if (label[0] == '.' && strchr(current_text, '.') != NULL) return;
            
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "%s%s", current_text, label);
            gtk_entry_set_text(GTK_ENTRY(state->entry), buffer);
        }
        return;
    }

    /* Handle Clear (C) */ 
    if (strcmp(label, "C") == 0) {
        gtk_entry_set_text(GTK_ENTRY(state->entry), "0");
        state->current_value = 0;
        state->pending_value = 0;
        state->pending_op = '\0';
        state->new_digit = TRUE;
        return;
    }

    /* Process pending operation before storing a new one */ 
    double val = atof(current_text);
    if (state->pending_op != '\0' && !state->new_digit) {
        switch (state->pending_op) {
            case '+': state->pending_value += val; break;
            case '-': state->pending_value -= val; break;
            case '*': state->pending_value *= val; break;
            case '/': 
                if (val != 0) {
                    state->pending_value /= val;
                } else {
                    gtk_entry_set_text(GTK_ENTRY(state->entry), "Error");
                    state->pending_op = '\0';
                    state->new_digit = TRUE;
                    return;
                }
                break;
        }
        char result_str[64];
        snprintf(result_str, sizeof(result_str), "%g", state->pending_value);
        gtk_entry_set_text(GTK_ENTRY(state->entry), result_str);
    } else {
        state->pending_value = val;
    }

    /* Handle Operators (+, -, *, /) */ 
    if (strchr("+-*/", label[0]) != NULL) {
        state->pending_op = label[0];
        state->new_digit = TRUE;
    } 
    /* Handle Equals (=) */ 
    else if (strcmp(label, "=") == 0) {
        state->pending_op = '\0';
        state->new_digit = TRUE;
    }
}

/* Callback to paste the current display value into the target text buffer */ 
static void on_paste_result_clicked(GtkWidget *button, gpointer user_data) {
    CalcState *state = (CalcState *)user_data;
    if (state->target_buffer) {
        const char *text = gtk_entry_get_text(GTK_ENTRY(state->entry));
        gtk_text_buffer_insert_at_cursor(state->target_buffer, text, -1);
    }
    gtk_widget_destroy(state->dialog);
}

/*
 * Main Calculator Sub-Function
 * @param parent Optional parent window for modal behavior (can be NULL)
 * @param target_buffer Optional GtkTextBuffer to insert the result into (can be NULL)
 */
 
void show_calculator_dialog(GtkWindow *parent, GtkTextBuffer *target_buffer) {
    CalcState *state = g_new0(CalcState, 1);
    state->target_buffer = target_buffer;
    state->new_digit = TRUE;

    /* Validate parent window safely before creation to prevent Gtk-CRITICAL warnings */ 
    GtkWindow *valid_parent = (parent && GTK_IS_WINDOW(parent)) ? parent : NULL;

    /* Create dialog window */ 
    state->dialog = gtk_dialog_new_with_buttons(
        "Calculator", 
        valid_parent, 
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Close", GTK_RESPONSE_CLOSE,
        NULL
    );
    gtk_window_set_default_size(GTK_WINDOW(state->dialog), 220, 280);
    gtk_window_set_resizable(GTK_WINDOW(state->dialog), FALSE);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(state->dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    /* Display Entry */ 
    state->entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(state->entry), "0");
    gtk_entry_set_alignment(GTK_ENTRY(state->entry), 1.0); /* Right align */
    gtk_editable_set_editable(GTK_EDITABLE(state->entry), FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), state->entry, FALSE, FALSE, 0);

    /* Button Grid Layout */ 
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 4);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 4);
    gtk_grid_set_row_homogeneous(GTK_GRID(grid), TRUE);
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), grid, TRUE, TRUE, 0);

    /* Calculator buttons layout */ 
    const char *buttons[5][4] = {
        {"C", "(", ")", "/"},
        {"7", "8", "9", "*"},
        {"4", "5", "6", "-"},
        {"1", "2", "3", "+"},
        {"0", ".", "=", "="}
    };

    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 4; col++) {
            /* Skip the second '=' grid space so we can span it across two columns */ 
            if (row == 4 && col == 3) continue;

            GtkWidget *btn = gtk_button_new_with_label(buttons[row][col]);
            g_signal_connect(btn, "clicked", G_CALLBACK(on_calc_button_clicked), state);

            if (row == 4 && col == 2) {
                /* Make '=' span 2 columns */ 
                gtk_grid_attach(GTK_GRID(grid), btn, col, row, 2, 1);
            } else {
                gtk_grid_attach(GTK_GRID(grid), btn, col, row, 1, 1);
            }
        }
    }

    /* Optional "Insert into Document" button if buffer was provided */ 
    if (target_buffer != NULL) {
        GtkWidget *paste_btn = gtk_button_new_with_label("Insert Result");
        g_signal_connect(paste_btn, "clicked", G_CALLBACK(on_paste_result_clicked), state);
        gtk_box_pack_start(GTK_BOX(vbox), paste_btn, FALSE, FALSE, 0);
    }

    /* Connect custom destroy callback to clean up memory safely when closed */ 
    g_signal_connect(state->dialog, "destroy", G_CALLBACK(on_calc_dialog_destroy), state);
    g_signal_connect_swapped(state->dialog, "response", G_CALLBACK(gtk_widget_destroy), state->dialog);

    gtk_widget_show_all(state->dialog);
}

/* Safe callback function for the menu item activation */ 
static void on_calculator_menu_activate(GtkMenuItem *menuitem, gpointer user_data) {
    GtkWindow *parent_window = NULL;
    GtkTextBuffer *target_buffer = NULL;

    if (user_data != NULL) {
        if (GTK_IS_WINDOW(user_data)) {
            parent_window = GTK_WINDOW(user_data);
        } else {
            /* Safely extract pointers if user_data is your main AppState struct pointer */ 
            AppState *app = (AppState *)user_data;
            if (app->window && GTK_IS_WINDOW(app->window)) {
                parent_window = GTK_WINDOW(app->window);
            }
            target_buffer = app->editor_buffer;
        }
    }

    show_calculator_dialog(parent_window, target_buffer);
}

/* =========================================================================
 * DIRECTORY MANAGEMENT CALLBACKS
 * ========================================================================= */

static void cb_on_create_directory(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Create Directory",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Create", GTK_RESPONSE_ACCEPT,
        "_Cancel", GTK_RESPONSE_REJECT,
        NULL);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content_area), 12);
    gtk_box_set_spacing(GTK_BOX(content_area), 8);

    GtkWidget *label = gtk_label_new("Enter directory path or name:");
    gtk_box_pack_start(GTK_BOX(content_area), label, FALSE, FALSE, 0);

    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "e.g. my_folder or /path/to/dir");
    gtk_box_pack_start(GTK_BOX(content_area), entry, FALSE, FALSE, 0);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        const char *dir_path = gtk_entry_get_text(GTK_ENTRY(entry));
        if (dir_path && strlen(dir_path) > 0) {
            if (g_mkdir_with_parents(dir_path, 0755) == 0) {
                char *msg = g_strdup_printf("Created directory: %s", dir_path);
                ui_set_status_alert(app, msg, FALSE);
                g_free(msg);
                logic_refresh_function_tree(app);
            } else {
                char *err_msg = g_strdup_printf("Failed to create directory: %s", dir_path);
                ui_set_status_alert(app, err_msg, TRUE);
                g_free(err_msg);
            }
        }
    }

    gtk_widget_destroy(dialog);
}

/* =========================================================================
 * TOOLS MENU CALLBACKS & TOGGLES
 * ========================================================================= */

/* Toggle visibility of the entire Recent Files & Program Functions Sidebar Container */
static void cb_on_toggle_sidebar_panel(GtkCheckMenuItem *check_item, gpointer user_data) {
    AppState *app = (AppState *)user_data;
    gboolean is_visible = gtk_check_menu_item_get_active(check_item);

    if (app && app->sidebar_notebook) {
        if (is_visible) {
            gtk_widget_show(app->sidebar_notebook);
            ui_set_status_alert(app, "Sidebar Panel Visible", FALSE);
            logic_update_assistant_message(app, "Sidebar is now visible and ready."); 
        } else {
            gtk_widget_hide(app->sidebar_notebook);
            ui_set_status_alert(app, "Sidebar Panel Hidden", FALSE);
            logic_update_assistant_message(app, "Sidebar panel for editor hidden.."); 
        }
    }
}

/* Toggle visibility of the Bash Terminal / Console Panel */
static void cb_on_toggle_console_panel(GtkCheckMenuItem *check_item, gpointer user_data) {
    AppState *app = (AppState *)user_data;
    gboolean is_visible = gtk_check_menu_item_get_active(check_item);

    if (app && app->console_view) {
        /* GTK TextViews are often wrapped in a GtkScrolledWindow parent */
        GtkWidget *target_widget = gtk_widget_get_parent(app->console_view);
        if (!target_widget) {
            target_widget = app->console_view;
        }

        if (is_visible) {
            gtk_widget_show(target_widget);
            ui_set_status_alert(app, "Bash Console Visible", FALSE);
            logic_update_assistant_message(app, "Bash console visible"); 
        } else {
            gtk_widget_hide(target_widget);
            ui_set_status_alert(app, "Bash Console Hidden", FALSE);
            logic_update_assistant_message(app, "Bash console hidden for now."); 
        }
    }
}

/* Callback to toggle the Compile Button visibility */
static void cb_on_toggle_compile_button(GtkCheckMenuItem *check_item, gpointer user_data) {
    AppState *app = (AppState *)user_data;
    gboolean active = gtk_check_menu_item_get_active(check_item);

    /* Toggle your compile button widget using the reference in app */
    if (app && app->btn_box) {
        gtk_widget_set_visible(app->btn_box, active);
    }
}

/* Callback to toggle the editor visibility and expand the browser view */
static void cb_on_toggle_editor_focus(GtkCheckMenuItem *check_item, gpointer user_data) {
    AppState *app = (AppState *)user_data;
    if (!app || !app->editor_view || !app->browser_view) return;

    gboolean hide_editor = gtk_check_menu_item_get_active(check_item);

    if (hide_editor) {
        /* Hide the editor view */
        gtk_widget_hide(app->editor_view);
        GtkWidget *parent = gtk_widget_get_parent(app->editor_view);
        if (GTK_IS_SCROLLED_WINDOW(parent)) {
            gtk_widget_hide(parent);
        }

        /* Ensure browser view expands to fill available space */
        gtk_widget_set_hexpand(app->browser_view, TRUE);
        gtk_widget_set_vexpand(app->browser_view, TRUE);

        ui_set_status_alert(app, "Focus Mode: Editor hidden", FALSE);
        logic_update_assistant_message(app, "Browser mode ACTIVE! Editor hidden..");
    } else {
        /* Show the editor view */
        GtkWidget *parent = gtk_widget_get_parent(app->editor_view);
        if (GTK_IS_SCROLLED_WINDOW(parent)) {
            gtk_widget_show(parent);
        }
        gtk_widget_show(app->editor_view);

        ui_set_status_alert(app, "Focus Mode: Editor restored", FALSE);
        logic_update_assistant_message(app, "Editor restored - Looking for source code errors in real time."); 
    }
}

/* IMBEDED AUDIO IDEA PROTOTYPE,
 * Cairo draw callback for rendering spectrum frequency bars */
static void cb_on_toggle_audio_mode(GtkCheckMenuItem *item, gpointer user_data) {
AppState *app = (AppState *)user_data;

    /* weird Gtk-CRITICAL assertion explot bug 
     * effects toggle switch wack-a-doo figet spiner users */
    if (!app) return;
    if (!app || !app->sidebar_notebook) return;
    /* Safety Guard: Ensure app and notebook exist and are valid widgets */
    if (!app || !app->sidebar_notebook || !GTK_IS_WIDGET(app->sidebar_notebook)) {
        g_warning("DEBUG: Sidebar notebook is invalid or uninitialized.");
        return;
    }
    
    gboolean active = gtk_check_menu_item_get_active(item);
    
    static GtkWidget *audio_tab_content = NULL;
    if (active) {
        if (!audio_tab_content) {
            /* Create WebKitWebView or Container for Audio Stream */
            audio_tab_content = webkit_web_view_new();
            webkit_web_view_load_uri(WEBKIT_WEB_VIEW(audio_tab_content), "https://somafm.com/player/#/");
            gtk_widget_show(audio_tab_content);
        }


        /* Append to sidebar_notebook */
        GtkWidget *label = gtk_label_new("Audio Stream");
        gint page_num = gtk_notebook_append_page(GTK_NOTEBOOK(app->sidebar_notebook), audio_tab_content, label);
	    
        gtk_widget_show_all(app->sidebar_notebook);
        gtk_notebook_set_current_page(GTK_NOTEBOOK(app->sidebar_notebook), page_num);
        
    } else {
        if (audio_tab_content) {
            gint page_num = gtk_notebook_page_num(GTK_NOTEBOOK(app->sidebar_notebook), audio_tab_content);
            if (page_num != -1) {
                gtk_notebook_remove_page(GTK_NOTEBOOK(app->sidebar_notebook), page_num);
            }
        }
    }

    if (active) {
        /* Open stream player if webview isn't initialized */
        if (!app->embedded_audio_view) {
            ui_open_audio_player(app, "https://somafm.com/player/#/gslive"); 
            if (app->audio_popup && GTK_IS_WIDGET(app->audio_popup)) {
                gtk_widget_destroy(app->audio_popup);
                app->audio_popup = NULL;
               }
        }
        
        /* Focus/Show sidebar or notebook tab containing audio stream */
        if (app->sidebar_notebook) {
            gtk_widget_show(app->sidebar_notebook);
        }
        ui_set_status_alert(app, "Audio Streaming Mode: Enabled", FALSE);
    } else {
        if (app->embedded_audio_view) {
            webkit_web_view_stop_loading(WEBKIT_WEB_VIEW(app->embedded_audio_view));
            gint page_num = gtk_notebook_page_num(
                GTK_NOTEBOOK(app->sidebar_notebook),
                GTK_WIDGET(app->embedded_audio_view)
            );
           if (page_num != -1) {
                /* This automatically destroys the widget when removed */
                gtk_notebook_remove_page(GTK_NOTEBOOK(app->sidebar_notebook), page_num);
            }

            /* Nullify the reference so it can be re-created on next toggle */
            app->embedded_audio_view = NULL; 
        }
        ui_set_status_alert(app, "Audio Streaming Mode: Disabled", FALSE);
        logic_update_assistant_message(app, "Audio Stream Embedded is now disabled.");
    }
  
    /* Locate the child widget inside the "Audio Stream" tab (Page Index 2) */
    GtkWidget *stream_tab_child = gtk_notebook_get_nth_page(GTK_NOTEBOOK(app->sidebar_notebook), 2);

    if (stream_tab_child) {
        if (active) {
            gtk_widget_show(stream_tab_child);
            logic_update_assistant_message(app, "Audio Stream Embedded enabled."); 
        } else {
            gtk_widget_hide(stream_tab_child);
            logic_update_assistant_message(app, "Audio Stream Embedded is now disabled."); 
        }
    }  
    
    /* Disable the menu item so it cannot be used again clean exit without errors like for example,
     * gcc -> output = (crystalcat:1388): Gtk-CRITICAL **: 11:57:01.225: gtk_notebook_append_page: assertion 'GTK_IS_WIDGET (child)' failed
     * */
     gtk_widget_set_sensitive(GTK_WIDGET(item), FALSE);
}

static void cb_on_clear_browsing_data(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;
    WebKitWebsiteDataManager *manager = webkit_web_view_get_website_data_manager(WEBKIT_WEB_VIEW(app->browser_view));
    
    webkit_website_data_manager_clear(
        manager,
        WEBKIT_WEBSITE_DATA_COOKIES | WEBKIT_WEBSITE_DATA_DISK_CACHE | 
        WEBKIT_WEBSITE_DATA_MEMORY_CACHE | WEBKIT_WEBSITE_DATA_LOCAL_STORAGE,
        0, NULL, NULL, NULL);
    trigger_cat_animation(app);    
    ui_set_status_alert(app, "Browsing data cleared successfully.", FALSE);
    logic_update_assistant_message(app, "Browsing data cleared successfully..");
}

/* Spoof the user agent of our browser */
static void cb_on_ua_selected(GtkRadioMenuItem *item, gpointer user_data) {
    if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item))) {
        return; /* Ignore deselection signal from the previous radio choice */
    }

    AppState *app = (AppState *)user_data;
    const char *ua_string = (const char *)g_object_get_data(G_OBJECT(item), "ua-string");

    if (!app || !app->browser_view) return;

    WebKitSettings *settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(app->browser_view));

    if (ua_string && strlen(ua_string) > 0) {
        webkit_settings_set_user_agent(settings, ua_string);
        trigger_cat_animation(app);
        ui_set_status_alert(app, "User-Agent set to custom spoof profile.", FALSE);
        gchar *msg = g_strdup_printf("User-agent custom spoof profile: %s", ua_string); 
        logic_update_assistant_message(app, msg); 
    } else {
        webkit_settings_set_user_agent(settings, NULL); /* Revert to WebKit default */
        ui_set_status_alert(app, "User-Agent reset to WebKit default.", FALSE);
        logic_update_assistant_message(app, "User-agent reset to WebKit default."); 
    }

    /* Reload current page to send new headers */
    webkit_web_view_reload(WEBKIT_WEB_VIEW(app->browser_view));
}

/* Reject All Cookies Kill'em all ! */
static void on_reject_all_cookies_activate(GtkMenuItem *menuitem, gpointer user_data) {
    AppState *app = (AppState *)user_data;
    if (!app || !app->browser_view) return;

    WebKitWebContext *context = webkit_web_view_get_context(WEBKIT_WEB_VIEW(app->browser_view));
    WebKitCookieManager *cookie_mgr = webkit_web_context_get_cookie_manager(context);

    /* Enforce strict rejection of all cookie storage requests */
    webkit_cookie_manager_set_accept_policy(cookie_mgr, WEBKIT_COOKIE_POLICY_ACCEPT_NEVER);

    /* Purge any existing cookies currently stored in the session memory/disk */
    WebKitWebsiteDataManager *data_mgr = webkit_web_context_get_website_data_manager(context);
    webkit_website_data_manager_clear(data_mgr, WEBKIT_WEBSITE_DATA_COOKIES, 0, NULL, NULL, NULL);
    trigger_cat_animation(app);
    ui_set_status_alert(app, "Cookie Policy: REJECT ALL (Session cleared)", FALSE);
    logic_update_assistant_message(app, "🚫 Cookie Policy set to REJECT ALL. All existing cookies purged.");
}

/* Alter / Edit Selected Cookie Value (Hack the tracking cookie pineapple style)*/
static void cb_on_save_altered_cookie(GtkWidget *widget, gpointer user_data) {
    CookieEditContext *ctx = (CookieEditContext *)user_data;
    if (!ctx) return;

    const char *name = gtk_entry_get_text(GTK_ENTRY(ctx->name_entry));
    const char *value = gtk_entry_get_text(GTK_ENTRY(ctx->value_entry));
    const char *domain = gtk_entry_get_text(GTK_ENTRY(ctx->domain_entry));
    const char *path = gtk_entry_get_text(GTK_ENTRY(ctx->path_entry));

    /* Construct an updated SoupCookie instance */
    SoupCookie *new_cookie = soup_cookie_new(name, value, domain, path, -1);
    
    if (new_cookie && ctx->manager) {
        webkit_cookie_manager_add_cookie(ctx->manager, new_cookie, NULL, NULL, NULL);
        soup_cookie_free(new_cookie);
    }

    if (ctx->dialog) {
        gtk_widget_destroy(ctx->dialog);
    }
    g_free(ctx);
}

static void on_menu_alter_cookie_activate(GtkMenuItem *menuitem, gpointer user_data) {
    AppState *app = (AppState *)user_data;
    if (!app) return;

    /* Open the Cookie Inspector/Manager dialog */
    ui_show_cookie_dialog(app);
    
    ui_set_status_alert(app, "Select a cookie from the manager to alter its value.", FALSE);
}

/* =========================================================================
 * COOKIE ALTERATION DIALOG IMPLEMENTATION
 * ========================================================================= */

static void on_alter_cookie_activated(GtkWidget *widget, gpointer user_data) {
    AppState *app = (AppState *)user_data;
    if (!app || !app->browser_view) return;

    WebKitWebContext *context = webkit_web_view_get_context(WEBKIT_WEB_VIEW(app->browser_view));
    WebKitCookieManager *manager = webkit_web_context_get_cookie_manager(context);

    GtkWidget *edit_dialog = gtk_dialog_new_with_buttons("Alter / Inject Cookie",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Save & Inject", GTK_RESPONSE_ACCEPT,
        "_Cancel", GTK_RESPONSE_REJECT,
        NULL);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(edit_dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content_area), 15);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_container_add(GTK_CONTAINER(content_area), grid);

    /* Retrieve current page host as dynamic default domain */
    const char *current_uri = webkit_web_view_get_uri(WEBKIT_WEB_VIEW(app->browser_view));
    char default_domain[256] = "";
    if (current_uri) {
        SoupURI *uri = soup_uri_new(current_uri);
        if (uri) {
            if (uri->host) {
                snprintf(default_domain, sizeof(default_domain), "%s", uri->host);
            }
            soup_uri_free(uri);
        }
    }

    GtkWidget *name_entry = gtk_entry_new();
    GtkWidget *value_entry = gtk_entry_new();
    GtkWidget *domain_entry = gtk_entry_new();
    GtkWidget *path_entry = gtk_entry_new();

    gtk_entry_set_text(GTK_ENTRY(domain_entry), default_domain);
    gtk_entry_set_text(GTK_ENTRY(path_entry), "/");

    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Cookie Name:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), name_entry, 1, 0, 1, 1);

    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Cookie Value:"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), value_entry, 1, 1, 1, 1);

    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Domain:"), 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), domain_entry, 1, 2, 1, 1);

    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Path:"), 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), path_entry, 1, 3, 1, 1);

    gtk_widget_show_all(edit_dialog);

    if (gtk_dialog_run(GTK_DIALOG(edit_dialog)) == GTK_RESPONSE_ACCEPT) {
        CookieEditContext *ctx = g_new0(CookieEditContext, 1);
        ctx->manager = manager;
        ctx->name_entry = name_entry;
        ctx->value_entry = value_entry;
        ctx->domain_entry = domain_entry;
        ctx->path_entry = path_entry;
        ctx->dialog = edit_dialog;

        cb_on_save_altered_cookie(NULL, ctx);
    } else {
        gtk_widget_destroy(edit_dialog);
    }
}

/* Spoof cookies with random tracking strings */
/* --- Utility Function: Generate Random Noise Strings --- */
static char* generate_random_string(size_t length) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-";
    char *random_str = g_malloc(length + 1);
    
    for (size_t n = 0; n < length; n++) {
        int key = rand() % (int)(sizeof(charset) - 1);
        random_str[n] = charset[key];
    }
    random_str[length] = '\0';
    return random_str;
}

/* --- Core Function: Inject Random Noise Cookies into WebKit --- */
static void inject_spoofed_cookie_noise(WebKitCookieManager *cookie_manager, const char *domain) {
    if (!cookie_manager || !domain) return;

    /* Common tracking/session cookie keys to spoof */
    const char *cookie_names[] = {
        "_ga", "_gid", "_fbp", "_session_id", "cf_clearance", 
        "visitor_id", "sess_tok", "auth_tracker", "user_noise"
    };
    int num_cookies = sizeof(cookie_names) / sizeof(cookie_names[0]);

    for (int i = 0; i < num_cookies; i++) {
        char *random_value = generate_random_string(16 + (rand() % 16));
        
        /* Construct a SoupCookie with randomized data */
        SoupCookie *cookie = soup_cookie_new(
            cookie_names[i],
            random_value,
            domain,
            "/",
            3600 /* Expires in 1 hour */
        );

        if (cookie) {
            /* Add the noise cookie to the WebKit Cookie Manager */
            webkit_cookie_manager_add_cookie(
                cookie_manager, 
                cookie, 
                NULL, 
                NULL, 
                NULL
            );
            soup_cookie_free(cookie);
        }
        g_free(random_value);
    }
}

/* --- Callback: Fires when the Toggle Switch / CheckMenuItem changes state --- */
static void on_toggle_cookie_spoofing(GtkCheckMenuItem *check_menu_item, gpointer user_data) {
    AppState *app = (AppState *)user_data;
    g_cookie_spoofing_active = gtk_check_menu_item_get_active(check_menu_item);

    if (!app || !app->browser_view) return;
    
    
    WebKitWebView *web_view = WEBKIT_WEB_VIEW(app->browser_view);
    WebKitWebContext *context = webkit_web_view_get_context(web_view);
    WebKitCookieManager *cookie_manager = webkit_web_context_get_cookie_manager(context);
    WebKitWebsiteDataManager *data_manager = webkit_web_context_get_website_data_manager(context);

    if (g_cookie_spoofing_active) {
        /* Step 1: Wipe existing legitimate cookies */
        webkit_website_data_manager_clear(
            data_manager, 
            WEBKIT_WEBSITE_DATA_COOKIES, 
            0, 
            NULL, 
            NULL, 
            NULL
        );

        /* Step 2: Determine current host or fall back to wildcard spoof targets */
        const char *current_uri = webkit_web_view_get_uri(web_view);
        char *hostname = NULL;

        if (current_uri) {
            SoupURI *soup_uri = soup_uri_new(current_uri);
            if (soup_uri && soup_uri->host) {
                hostname = g_strdup(soup_uri->host);
            }
            if (soup_uri) soup_uri_free(soup_uri);
        }

        /* Inject into current domain and standard cross-browser trackers */
        if (hostname) {
            inject_spoofed_cookie_noise(cookie_manager, hostname);
            g_free(hostname);
        }
        inject_spoofed_cookie_noise(cookie_manager, ".doubleclick.net");
        inject_spoofed_cookie_noise(cookie_manager, ".google.com");
        /*  3rd-party cross-site cookies and common tracking search */
        webkit_cookie_manager_set_accept_policy(
            cookie_manager, 
            WEBKIT_COOKIE_POLICY_ACCEPT_NO_THIRD_PARTY
        );
        webkit_website_data_manager_set_itp_enabled(data_manager, TRUE);
        if (current_uri) {
            GUri *guri = g_uri_parse(current_uri, G_URI_FLAGS_NONE, NULL);
            if (guri) {
                const char *host = g_uri_get_host(guri);
                if (host) hostname = g_strdup(host);
                g_uri_unref(guri);
            }
        }
        if (hostname) {
            inject_spoofed_cookie_noise(cookie_manager, hostname);
            g_free(hostname);
        }

        static const char *tracker_domains[] = {
            ".doubleclick.net",
            ".google-analytics.com",
            ".facebook.com",
            ".adnxs.com",
            ".criteo.com",
            NULL
        };

        for (int i = 0; tracker_domains[i] != NULL; i++) {
            inject_spoofed_cookie_noise(cookie_manager, tracker_domains[i]);
        }
        
        ui_set_status_alert(app, "[PRIVACY] Cross-Browser Cookie Spoofing: ACTIVE (Noise Injected)", FALSE);
        trigger_cat_animation(app); 
        logic_update_assistant_message(app, "🛡️ Cookie spoofing enabled! Authentic tracking data replaced with random noise.");
    } else {
		/* Restore standard cookie acceptance and disable ITP */
        webkit_cookie_manager_set_accept_policy(
            cookie_manager, 
            WEBKIT_COOKIE_POLICY_ACCEPT_ALWAYS
        );
        webkit_website_data_manager_set_itp_enabled(data_manager, FALSE);
        ui_set_status_alert(app, "[PRIVACY] Cross-Browser Cookie Spoofing: DISABLED", FALSE);
        logic_update_assistant_message(app, "⚠️ Cookie spoofing disabled. Standard cookie persistence restored.");
    }


    /* Reload view to apply new cookie context */
    webkit_web_view_reload(web_view);
}

/* --- UI Initialization Helper: Attach to Menu Construction --- */
GtkWidget* ui_create_cookie_spoof_menu_item(AppState *app) {
    GtkWidget *spoof_item = gtk_check_menu_item_new_with_label("Spoof Cookies with Random Noise");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(spoof_item), FALSE);

    g_signal_connect(
        G_OBJECT(spoof_item), 
        "toggled", 
        G_CALLBACK(on_toggle_cookie_spoofing), 
        app
    );

    return spoof_item;
}

/*
 *        Sudorandom Location generator function without proxy or vpn,
 *        you may have to configure this to your time zone.   
 * */
 
static double get_random_in_range(double min, double max) {
	double scale = (double)arc4random() / (double)UINT32_MAX;
	return min + scale * (max - min);  
}

static void apply_random_geolocation_script(WebKitWebView *web_view) {
    if (!web_view || !WEBKIT_IS_WEB_VIEW(web_view)) {
        g_warning("apply_random_geolocation_script: Invalid WebKitWebView instance");
        return;
    }

    double min_lat = 31.33;
    double max_lat = 49.00;
    double min_lng = -114.05;
    double max_lng = -104.05;

    /* Uses arc4random() / custom C-native PRNG implementation */
    double rand_lat = get_random_in_range(min_lat, max_lat);
    double rand_lng = get_random_in_range(min_lng, max_lng);

    gchar *js_snippet = g_strdup_printf(
        "(function() {"
        "  const funnyPlaces = ["
        "    '📍 Coordinates: Somewhere in the Bermuda Triangle 🛸',"
        "    '📍 Location: Area 51 (Shhh, strictly classified) 👽',"
        "    '📍 Current location: Bigfoots home address️',"
        "    '📍 Geolocation: Batcave, Gotham City 🦇',"
        "    '📍 Location: Mars Rover Curiosity 🚀',"
        "    '📍 Location: On your mothers box bro!'"
        "  ];"
        "  const funnyMsg = funnyPlaces[Math.floor(Math.random() * funnyPlaces.length)];"
        ""
        "  /* Mock location data */"
        "  const mockCoords = { latitude: %.6f, longitude: %.6f, accuracy: 15.0 };"
        "  const mockPos = { coords: mockCoords, timestamp: Date.now() };"
        ""
        "  /* Spoof Geolocation API methods */"
        "  navigator.geolocation.getCurrentPosition = function(success, error, options) {"
        "    console.log('[Geolocation Spoofed]:', funnyMsg);"
        "    setTimeout(() => success(mockPos), 10);"
        "  };"
        "  navigator.geolocation.watchPosition = function(success, error, options) {"
        "    console.log('[Geolocation Watch Spoofed]:', funnyMsg);"
        "    setTimeout(() => success(mockPos), 10);"
        "    return 1;"
        "  };"
        ""
        "  /* Inject floating funny message UI overlay onto the page */"
        "  window.addEventListener('DOMContentLoaded', () => {"
        "    const badge = document.createElement('div');"
        "    badge.textContent = funnyMsg + ' (Lat: ' + %.6f + ', Lng: ' + %.6f + ')';"
        "    badge.style.cssText = 'position:fixed;top:10px;right:10px;z-index:999999;"
        "                           background:rgba(0,0,0,0.85);color:#00ffcc;"
        "                           padding:8px 14px;border-radius:20px;"
        "                           font-family:sans-serif;font-size:12px;font-weight:bold;"
        "                           box-shadow:0 4px 12px rgba(0,0,0,0.3);pointer-events:none;';"
        "    document.body.appendChild(badge);"
        "  });"
        "})();",
        rand_lat, rand_lng, rand_lat, rand_lng
    );

    WebKitUserContentManager *ucm = webkit_web_view_get_user_content_manager(web_view);

    /* Clear previous user scripts so they don't stack on page reloads */
    webkit_user_content_manager_remove_all_scripts(ucm);

    WebKitUserScript *script = webkit_user_script_new(
        js_snippet,
        WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
        WEBKIT_USER_CONTENT_INJECT_TOP_FRAME,
        NULL, NULL
    );

    webkit_user_content_manager_add_script(ucm, script);

    webkit_user_script_unref(script);
    g_free(js_snippet);
}

static gboolean on_permission_request(WebKitWebView *web_view,
                                       WebKitPermissionRequest *request,
                                       gpointer user_data) {
    if (WEBKIT_IS_GEOLOCATION_PERMISSION_REQUEST(request)) {
        webkit_permission_request_allow(request);
        return TRUE; /* Handled */
    }
    
    AppState *app = (AppState *)user_data;

    /* Check if the page is attempting to access the webcam or microphone */
    if (WEBKIT_IS_USER_MEDIA_PERMISSION_REQUEST(request)) {
        /* Alert the user via your assistant status bar */
        logic_update_assistant_message(app, "⚠️ Warning: Page requested webcam/microphone access!");
        
        /* Create an interactive modal warning dialog box */
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(app->window),
            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_WARNING,
            GTK_BUTTONS_NONE,
            "Hardware Access Request"
        );
        
        gtk_message_dialog_format_secondary_text(
            GTK_MESSAGE_DIALOG(dialog),
            "The active webpage is attempting to access your webcam and/or microphone.\n\n"
            "Do you wish to allow or deny this hardware access?"
        );
        
        /* Add response buttons */
        gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                               "_Deny", GTK_RESPONSE_REJECT,
                               "_Allow", GTK_RESPONSE_ACCEPT,
                               NULL);
        
        /* Run the dialog and wait for user response */
        gint response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        
        /* Handle user choice */
        if (response == GTK_RESPONSE_ACCEPT) {
            webkit_permission_request_allow(request);
            logic_update_assistant_message(app, "✅ Webcam/microphone access granted by user.");
        } else {
            webkit_permission_request_deny(request);
            logic_update_assistant_message(app, "🔒 Webcam/microphone access blocked by user.");
        }
        
        return TRUE; /* Handled */
    }
    
    return FALSE; /* Pass other permission requests to default handlers */
}

static void on_load_changed_refresh_geo(WebKitWebView *web_view, 
                                        WebKitLoadEvent load_event, 
                                        gpointer user_data) {
    if (load_event == WEBKIT_LOAD_STARTED) {
        apply_random_geolocation_script(web_view);
    }
}

void setup_browser_geolocation_spoofing(WebKitWebView *web_view) {
    if (!web_view || !WEBKIT_IS_WEB_VIEW(web_view)) return;

    /* Auto-allow location requests */
    g_signal_connect(web_view, "permission-request", 
                     G_CALLBACK(on_permission_request), NULL);

    /* Apply initial mock coordinates */
    apply_random_geolocation_script(web_view);

    /*  Re-randomize location each time a new load starts */
    g_signal_connect(web_view, "load-changed", 
                     G_CALLBACK(on_load_changed_refresh_geo), NULL);
}

static void on_randomize_location_activate(GtkMenuItem *menuitem, gpointer user_data) {
    WebKitWebView *web_view = NULL;
    AppState *app = (AppState *)user_data;
    
    /* Handle both direct WebKitWebView* and AppState* user_data contexts */
    if (WEBKIT_IS_WEB_VIEW(user_data)) {
        web_view = WEBKIT_WEB_VIEW(user_data);
    } else if (user_data != NULL) {
        /* Assuming AppState contains browser_view as a GtkWidget* */
        AppState *app = (AppState *)user_data;
        if (app->browser_view && WEBKIT_IS_WEB_VIEW(app->browser_view)) {
            web_view = WEBKIT_WEB_VIEW(app->browser_view);
        }
    }

    if (web_view) {
        apply_random_geolocation_script(web_view);
        webkit_web_view_reload(web_view);
        trigger_cat_animation(app); 
        logic_update_assistant_message(app, "Local geolocation of device is now randomized.");
    } else {
        g_warning("on_randomize_location_activate: Could not retrieve valid WebKitWebView");
    }
}

/*==========================================================================
 *  Block ad-spam within a website idea LAGACY CODE ENGINE 
 *          Well men let's stomp some ads today!
 * =========================================================================
 * */
 
char* build_css_hide_statement(sqlite3 *db) {
    sqlite3_stmt *stmt;
    /* Only query rows explicitly marked for CSS element hiding */
    const char *sql = "SELECT selector FROM ad_filters WHERE action_type = 'css-display-none' AND enabled = 1 AND selector IS NOT NULL;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return NULL;
    }

    GString *css_rules = g_string_new("");
    gboolean first = TRUE;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *selector = (const char*)sqlite3_column_text(stmt, 0);

        if (!first) {
            g_string_append(css_rules, ", ");
        }
        g_string_append(css_rules, selector);
        first = FALSE;
    }

    sqlite3_finalize(stmt);

    if (css_rules->len == 0) {
        g_string_free(css_rules, TRUE);
        return NULL;
    }

    g_string_append(css_rules, " { display: none !important; }");
    return g_string_free(css_rules, FALSE);
}

void apply_css_element_blocking(WebKitWebView *web_view, sqlite3 *db) {
    WebKitUserContentManager *manager = webkit_web_view_get_user_content_manager(web_view);

    /* Clear any previously injected user scripts/styles */
    webkit_user_content_manager_remove_all_style_sheets(manager);

    char *css_string = build_css_hide_statement(db);
    if (!css_string) return;

    /* Create a WebKit User Style Sheet */
    WebKitUserStyleSheet *stylesheet = webkit_user_style_sheet_new(
        css_string,
        WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
        WEBKIT_USER_STYLE_LEVEL_USER,
        NULL, /* Inject for all domains */
        NULL
    );

    /* Add the stylesheet directly to the page rendering engine */
    webkit_user_content_manager_add_style_sheet(manager, stylesheet);

    /* Clean up memory */
    webkit_user_style_sheet_unref(stylesheet);
    g_free(css_string);
    
    /* CSS selectors targeting structural ad containers and wrappers */
    const char *ad_css_rules = 
        ".ad-container, .ad-wrapper, .ad-box, .ad-slot, .ad-banner, "
        "[id*='google_ads'], [id*='taboola'], [id*='outbrain'], "
        "div[class*='-ad-'], div[class*='_ad_'], iframe[src*='doubleclick.net'], "
        "ins.adsbygoogle, .sponsored-post, .native-ad-unit "
        "{ display: none !important; visibility: hidden !important; height: 0 !important; min-height: 0 !important; }";

    WebKitUserStyleSheet *style_sheet = webkit_user_style_sheet_new(
        ad_css_rules,
        WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
        WEBKIT_USER_STYLE_LEVEL_USER,
        NULL, NULL
    );

    webkit_user_content_manager_add_style_sheet(manager, style_sheet);
    webkit_user_style_sheet_unref(style_sheet);
  
    webkit_user_style_sheet_unref(style_sheet);
    
    const char *js_cleanup = 
        "document.addEventListener('DOMContentLoaded', function() {"
        "  const observer = new MutationObserver(() => {"
        "    document.querySelectorAll('.ad-container, ins.adsbygoogle, [id*=\"google_ads_frame\"]').forEach(el => {"
        "      if (el.offsetHeight > 0 || el.offsetWidth > 0) {"
        "        el.style.setProperty('display', 'none', 'important');"
        "      }"
        "    });"
        "  });"
        "  observer.observe(document.body, { childList: true, subtree: true });"
        "});";

    WebKitUserScript *script = webkit_user_script_new(
        js_cleanup,
        WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
        WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_END,
        NULL, NULL
    );

    webkit_user_content_manager_add_script(manager, script);
    webkit_user_script_unref(script);

}

void block_css_element(sqlite3 *db, WebKitWebView *web_view, const char *selector) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT OR IGNORE INTO ad_filters (selector) VALUES (?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, selector, -1, SQLITE_STATIC);
        
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            /* Refresh stylesheet in WebKit directly from DB */
            apply_css_element_blocking(web_view, db);
        }
        sqlite3_finalize(stmt);
    }
}

/* ==============================================================================
 * AD BLOCKING & CONTENT FILTERING ENGINE HOOKS ON THE FLY (NEW STOMPYER VERSION) 
 * ============================================================================== */

static void on_filter_saved_2(WebKitUserContentFilterStore *store, GAsyncResult *res, gpointer user_data) {
    GError *error = NULL;
    WebKitUserContentFilter *filter = webkit_user_content_filter_store_save_finish(store, res, &error);
    
    if (filter) {
        WebKitUserContentManager *manager = WEBKIT_USER_CONTENT_MANAGER(user_data);
        /* Ensure existing filter rules with the same identifier are removed first */
        webkit_user_content_manager_remove_filter(manager, filter);
        webkit_user_content_manager_add_filter(manager, filter);
        webkit_user_content_filter_unref(filter);
        g_print("[AdBlock] Content filter rules successfully applied to WebKit Engine.\n");
    } else {
        if (error) {
            g_printerr("[AdBlock Error] Failed to save filter store: %s\n", error->message);
            g_error_free(error);
        }
    }
}

static char* logic_get_filters_as_json_2(sqlite3 *db) {
    if (!db) return NULL;
    
    sqlite3_stmt *stmt;
    /* Constructs valid WebKit Content Blocker JSON JSON array directly from SQLite database 
    // It will block all content always the html response is as follows, "The URL was blocked by a content blocker"
    const char *sql = 
        "SELECT json_group_array("
        "  CASE "
        "    WHEN action_type = 'css-display-none' THEN "
        "      json_object("
        "        'trigger', json_object('url-filter', url_pattern),"
        "        'action', json_object('type', 'css-display-none', 'selector', selector)"
        "      )"
        "    ELSE "
        "      json_object("
        "        'trigger', json_object('url-filter', url_pattern),"
        "        'action', json_object('type', action_type)"
        "      )"
        "  END"
        ") FROM ad_filters WHERE enabled = 1;";
        * it's angry at the world blocking power ! 
        */ 
        const char *sql = 
        "SELECT json_group_array("
        "  CASE "
        "    WHEN action_type = 'css-display-none' THEN "
        "      json_object("
        "        'trigger', json_object('url-filter', url_pattern),"
        "        'action', json_object('type', 'css-display-none', 'selector', COALESCE(selector, ''))"
        "      )"
        "    ELSE "
        "      json_object("
        "        'trigger', json_object("
        "          'url-filter', url_pattern,"
        "          'resource-type', json_array('script', 'image', 'style-sheet', 'raw', 'media', 'font', 'popup', 'ping')"
        "        ),"
        "        'action', json_object('type', 'block')"
        "      )"
        "  END"
        ") FROM ad_filters WHERE enabled = 1;";
                       
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        g_printerr("[AdBlock Error] SQLite statement preparation failed: %s\n", sqlite3_errmsg(db));
        return NULL;
    }

    char *json_output = NULL;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *result = (const char*)sqlite3_column_text(stmt, 0);
        if (result && strlen(result) > 0) {
            json_output = g_strdup(result);
        }
    }

    sqlite3_finalize(stmt);
    
    /* Fallback default schema if table is empty */
    if (!json_output || strcmp(json_output, "[null]") == 0) {
        if (json_output) g_free(json_output);
        json_output = g_strdup("["
            "{\"trigger\":{\"url-filter\":\".*google-analytics\\\\.com/.*\"},\"action\":{\"type\":\"block\"}},"
            "{\"trigger\":{\"url-filter\":\".*doubleclick\\\\.net/.*\"},\"action\":{\"type\":\"block\"}},"
            "{\"trigger\":{\"url-filter\":\".*adservice\\\\.google\\\\..*\"},\"action\":{\"type\":\"block\"}}"
        "]");
    }

    return json_output;
}

void apply_ad_filters_2(WebKitWebView *web_view, sqlite3 *db) {
    if (!web_view || !db) return;

    WebKitUserContentManager *manager = webkit_web_view_get_user_content_manager(web_view);
    const char *cache_path = g_get_user_cache_dir();
    WebKitUserContentFilterStore *store = webkit_user_content_filter_store_new(cache_path);
    
    char *json_data = logic_get_filters_as_json_2(db);
    
    if (json_data) {
        GBytes *bytes = g_bytes_new_take(json_data, strlen(json_data));
        
        /* Load and compile user rules in WebKit Engine */
        webkit_user_content_filter_store_save(
            store, 
            "crystalcat-adblock-rules", 
            bytes, 
            NULL, 
            (GAsyncReadyCallback)on_filter_saved_2, 
            manager
        );
        
        g_bytes_unref(bytes);
    }
    
    g_object_unref(store);
}

/* Supplemental CSS Element Hiding Injection for Cosmetic Blocking */
void apply_css_element_blocking_2(WebKitWebView *web_view, sqlite3 *db) {
    if (!web_view) return;

    WebKitUserContentManager *manager = webkit_web_view_get_user_content_manager(web_view);
    
    /* Standard CSS stylesheet rule targeting ad containers and display slots */
    const char *ad_style = 
        ".ad, .ads, .ad-banner, .ad-wrapper, .ad-slot, .sponsored-post, "
        "iframe[src*='doubleclick.net'], iframe[src*='googlesyndication.com'], "
        "div[id^='google_ads_iframe'] { display: none !important; visibility: hidden !important; }";

    WebKitUserStyleSheet *stylesheet = webkit_user_style_sheet_new(
        ad_style,
        WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
        WEBKIT_USER_STYLE_LEVEL_USER,
        NULL,
        NULL
    );

    webkit_user_content_manager_add_style_sheet(manager, stylesheet);
    webkit_user_style_sheet_unref(stylesheet);
}

/* Helper function to defuse Anti-AD-blocking websites */
void inject_script_defusal(WebKitUserContentManager *manager) {
    if (!manager) return;

    /* Hardcoded JavaScript scriptlet payload  */ 
    const gchar *defusal_scriptlet = 
        "(function() {"
        "  'use strict';"
        "  window.canRunAds = true;"
        "  window.canShowAds = true;"
        "  window.isAdBlockActive = false;"
        "  window.adBlockDetected = false;"
        "  window.hasAdBlocker = false;"
        "  window.google_ad_client = 'ca-pub-0000000000000000';"
        "  window.google_ad_status = 1;"
        "  window.apstag = { init: function(){}, fetchBids: function(a,b){ if(b)b([]); }, setDisplayBids: function(){} };"
        "  window.pbjs = window.pbjs || { que: [], requestBids: function(o){ if(o&&o.bidsBackHandler)o.bidsBackHandler(); } };"
        "  window.googletag = window.googletag || {};"
        "  window.googletag.cmd = window.googletag.cmd || [];"
        "  window.googletag.apiReady = true;"
        "  window.googletag.display = function() {};"
        "  window.googletag.destroySlots = function() {};"
        "  window.googletag.pubads = function() {"
        "    return {"
        "      addEventListener: function() {},"
        "      clear: function() {},"
        "      enableSingleRequest: function() {},"
        "      refresh: function() {}"
        "    };"
        "  };"
        "  const originalFetch = window.fetch;"
        "  window.fetch = function(resource, init) {"
        "    if (typeof resource === 'string' && (resource.includes('ads') || resource.includes('analytics'))) {"
        "      return Promise.resolve(new Response('{}', {"
        "        status: 200,"
        "        statusText: 'OK',"
        "        headers: { 'Content-Type': 'application/json' }"
        "      }));"
        "    }"
        "    return originalFetch.apply(this, arguments);"
        "  };"
        "})();";

    /* Create user script targeting top frame at document start */
    WebKitUserScript *script = webkit_user_script_new(
        defusal_scriptlet,
        WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
        WEBKIT_USER_CONTENT_INJECT_TOP_FRAME,
        NULL,  /* Whitelist domains (NULL applies to all pages) */
        NULL   /* Blacklist domains */
    );

    /* Register script with the content manager */
    webkit_user_content_manager_add_script(manager, script);
    /* Clean up handle exit clean */
    webkit_user_script_unref(script);
}


/* Helper function to clean up database of duplicate entries*/
gboolean deduplicate_ad_filters(sqlite3 *db) {
    if (!db) {
        g_printerr("[DB Error] Database connection is NULL.\n");
        return FALSE;
    }

    char *err_msg = NULL;
    
    /* 
     * SQL Query Logic:
     * Deletes any row whose ID is not the minimum ID (first inserted) 
     * for a given url_pattern group.
     */
    const char *sql_dedup = 
        "DELETE FROM ad_filters "
        "WHERE id NOT IN ("
        "    SELECT MIN(id) "
        "    FROM ad_filters "
        "    GROUP BY url_pattern"
        ");";

    /* Execute the cleanup query */
    int rc = sqlite3_exec(db, sql_dedup, NULL, NULL, &err_msg);

    if (rc != SQLITE_OK) {
        g_printerr("DEBUG: [Error] Failed to deduplicate ad_filters: %s\n", 
                   err_msg ? err_msg : "Unknown error");
        if (err_msg) sqlite3_free(err_msg);
        return FALSE;
    }

    /* Verify how many duplicate rows were deleted */
    int rows_deleted = sqlite3_changes(db);
    g_print("Successfully removed %d duplicate filter entry(ies).\n", rows_deleted);
    
    return TRUE;
}

/*
 * Injects a defensive Script Defusal script into WebKit User Content Manager.
 * Overrides high-risk browser functions (eval, dynamic script elements, document.write)
 * and enforces a defensive Content Security Policy dynamically.
 */
void inject_www_script_defusal(WebKitUserContentManager *manager) {
    if (!manager) return;

    /* Defusal Payload: Overrides execution vectors used by malicious scripts */
    const char *defusal_js = 
        "(function() {"
        "   'use strict';"
        "   /* Neutralize dynamic evaluation vectors */"
        "   window.eval = function() { console.warn('[DEFUSAL] eval() execution blocked.'); return null; };"
        "   window.Function = function() { console.warn('[DEFUSAL] Dynamic Function creation blocked.'); return function(){}; };"
        "   document.write = function() { console.warn('[DEFUSAL] document.write blocked.'); };"
        "   document.writeln = function() { console.warn('[DEFUSAL] document.writeln blocked.'); };"
        "   "
        "   /* Prevent attacker scripts from injecting external <script> nodes */"
        "   const originalAppend = Element.prototype.appendChild;"
        "   Element.prototype.appendChild = function(node) {"
        "       if (node && node.tagName === 'SCRIPT') {"
        "           console.warn('[DEFUSAL] Dynamically created <script> tag insertion blocked:', node.src);"
        "           return null;"
        "       }"
        "       return originalAppend.apply(this, arguments);"
        "   };"
        "})();";

    WebKitUserScript *script = webkit_user_script_new(
        defusal_js,
        WEBKIT_USER_CONTENT_INJECT_TOP_FRAME,
        WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
        NULL, NULL
    );

    webkit_user_content_manager_add_script(manager, script);
    webkit_user_script_unref(script);

    /* Inject Meta Content Security Policy (CSP) to restrict hostile execution */
    const char *csp_meta_js = 
        "(function() {"
        "   var meta = document.createElement('meta');"
        "   meta.httpEquiv = 'Content-Security-Policy';"
        "   meta.content = \"default-src 'self' 'unsafe-inline'; script-src 'self'; object-src 'none';\";"
        "   if (document.head) { document.head.appendChild(meta); }"
        "   else { document.addEventListener('DOMContentLoaded', function() { document.head.appendChild(meta); }); }"
        "})();";

    WebKitUserScript *csp_script = webkit_user_script_new(
        csp_meta_js,
        WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
        WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
        NULL, NULL
    );

    webkit_user_content_manager_add_script(manager, csp_script);
    webkit_user_script_unref(csp_script);
}

/* Toggle Callback: Enable or disable script defusal & JavaScript execution on the browser view. */
static void on_toggle_script_defusal(GtkCheckMenuItem *check_item, gpointer user_data) {
    AppState *app = (AppState *)user_data;
    if (!app || !app->browser_view) return;

    g_block_hostile_scripts = gtk_check_menu_item_get_active(check_item);
    
    WebKitWebView *web_view = WEBKIT_WEB_VIEW(app->browser_view);
    WebKitSettings *settings = webkit_web_view_get_settings(web_view);
    WebKitUserContentManager *manager = webkit_web_view_get_user_content_manager(web_view);

    if (g_block_hostile_scripts) {
        /* Apply active defusal mechanisms */
        inject_www_script_defusal(manager);
        
        /* Disable dangerous features */
        webkit_settings_set_enable_javascript_markup(settings, FALSE);
        
        ui_set_status_alert(app, "Hostile Script Protection: ACTIVE (Defusal & CSP Enforced)", FALSE);
        logic_update_assistant_message(app, "🛡️ Anti-exploit script defusal active!");
    } else {
        /* Clear custom script rules and restore standard execution */
        webkit_user_content_manager_remove_all_scripts(manager);
        webkit_settings_set_enable_javascript_markup(settings, TRUE);
        
        ui_set_status_alert(app, "Hostile Script Protection: DISABLED", TRUE);
        logic_update_assistant_message(app, "⚠️ Script protection disabled. Proceed with caution.");
    }

    webkit_web_view_reload(web_view);
}

/* =========================================================================
 * OPEN LOCAL HTML FILE DIALOG & CALLBACK
 * ========================================================================= */

static void cb_on_open_local_html(GtkWidget *widget, gpointer data) {
    AppState *app = (AppState *)data;

    if (!app->browser_view) {
        ui_set_status_alert(app, "Browser component is not initialized.", TRUE);
        return;
    }

    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Open Local HTML File",
        GTK_WINDOW(app->window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL
    );

    /* Filter to display HTML files */
    GtkFileFilter *html_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(html_filter, "HTML Files (*.html, *.htm)");
    gtk_file_filter_add_pattern(html_filter, "*.html");
    gtk_file_filter_add_pattern(html_filter, "*.htm");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), html_filter);

    /* Fallback filter for all files */
    GtkFileFilter *all_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(all_filter, "All Files");
    gtk_file_filter_add_pattern(all_filter, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), all_filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        if (filename) {
            /* Convert local file path to a valid file:// URI */
            char *file_uri = g_filename_to_uri(filename, NULL, NULL);

            if (file_uri) {
                /* Load file into the WebKit browser */
                webkit_web_view_load_uri(WEBKIT_WEB_VIEW(app->browser_view), file_uri);

                /* Optional: Update search/address entry if present */
                if (app->search_entry) {
                    gtk_entry_set_text(GTK_ENTRY(app->search_entry), file_uri);
                }
                /* Add to recent files tracker */
                logic_add_recent_file(app, filename);

                char *status_msg = g_strdup_printf("Opened HTML: %s", filename);
                ui_set_status_alert(app, status_msg, FALSE);
                g_free(status_msg);
                g_free(file_uri);
            } else {
                ui_set_status_alert(app, "Failed to construct file URI.", TRUE);
            }
            g_free(filename);
        }
    }
    gtk_widget_destroy(dialog);
}

/*  Start / Stop filters AD block & Reader modes menu options section */ 

typedef struct {
    gboolean enabled;

    gboolean disable_javascript;
    gboolean disable_javascript_markup;

    gboolean hide_navigation;
    gboolean hide_header_footer;
    gboolean hide_sidebars;
    gboolean hide_ads;
    gboolean hide_social;
    gboolean hide_comments;
    gboolean hide_popups;

    gboolean block_fonts;
    gboolean block_media;
    gboolean block_raw;
    gboolean block_popups;

} ArticleIsolationSettings;

/* NORMAL BROWSING default preset */
static ArticleIsolationSettings g_article_isolation = {
    FALSE,
    FALSE,
    FALSE,
    FALSE,
    FALSE,
    FALSE,
    FALSE,
    FALSE,
    FALSE,
    FALSE,
    FALSE,
    FALSE,
    FALSE,
    FALSE
};

static WebKitUserContentFilter *g_article_isolation_filter = NULL;
 
static void on_article_isolation_filter_saved(
    WebKitUserContentFilterStore *store,
    GAsyncResult *res,
    gpointer user_data)
{
    GError *error = NULL;

    WebKitUserContentFilter *filter =
        webkit_user_content_filter_store_save_finish(
            store,
            res,
            &error
        );

    if (!filter) {
        if (error) {
            g_printerr(
                "[Article Isolation] Failed to save filter: %s\n",
                error->message
            );

            g_error_free(error);
        }

        return;
    }

    WebKitUserContentManager *manager =
        WEBKIT_USER_CONTENT_MANAGER(user_data);

    if (!manager) {
        webkit_user_content_filter_unref(filter);
        return;
    }

    /*
     * Remove the previous Article Isolation filter if one exists.
     */
    if (g_article_isolation_filter) {

        webkit_user_content_manager_remove_filter(
            manager,
            g_article_isolation_filter
        );

        webkit_user_content_filter_unref(
            g_article_isolation_filter
        );

        g_article_isolation_filter = NULL;
    }

    /*
     * Keep our own reference to the newly installed filter.
     */
    g_article_isolation_filter =
        webkit_user_content_filter_ref(filter);

    /*
     * Add the new filter to WebKit.
     */
    webkit_user_content_manager_add_filter(
        manager,
        filter
    );

    webkit_user_content_filter_unref(filter);

    g_print(
        "[Article Isolation] Filter installed successfully.\n"
    );
}
 
 
 /*Generate JSON Content Filter rules for Article + Image isolation */
static char* logic_get_article_only_filter_json(void) {
    /* Rules:
     * 1. Block resource-types: script, style-sheet, font, media, raw, popup.
     * 2. Hide common non-article layout elements (nav, header, footer, sidebars, ads).
     */
    return g_strdup(
        "["
        "  {"
        "    \"trigger\": {"
        "      \"url-filter\": \".*\""
        "    },"
        "    \"action\": {"
        "      \"type\": \"css-display-none\","
        "      \"selector\": \"nav, header, footer, sidebar, .sidebar, .ad, .ads, .social-share, #comments, aside\""
        "    }"
        "  },"
        "  {"
        "    \"trigger\": {"
        "      \"url-filter\": \".*\","
        "      \"resource-type\": [\"script\", \"font\", \"media\", \"raw\", \"popup\"]"
        "    },"
        "    \"action\": {"
        "      \"type\": \"block\""
        "    }"
        "  }"
        "]"
    );
}

/*
 * ------------------------------------------------------------
 * Restore the actual browser/application state to Normal Browsing.
 * This is the canonical "off" state for Article & Image Isolation.
 * ------------------------------------------------------------
 */
static void article_isolation_set_normal_defaults(void)
{
    g_article_isolation.enabled = FALSE;

    /*
     * These are "disable" options, so FALSE means normal operation.
     */
    g_article_isolation.disable_javascript = FALSE;
    g_article_isolation.disable_javascript_markup = FALSE;

    /*
     * No page cleanup.
     */
    g_article_isolation.hide_navigation = FALSE;
    g_article_isolation.hide_header_footer = FALSE;
    g_article_isolation.hide_sidebars = FALSE;
    g_article_isolation.hide_ads = FALSE;
    g_article_isolation.hide_social = FALSE;
    g_article_isolation.hide_comments = FALSE;
    g_article_isolation.hide_popups = FALSE;

    /*
     * No additional resource blocking.
     */
    g_article_isolation.block_fonts = FALSE;
    g_article_isolation.block_media = FALSE;
    g_article_isolation.block_raw = FALSE;
    g_article_isolation.block_popups = FALSE;
}


/*
 * ------------------------------------------------------------
 * Set all dialog switches from the currently selected preset.
 *
 * IMPORTANT:
 * This updates the UI only.
 * The actual global settings are committed when Apply is pressed.
 * ------------------------------------------------------------
 */
static void article_isolation_set_preset_controls(GtkComboBox *combo)
{
    if (!combo)
        return;

    GtkWidget *enable_check =
        GTK_WIDGET(g_object_get_data(
            G_OBJECT(combo), "isolation-enable"));

    GtkWidget *javascript_check =
        GTK_WIDGET(g_object_get_data(
            G_OBJECT(combo), "isolation-javascript"));

    GtkWidget *javascript_markup_check =
        GTK_WIDGET(g_object_get_data(
            G_OBJECT(combo), "isolation-markup"));

    GtkWidget *hide_navigation =
        GTK_WIDGET(g_object_get_data(
            G_OBJECT(combo), "isolation-navigation"));

    GtkWidget *hide_header_footer =
        GTK_WIDGET(g_object_get_data(
            G_OBJECT(combo), "isolation-header-footer"));

    GtkWidget *hide_sidebars =
        GTK_WIDGET(g_object_get_data(
            G_OBJECT(combo), "isolation-sidebars"));

    GtkWidget *hide_ads =
        GTK_WIDGET(g_object_get_data(
            G_OBJECT(combo), "isolation-ads"));

    GtkWidget *hide_social =
        GTK_WIDGET(g_object_get_data(
            G_OBJECT(combo), "isolation-social"));

    GtkWidget *hide_comments =
        GTK_WIDGET(g_object_get_data(
            G_OBJECT(combo), "isolation-comments"));

    GtkWidget *hide_popups =
        GTK_WIDGET(g_object_get_data(
            G_OBJECT(combo), "isolation-popups"));

    GtkWidget *block_fonts =
        GTK_WIDGET(g_object_get_data(
            G_OBJECT(combo), "isolation-fonts"));

    GtkWidget *block_media =
        GTK_WIDGET(g_object_get_data(
            G_OBJECT(combo), "isolation-media"));

    GtkWidget *block_raw =
        GTK_WIDGET(g_object_get_data(
            G_OBJECT(combo), "isolation-raw"));

    GtkWidget *block_resource_popups =
        GTK_WIDGET(g_object_get_data(
            G_OBJECT(combo), "isolation-resource-popups"));

    if (!enable_check ||
        !javascript_check ||
        !javascript_markup_check ||
        !hide_navigation ||
        !hide_header_footer ||
        !hide_sidebars ||
        !hide_ads ||
        !hide_social ||
        !hide_comments ||
        !hide_popups ||
        !block_fonts ||
        !block_media ||
        !block_raw ||
        !block_resource_popups) {
        return;
    }

    gint index = gtk_combo_box_get_active(combo);

    gboolean enabled = FALSE;
    gboolean disable_js = FALSE;
    gboolean disable_markup = FALSE;

    gboolean navigation = FALSE;
    gboolean header_footer = FALSE;
    gboolean sidebars = FALSE;
    gboolean ads = FALSE;
    gboolean social = FALSE;
    gboolean comments = FALSE;
    gboolean popups = FALSE;

    gboolean fonts = FALSE;
    gboolean media = FALSE;
    gboolean raw = FALSE;
    gboolean resource_popups = FALSE;


    /*
     * ------------------------------------------------------------
     * 0 = Strict Reader
     * ------------------------------------------------------------
     */
    if (index == 0) {

        enabled = TRUE;

        disable_js = TRUE;
        disable_markup = TRUE;

        navigation = TRUE;
        header_footer = TRUE;
        sidebars = TRUE;
        ads = TRUE;
        social = TRUE;
        comments = TRUE;
        popups = TRUE;

        fonts = TRUE;
        media = TRUE;
        raw = TRUE;
        resource_popups = TRUE;
    }


    /*
     * ------------------------------------------------------------
     * 1 = Balanced Reader
     *
     * Reader cleanup remains aggressive, but JavaScript and
     * normal page resources are allowed where possible.
     * ------------------------------------------------------------
     */
    else if (index == 1) {

        enabled = TRUE;

        disable_js = FALSE;
        disable_markup = FALSE;

        navigation = TRUE;
        header_footer = TRUE;
        sidebars = TRUE;
        ads = TRUE;
        social = TRUE;
        comments = TRUE;
        popups = TRUE;

        fonts = FALSE;
        media = FALSE;
        raw = TRUE;
        resource_popups = TRUE;
    }


    /*
     * ------------------------------------------------------------
     * 2 = Images + Text
     *
     * Preserve images/media while stripping the surrounding
     * page clutter.
     * ------------------------------------------------------------
     */
    else if (index == 2) {

        enabled = TRUE;

        disable_js = FALSE;
        disable_markup = FALSE;

        navigation = TRUE;
        header_footer = TRUE;
        sidebars = TRUE;
        ads = TRUE;
        social = TRUE;
        comments = TRUE;
        popups = TRUE;

        fonts = FALSE;
        media = FALSE;
        raw = FALSE;
        resource_popups = TRUE;
    }


    /*
     * ------------------------------------------------------------
     * 3 = Normal Browsing
     * ------------------------------------------------------------
     */
    else if (index == 3) {

        enabled = FALSE;

        disable_js = FALSE;
        disable_markup = FALSE;

        navigation = FALSE;
        header_footer = FALSE;
        sidebars = FALSE;
        ads = FALSE;
        social = FALSE;
        comments = FALSE;
        popups = FALSE;

        fonts = FALSE;
        media = FALSE;
        raw = FALSE;
        resource_popups = FALSE;
    } else {
        enabled = FALSE;
        disable_js = FALSE;
        disable_markup = FALSE;
        navigation = FALSE;
        header_footer = FALSE;
        sidebars = FALSE;
        ads = FALSE;
        social = FALSE;
        comments = FALSE;
        popups = FALSE;
        fonts = FALSE;
        media = FALSE;
        raw = FALSE;
        resource_popups = FALSE;
	}
	


    /*
     * Update every visible switch.
     */
    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(enable_check), enabled);

    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(javascript_check), disable_js);

    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(javascript_markup_check), disable_markup);

    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(hide_navigation), navigation);

    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(hide_header_footer), header_footer);

    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(hide_sidebars), sidebars);

    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(hide_ads), ads);

    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(hide_social), social);

    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(hide_comments), comments);

    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(hide_popups), popups);

    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(block_fonts), fonts);

    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(block_media), media);

    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(block_raw), raw);

    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(block_resource_popups),
        resource_popups);
}


/*
 * ------------------------------------------------------------
 * Reader Preset combo callback.
 * ------------------------------------------------------------
 */
static void on_reader_preset_changed(GtkComboBox *combo,
                                     gpointer user_data)
{
    AppState *app = (AppState *)user_data;

    if (!combo)
        return;

    logic_update_assistant_message(app, "Thinking about applying new browser settings."); 
                  
    article_isolation_set_preset_controls(combo);
    gint index = gtk_combo_box_get_active(combo);


    const char *message = "Reader preset selected.";

    switch (index) {
        case 0:
            message = "Strict Reader preset selected.";
            break;

        case 1:
            message = "Balanced Reader preset selected.";
            break;

        case 2:
            message = "Images + Text preset selected.";
            break;

        case 3:
            message = "Normal Browsing preset selected.";
            break;
    }

    if (app) {
		trigger_cat_animation(app);
        ui_set_status_alert(app, message, FALSE);
    }
}


void toggle_article_image_reader_mode(AppState *app, gboolean enable) {
    if (!app->browser_view || !WEBKIT_IS_WEB_VIEW(app->browser_view)) return;

    WebKitUserContentManager *manager = webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(app->browser_view));
    WebKitSettings *settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(app->browser_view));

    if (enable) {
        /* 1. Disable JavaScript execution for strict isolation */
        webkit_settings_set_enable_javascript(settings, FALSE);
        webkit_settings_set_enable_javascript_markup(settings, FALSE);

        /* 2. Load Article + Image filter store */
        const char *cache_path = g_get_user_cache_dir();
        WebKitUserContentFilterStore *store = webkit_user_content_filter_store_new(cache_path);
        
        char *json = logic_get_article_only_filter_json();
        GBytes *bytes = g_bytes_new_take(json, strlen(json));

        webkit_user_content_filter_store_save(
        store,
        "article-image-filter",
        bytes,
        NULL,
        (GAsyncReadyCallback)on_filter_saved,
        manager
    );

        g_bytes_unref(bytes);
        ui_set_status_alert(app, "Article & Image Isolation Mode Activated", FALSE);
    } else {
           /*
     * Restore JavaScript.
     */
    webkit_settings_set_enable_javascript(
        settings,
        TRUE
    );

    webkit_settings_set_enable_javascript_markup(
        settings,
        TRUE
    );

    /*
     * Remove Article Isolation filter.
     */
     
    if (g_article_isolation_filter) {

        webkit_user_content_manager_remove_filter(
            manager,
            g_article_isolation_filter
        );

        webkit_user_content_filter_unref(
            g_article_isolation_filter
        );

        g_article_isolation_filter = NULL;
    }

    /*
     * Restore normal database filters.
     */
    if (app->db) {
        apply_ad_filters(
            WEBKIT_WEB_VIEW(app->browser_view),
            app->db
        );
    }

    ui_set_status_alert(
        app,
        "Standard Browsing Restored",
        FALSE
    );
        
        /* Reapply default SQLite ad filters */
        apply_ad_filters(WEBKIT_WEB_VIEW(app->browser_view), app->db);
        ui_set_status_alert(app, "Standard Browsing Restored", FALSE);
    }

    webkit_web_view_reload(WEBKIT_WEB_VIEW(app->browser_view));
}

static void on_toggle_article_mode(GtkWidget *widget, gpointer data) {
  AppState *app = (AppState *)data;

    if (!app || !widget) {
        return;
    }

    gboolean active = FALSE;

    if (GTK_IS_CHECK_MENU_ITEM(widget)) {
        active = gtk_check_menu_item_get_active(
            GTK_CHECK_MENU_ITEM(widget)
        );
    } else if (GTK_IS_TOGGLE_BUTTON(widget)) {
        active = gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(widget)
        );
    } else {
        g_warning("Article isolation: unsupported checkbox widget type");
        return;
    }

    toggle_article_image_reader_mode(app, active);
}

/* Call this inside your GTK menu builder UI function (e.g., inside ui_create_menu) */
GtkWidget* ui_create_article_isolation_menu_item(AppState *app) {
    GtkWidget *item = gtk_check_menu_item_new_with_label("Isolate Articles & Images Only");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), FALSE);
    
    g_signal_connect(G_OBJECT(item), "toggled", 
                     G_CALLBACK(on_toggle_article_mode), app);
    
    return item;
}


/*
 * Apply the current Article & Image Isolation settings.
 *
 * This function:
 *   1. Updates WebKit JavaScript settings.
 *   2. Generates the current content-filter JSON.
 *   3. Saves the filter into WebKit's filter store.
 *   4. Reloads the browser page.
 */
static void article_isolation_apply_settings(AppState *app)
{
    if (!app ||
        !app->browser_view ||
        !WEBKIT_IS_WEB_VIEW(app->browser_view)) {
        g_warning("Article isolation: invalid browser view");
        logic_update_assistant_message(app, "Article isolation of browser view is invalid."); 
        return;
    }

    WebKitWebView *web_view =
        WEBKIT_WEB_VIEW(app->browser_view);

    WebKitSettings *settings =
        webkit_web_view_get_settings(web_view);

    WebKitUserContentManager *manager =
        webkit_web_view_get_user_content_manager(web_view);

    if (!settings || !manager) {
        g_warning("Article isolation: unable to obtain WebKit settings");
        logic_update_assistant_message(app, "Article isolation: unable to obtain WebKit settings");
        return;
    }

    /*
     * ------------------------------------------------------------
     * Normal Browsing
     * ------------------------------------------------------------
     */
 if (!g_article_isolation.enabled) {

    /*
     * --------------------------------------------------------
     * RESTORE NORMAL BROWSING
     * --------------------------------------------------------
     */

    /*
     * Re-enable JavaScript.
     */
    webkit_settings_set_enable_javascript(
        settings,
        TRUE
    );

    webkit_settings_set_enable_javascript_markup(
        settings,
        TRUE
    );

    /*
     * CRITICAL:
     *
     * Remove the Article & Image Isolation filter that was
     * previously attached to this WebView.
     */
    if (g_article_isolation_filter) {

        webkit_user_content_manager_remove_filter(
            manager,
            g_article_isolation_filter
        );

        webkit_user_content_filter_unref(
            g_article_isolation_filter
        );

        g_article_isolation_filter = NULL;

        g_print(
            "[Article Isolation] Isolation filter removed.\n"
        );
        logic_update_assistant_message(app, "Article Isolation filter removed.");
    }

    /*
     * Restore the normal SQLite-based ad filters.
     */
    if (app->db) {
        apply_ad_filters(
            web_view,
            app->db
        );
    }

    /*
     * Tell the user that the browser is back to normal.
     */
    ui_set_status_alert(
        app,
        "Standard Browsing Restored",
        FALSE
    );
    logic_update_assistant_message(app, "Standard browsing restored."); 
    /*
     * Reload only after the isolation filter has been removed.
     */
    webkit_web_view_reload(web_view);

    return;
}


    /*
     * ------------------------------------------------------------
     * JavaScript
     * ------------------------------------------------------------
     */
    webkit_settings_set_enable_javascript(
        settings,
        !g_article_isolation.disable_javascript
    );

    webkit_settings_set_enable_javascript_markup(
        settings,
        !g_article_isolation.disable_javascript_markup
    );


    /*
     * ------------------------------------------------------------
     * Generate the Article / Image content filter.
     * ------------------------------------------------------------
     */
    char *json =
        logic_get_article_only_filter_json();

    if (!json) {
        g_warning(
            "Article isolation: failed to generate filter JSON"
        );
        logic_update_assistant_message(app, "Article isolation failed to generate filter JSON hack.");
        return;
    }


    /*
     * ------------------------------------------------------------
     * Store the WebKit content filter.
     * ------------------------------------------------------------
     */
    const char *cache_path =
        g_get_user_cache_dir();

    WebKitUserContentFilterStore *filter_store =
        webkit_user_content_filter_store_new(
            cache_path
        );

    if (!filter_store) {
        g_warning(
            "Article isolation: unable to create filter store"
        );
        logic_update_assistant_message(app, "Atricle isolation unable to create filter store");
        g_free(json);
        return;
    }


    GBytes *bytes =
        g_bytes_new_take(
            json,
            strlen(json)
        );

    if (!bytes) {
        g_warning(
            "Article isolation: unable to create filter bytes"
        );
        logic_update_assistant_message(app, "Article isolation unable to create filter bytes"); 
        g_free(json);
        g_object_unref(filter_store);
        return;
    }


    /*
     * Save the generated filter.
     *
     * on_filter_saved() will add the resulting WebKit filter
     * to the current WebKitUserContentManager.
     */
    webkit_user_content_filter_store_save(
        filter_store,
        "article-image-filter",
        bytes,
        NULL,
        (GAsyncReadyCallback)on_article_isolation_filter_saved,
        manager
    );


    g_bytes_unref(bytes);
    g_object_unref(filter_store);


    /*
     * ------------------------------------------------------------
     * Status
     * ------------------------------------------------------------
     */
    ui_set_status_alert(
        app,
        "Article & Image Isolation Settings Applied",
        FALSE
    );
    logic_update_assistant_message(app, "Article & image isolation settings applied to browser.");

    /*
     * ------------------------------------------------------------
     * Reload the current page so the new settings take effect.
     * ------------------------------------------------------------
     */
    webkit_web_view_reload(web_view);
}


static void ui_show_article_isolation_dialog(GtkMenuItem *menuitem,
                                             gpointer user_data)
{
    (void)menuitem;

    AppState *app = (AppState *)user_data;

    if (!app || !app->window || !GTK_IS_WINDOW(app->window)) {
        g_warning("Article isolation dialog: invalid application window");
        logic_update_assistant_message(app, "Article isolation dialog: invalid application window");
        return;
    }

    GtkWindow *parent = GTK_WINDOW(app->window);

    /*
     * ------------------------------------------------------------
     * Dialog
     * ------------------------------------------------------------
     */
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
    "Article & Image Isolation Settings",
    parent,
    GTK_DIALOG_DESTROY_WITH_PARENT,
    "Restore Normal Browsing", 1001,
    "Reset Controls",          GTK_RESPONSE_REJECT,
    "Close",                   GTK_RESPONSE_CLOSE,
    "Apply",                   GTK_RESPONSE_APPLY,
    NULL
    );

    gtk_window_set_default_size(GTK_WINDOW(dialog), 760, 969);
    gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);

    GtkWidget *content_area =
        gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    gtk_container_set_border_width(
        GTK_CONTAINER(content_area), 12);


    /*
     * ------------------------------------------------------------
     * Main scrolling area
     * ------------------------------------------------------------
     */
    GtkWidget *scroll =
        gtk_scrolled_window_new(NULL, NULL);

    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_AUTOMATIC,
        GTK_POLICY_AUTOMATIC
    );

    gtk_box_pack_start(
        GTK_BOX(content_area),
        scroll,
        TRUE,
        TRUE,
        0
    );

    GtkWidget *main_box =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    gtk_container_set_border_width(
        GTK_CONTAINER(main_box), 4);

    gtk_container_add(
        GTK_CONTAINER(scroll),
        main_box
    );


    /*
     * ------------------------------------------------------------
     * Description
     * ------------------------------------------------------------
     */
    GtkWidget *description =
        gtk_label_new(NULL);

    gtk_label_set_markup(
        GTK_LABEL(description),
        "<b>Article &amp; Image Isolation</b>\n"
        "Create a cleaner reading view by removing common page "
        "navigation, advertisements, sidebars, social widgets, "
        "comments, and unnecessary external resources."
    );

    gtk_label_set_line_wrap(
        GTK_LABEL(description),
        TRUE
    );

    gtk_label_set_xalign(
        GTK_LABEL(description),
        0.0
    );

    gtk_box_pack_start(
        GTK_BOX(main_box),
        description,
        FALSE,
        FALSE,
        4
    );


    /*
     * ------------------------------------------------------------
     * Presets
     * ------------------------------------------------------------
     */
GtkWidget *preset_frame =
    gtk_frame_new(NULL);

gtk_frame_set_label_widget(
    GTK_FRAME(preset_frame),
    gtk_label_new(NULL)
);

GtkWidget *preset_title =
    gtk_frame_get_label_widget(GTK_FRAME(preset_frame));

gtk_label_set_markup(
    GTK_LABEL(preset_title),
    "<span foreground=\"green\"><b>Reader Presets</b></span>"
);

    gtk_box_pack_start(
        GTK_BOX(main_box),
        preset_frame,
        FALSE,
        FALSE,
        4
    );

    GtkWidget *preset_box =
        gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);

    gtk_container_set_border_width(
        GTK_CONTAINER(preset_box),
        10
    );

    gtk_container_add(
        GTK_CONTAINER(preset_frame),
        preset_box
    );

    GtkWidget *preset_combo =
        gtk_combo_box_text_new();

/*
 * Green text on black background for Reader Presets.
 */
GtkCssProvider *preset_css =
    gtk_css_provider_new();

gtk_css_provider_load_from_data(
    preset_css,
    "combobox#reader-presets {"
    "  background-color: #000000;"
    "  color: #00ff00;"
    "}"
    "combobox#reader-presets box {"
    "  background-color: #000000;"
    "}"
    "combobox#reader-presets button {"
    "  background-color: #000000;"
    "  color: #00ff00;"
    "}"
    "combobox#reader-presets entry {"
    "  background-color: #000000;"
    "  color: #00ff00;"
    "}"
    "combobox#reader-presets cellview {"
    "  background-color: #000000;"
    "  color: #00ff00;"
    "}"
    "combobox#reader-presets menu {"
    "  background-color: #000000;"
    "}"
    "combobox#reader-presets menuitem {"
    "  background-color: #000000;"
    "  color: #00ff00;"
    "}"
    "combobox#reader-presets menuitem:hover {"
    "  background-color: #003300;"
    "  color: #00ff00;"
    "}",
    -1,
    NULL
);

gtk_widget_set_name(
    preset_combo,
    "reader-presets"
);

gtk_style_context_add_provider(
    gtk_widget_get_style_context(preset_combo),
    GTK_STYLE_PROVIDER(preset_css),
    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
);

g_object_unref(preset_css);

/*
 * Force the combo box's internal text renderer
 * to use green text.
 */
GtkCellRenderer *preset_renderer = NULL;

GtkTreeModel *preset_model =
    gtk_combo_box_get_model(
        GTK_COMBO_BOX(preset_combo)
    );

if (GTK_IS_TREE_MODEL(preset_model)) {

    GList *cells =
        gtk_cell_layout_get_cells(
            GTK_CELL_LAYOUT(preset_combo)
        );

    if (cells) {
        preset_renderer =
            GTK_CELL_RENDERER(cells->data);

        g_object_set(
            G_OBJECT(preset_renderer),
            "foreground",
            "#00ff00",
            "background",
            "#000000",
            NULL
        );

        g_list_free(cells);
    }
}

    gtk_combo_box_text_append(
        GTK_COMBO_BOX_TEXT(preset_combo),
        "strict",
        "Strict Reader"
    );

    gtk_combo_box_text_append(
        GTK_COMBO_BOX_TEXT(preset_combo),
        "balanced",
        "Balanced Reader"
    );

    gtk_combo_box_text_append(
        GTK_COMBO_BOX_TEXT(preset_combo),
        "images",
        "Images + Text"
    );

    gtk_combo_box_text_append(
        GTK_COMBO_BOX_TEXT(preset_combo),
        "normal",
        "Normal Browsing"
    );

    gtk_combo_box_set_active(
        GTK_COMBO_BOX(preset_combo),
        3
    );

    g_signal_connect(
    G_OBJECT(preset_combo),
    "changed",
    G_CALLBACK(on_reader_preset_changed),
    app
    );

    gtk_box_pack_start(
        GTK_BOX(preset_box),
        gtk_label_new("Preset:"),
        FALSE,
        FALSE,
        0
    );

    gtk_box_pack_start(
        GTK_BOX(preset_box),
        preset_combo,
        TRUE,
        TRUE,
        0
    );


    /*
     * ------------------------------------------------------------
     * Isolation
     * ------------------------------------------------------------
     */
    GtkWidget *isolation_frame =
        gtk_frame_new("Isolation");

    gtk_box_pack_start(
        GTK_BOX(main_box),
        isolation_frame,
        FALSE,
        FALSE,
        4
    );

    GtkWidget *isolation_box =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    gtk_container_set_border_width(
        GTK_CONTAINER(isolation_box),
        10
    );

    gtk_container_add(
        GTK_CONTAINER(isolation_frame),
        isolation_box
    );

    GtkWidget *enable_check =
        gtk_check_button_new_with_label(
            "Enable Article & Image Isolation"
        );

    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(enable_check),
        g_article_isolation.enabled
    );

    gtk_box_pack_start(
        GTK_BOX(isolation_box),
        enable_check,
        FALSE,
        FALSE,
        0
    );

    GtkWidget *javascript_check =
        gtk_check_button_new_with_label(
            "Disable JavaScript"
        );

    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(javascript_check),
        g_article_isolation.disable_javascript
    );

    gtk_box_pack_start(
        GTK_BOX(isolation_box),
        javascript_check,
        FALSE,
        FALSE,
        0
    );

    GtkWidget *javascript_markup_check =
        gtk_check_button_new_with_label(
            "Disable JavaScript Markup"
        );

    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(javascript_markup_check),
        g_article_isolation.disable_javascript_markup
    );

    gtk_box_pack_start(
        GTK_BOX(isolation_box),
        javascript_markup_check,
        FALSE,
        FALSE,
        0
    );


    /*
     * ------------------------------------------------------------
     * Page Cleanup
     * ------------------------------------------------------------
     */
    GtkWidget *cleanup_frame =
        gtk_frame_new("Page Cleanup");

    gtk_box_pack_start(
        GTK_BOX(main_box),
        cleanup_frame,
        FALSE,
        FALSE,
        4
    );

    GtkWidget *cleanup_grid =
        gtk_grid_new();

    gtk_grid_set_row_spacing(
        GTK_GRID(cleanup_grid),
        4
    );

    gtk_grid_set_column_spacing(
        GTK_GRID(cleanup_grid),
        30
    );

    gtk_container_set_border_width(
        GTK_CONTAINER(cleanup_grid),
        10
    );

    gtk_container_add(
        GTK_CONTAINER(cleanup_frame),
        cleanup_grid
    );

    GtkWidget *hide_navigation =
        gtk_check_button_new_with_label(
            "Hide navigation"
        );

    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(hide_navigation),
        g_article_isolation.hide_navigation
    );

    gtk_grid_attach(
        GTK_GRID(cleanup_grid),
        hide_navigation,
        0, 0, 1, 1
    );


    GtkWidget *hide_header_footer =
        gtk_check_button_new_with_label(
            "Hide headers / footers"
        );

    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(hide_header_footer),
        g_article_isolation.hide_header_footer
    );

    gtk_grid_attach(
        GTK_GRID(cleanup_grid),
        hide_header_footer,
        1, 0, 1, 1
    );


    GtkWidget *hide_sidebars =
        gtk_check_button_new_with_label(
            "Hide sidebars"
        );

    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(hide_sidebars),
        g_article_isolation.hide_sidebars
    );

    gtk_grid_attach(
        GTK_GRID(cleanup_grid),
        hide_sidebars,
        0, 1, 1, 1
    );


    GtkWidget *hide_ads =
        gtk_check_button_new_with_label(
            "Hide advertisements"
        );

    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(hide_ads),
        g_article_isolation.hide_ads
    );

    gtk_grid_attach(
        GTK_GRID(cleanup_grid),
        hide_ads,
        1, 1, 1, 1
    );


    GtkWidget *hide_social =
        gtk_check_button_new_with_label(
            "Hide social sharing"
        );

    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(hide_social),
        g_article_isolation.hide_social
    );

    gtk_grid_attach(
        GTK_GRID(cleanup_grid),
        hide_social,
        0, 2, 1, 1
    );


    GtkWidget *hide_comments =
        gtk_check_button_new_with_label(
            "Hide comments"
        );

    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(hide_comments),
        g_article_isolation.hide_comments
    );

    gtk_grid_attach(
        GTK_GRID(cleanup_grid),
        hide_comments,
        1, 2, 1, 1
    );


    GtkWidget *hide_popups =
        gtk_check_button_new_with_label(
            "Hide popup elements"
        );

    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(hide_popups),
        g_article_isolation.hide_popups
    );

    gtk_grid_attach(
        GTK_GRID(cleanup_grid),
        hide_popups,
        0, 3, 1, 1
    );


    /*
     * ------------------------------------------------------------
     * Resource Control
     * ------------------------------------------------------------
     */
    GtkWidget *resource_frame =
        gtk_frame_new("Resource Control");

    gtk_box_pack_start(
        GTK_BOX(main_box),
        resource_frame,
        FALSE,
        FALSE,
        4
    );

    GtkWidget *resource_grid =
        gtk_grid_new();

    gtk_grid_set_row_spacing(
        GTK_GRID(resource_grid),
        4
    );

    gtk_grid_set_column_spacing(
        GTK_GRID(resource_grid),
        30
    );

    gtk_container_set_border_width(
        GTK_CONTAINER(resource_grid),
        10
    );

    gtk_container_add(
        GTK_CONTAINER(resource_frame),
        resource_grid
    );


    GtkWidget *block_fonts =
        gtk_check_button_new_with_label(
            "Block external fonts"
        );

    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(block_fonts),
        g_article_isolation.block_fonts
    );

    gtk_grid_attach(
        GTK_GRID(resource_grid),
        block_fonts,
        0, 0, 1, 1
    );


    GtkWidget *block_media =
        gtk_check_button_new_with_label(
            "Block audio / video media"
        );

    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(block_media),
        g_article_isolation.block_media
    );

    gtk_grid_attach(
        GTK_GRID(resource_grid),
        block_media,
        1, 0, 1, 1
    );


    GtkWidget *block_raw =
        gtk_check_button_new_with_label(
            "Block raw resources"
        );

    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(block_raw),
        g_article_isolation.block_raw
    );

    gtk_grid_attach(
        GTK_GRID(resource_grid),
        block_raw,
        0, 1, 1, 1
    );


    GtkWidget *block_resource_popups =
        gtk_check_button_new_with_label(
            "Block popup resources"
        );

    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(block_resource_popups),
        g_article_isolation.block_popups
    );

    gtk_grid_attach(
        GTK_GRID(resource_grid),
        block_resource_popups,
        1, 1, 1, 1
    );


    /*
     * ------------------------------------------------------------
     * Generated Filter Source
     * ------------------------------------------------------------
     */
    GtkWidget *source_frame =
        gtk_frame_new("Generated Isolation Filter");

    gtk_box_pack_start(
        GTK_BOX(main_box),
        source_frame,
        TRUE,
        TRUE,
        4
    );

    GtkWidget *source_box =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    gtk_container_set_border_width(
        GTK_CONTAINER(source_box),
        8
    );

    gtk_container_add(
        GTK_CONTAINER(source_frame),
        source_box
    );

    GtkWidget *source_info =
        gtk_label_new(
            "This is the WebKit content-filter JSON generated "
            "by the current isolation settings."
        );

    gtk_label_set_line_wrap(
        GTK_LABEL(source_info),
        TRUE
    );

    gtk_label_set_xalign(
        GTK_LABEL(source_info),
        0.0
    );

    gtk_box_pack_start(
        GTK_BOX(source_box),
        source_info,
        FALSE,
        FALSE,
        2
    );


    GtkWidget *source_scroll =
        gtk_scrolled_window_new(NULL, NULL);

    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(source_scroll),
        GTK_POLICY_AUTOMATIC,
        GTK_POLICY_AUTOMATIC
    );

    gtk_widget_set_size_request(
        source_scroll,
        -1,
        240
    );

    gtk_box_pack_start(
        GTK_BOX(source_box),
        source_scroll,
        TRUE,
        TRUE,
        0
    );


    GtkWidget *source_view =
        gtk_text_view_new();

    gtk_text_view_set_editable(
        GTK_TEXT_VIEW(source_view),
        FALSE
    );

    gtk_text_view_set_cursor_visible(
        GTK_TEXT_VIEW(source_view),
        FALSE
    );

    gtk_text_view_set_wrap_mode(
        GTK_TEXT_VIEW(source_view),
        GTK_WRAP_NONE
    );

    GtkTextBuffer *source_buffer =
        gtk_text_view_get_buffer(
            GTK_TEXT_VIEW(source_view)
        );

char *filter_source =
    logic_get_article_only_filter_json();

if (filter_source) {

    /*
     * Pretty-print the JSON for display only.
     *
     * The actual WebKit filter JSON is not changed.
     * This simply makes the source easier to read in the
     * Article & Image Isolation Settings control panel.
     */
    GString *pretty =
        g_string_new(NULL);

    gint indent = 0;
    gboolean in_string = FALSE;
    gboolean escaped = FALSE;

    for (const char *p = filter_source; *p; p++) {

        char c = *p;

        /*
         * Handle JSON strings carefully so that commas,
         * braces, etc. inside selectors are not treated
         * as JSON structure.
         */
        if (c == '"' && !escaped) {
            in_string = !in_string;

            g_string_append_c(
                pretty,
                c
            );

            continue;
        }

        if (in_string) {

            g_string_append_c(
                pretty,
                c
            );

            if (c == '\\' && !escaped)
                escaped = TRUE;
            else
                escaped = FALSE;

            continue;
        }

        /*
         * Opening object / array.
         */
        if (c == '{' || c == '[') {

            g_string_append_c(
                pretty,
                c
            );

            g_string_append_c(
                pretty,
                '\n'
            );

            indent++;

            for (gint i = 0; i < indent * 2; i++)
                g_string_append_c(
                    pretty,
                    ' '
                );

            continue;
        }

        /*
         * Closing object / array.
         */
        if (c == '}' || c == ']') {

            g_string_append_c(
                pretty,
                '\n'
            );

            indent--;

            if (indent < 0)
                indent = 0;

            for (gint i = 0; i < indent * 2; i++)
                g_string_append_c(
                    pretty,
                    ' '
                );

            g_string_append_c(
                pretty,
                c
            );

            continue;
        }

        /*
         * Commas separate JSON members/items.
         */
        if (c == ',') {

            g_string_append_c(
                pretty,
                c
            );

            g_string_append_c(
                pretty,
                '\n'
            );

            for (gint i = 0; i < indent * 2; i++)
                g_string_append_c(
                    pretty,
                    ' '
                );

            continue;
        }

        /*
         * Colon separates JSON keys from values.
         */
        if (c == ':') {

            g_string_append_c(
                pretty,
                c
            );

            g_string_append_c(
                pretty,
                ' '
            );

            continue;
        }

        /*
         * Ignore whitespace from the original compact JSON.
         * Whitespace inside strings was already handled above.
         */
        if (g_ascii_isspace(c))
            continue;

        g_string_append_c(
            pretty,
            c
        );
    }

    gtk_text_buffer_set_text(
        source_buffer,
        pretty->str,
        -1
    );

    g_string_free(
        pretty,
        TRUE
    );

    g_free(filter_source);
}
    gtk_container_add(
        GTK_CONTAINER(source_scroll),
        source_view
    );


    /*
     * ------------------------------------------------------------
     * Preset handling
     *
     * The controls are intentionally updated here rather than
     * immediately applying the preset. The user can select a
     * preset, inspect the options, then press Apply.
     * ------------------------------------------------------------
     */
    g_object_set_data(
        G_OBJECT(preset_combo),
        "isolation-enable",
        enable_check
    );

    g_object_set_data(
        G_OBJECT(preset_combo),
        "isolation-javascript",
        javascript_check
    );

    g_object_set_data(
        G_OBJECT(preset_combo),
        "isolation-markup",
        javascript_markup_check
    );

    g_object_set_data(
        G_OBJECT(preset_combo),
        "isolation-navigation",
        hide_navigation
    );

    g_object_set_data(
        G_OBJECT(preset_combo),
        "isolation-header-footer",
        hide_header_footer
    );

    g_object_set_data(
        G_OBJECT(preset_combo),
        "isolation-sidebars",
        hide_sidebars
    );

    g_object_set_data(
        G_OBJECT(preset_combo),
        "isolation-ads",
        hide_ads
    );

    g_object_set_data(
        G_OBJECT(preset_combo),
        "isolation-social",
        hide_social
    );

    g_object_set_data(
        G_OBJECT(preset_combo),
        "isolation-comments",
        hide_comments
    );

    g_object_set_data(
        G_OBJECT(preset_combo),
        "isolation-popups",
        hide_popups
    );

    g_object_set_data(
        G_OBJECT(preset_combo),
        "isolation-fonts",
        block_fonts
    );

    g_object_set_data(
        G_OBJECT(preset_combo),
        "isolation-media",
        block_media
    );

    g_object_set_data(
        G_OBJECT(preset_combo),
        "isolation-raw",
        block_raw
    );

    g_object_set_data(
        G_OBJECT(preset_combo),
        "isolation-resource-popups",
        block_resource_popups
    );


    /*
     * ------------------------------------------------------------
     * Apply / Reset
     * ------------------------------------------------------------
     */
    gint response;
    gtk_widget_show_all(dialog);
    
    do {
        response = gtk_dialog_run(GTK_DIALOG(dialog));
if (response == 1001) {

    /*
     * --------------------------------------------------------
     * RESTORE NORMAL BROWSING
     *
     * This is the real browser reset.
     * --------------------------------------------------------
     */

    article_isolation_set_normal_defaults();

    /*
     * Update every dialog switch as well.
     */
    gtk_combo_box_set_active(
        GTK_COMBO_BOX(preset_combo),
        3
    );

    /*
     * Apply the normal-browsing state immediately.
     */
    article_isolation_apply_settings(app);

    /*
     * Make the dialog's filter-source display reflect the
     * restored state.
     */
    gtk_text_buffer_set_text(
        source_buffer,
        "Normal Browsing\n\n"
        "Article & Image Isolation is disabled.\n"
        "JavaScript is enabled.\n"
        "No additional article-isolation resource blocking is active.\n",
        -1
    );

    /*
     * Give the user explicit confirmation.
     */
    ui_set_status_alert(
        app,
        "Normal Browsing Restored",
        FALSE
    );

    logic_update_assistant_message(
        app,
        "Normal browsing restored — Article & Image Isolation is OFF."
    );

    /*
     * Keep the dialog open so the user can immediately choose
     * another reader preset if desired.
     */
}
      else  if (response == GTK_RESPONSE_REJECT) {

            /*
             * Reset all controls
             
            gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(enable_check), FALSE);

            gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(javascript_check), FALSE);

            gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(javascript_markup_check), FALSE);

            gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(hide_navigation), FALSE);

            gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(hide_header_footer), FALSE);

            gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(hide_sidebars), FALSE);

            gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(hide_ads), FALSE);

            gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(hide_social), FALSE);

            gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(hide_comments), FALSE);

            gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(hide_popups), FALSE);

            gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(block_fonts), FALSE);

            gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(block_media), FALSE);

            gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(block_raw), FALSE);

            gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(block_resource_popups), FALSE);
     * 
     * Reset the dialog controls to Normal Browsing.
     *
     * This intentionally does NOT apply the settings to the
     * browser. The user can still change them before Apply.
*/ 
            gtk_combo_box_set_active(
                GTK_COMBO_BOX(preset_combo), 3
            );
        }
        else if (response == GTK_RESPONSE_APPLY) {

            /*
             * Copy the dialog state into the application settings.
             */
            g_article_isolation.enabled =
                gtk_toggle_button_get_active(
                    GTK_TOGGLE_BUTTON(enable_check));

            g_article_isolation.disable_javascript =
                gtk_toggle_button_get_active(
                    GTK_TOGGLE_BUTTON(javascript_check));

            g_article_isolation.disable_javascript_markup =
                gtk_toggle_button_get_active(
                    GTK_TOGGLE_BUTTON(javascript_markup_check));

            g_article_isolation.hide_navigation =
                gtk_toggle_button_get_active(
                    GTK_TOGGLE_BUTTON(hide_navigation));

            g_article_isolation.hide_header_footer =
                gtk_toggle_button_get_active(
                    GTK_TOGGLE_BUTTON(hide_header_footer));

            g_article_isolation.hide_sidebars =
                gtk_toggle_button_get_active(
                    GTK_TOGGLE_BUTTON(hide_sidebars));

            g_article_isolation.hide_ads =
                gtk_toggle_button_get_active(
                    GTK_TOGGLE_BUTTON(hide_ads));

            g_article_isolation.hide_social =
                gtk_toggle_button_get_active(
                    GTK_TOGGLE_BUTTON(hide_social));

            g_article_isolation.hide_comments =
                gtk_toggle_button_get_active(
                    GTK_TOGGLE_BUTTON(hide_comments));

            g_article_isolation.hide_popups =
                gtk_toggle_button_get_active(
                    GTK_TOGGLE_BUTTON(hide_popups));

            g_article_isolation.block_fonts =
                gtk_toggle_button_get_active(
                    GTK_TOGGLE_BUTTON(block_fonts));

            g_article_isolation.block_media =
                gtk_toggle_button_get_active(
                    GTK_TOGGLE_BUTTON(block_media));

            g_article_isolation.block_raw =
                gtk_toggle_button_get_active(
                    GTK_TOGGLE_BUTTON(block_raw));

            g_article_isolation.block_popups =
                gtk_toggle_button_get_active(
                    GTK_TOGGLE_BUTTON(block_resource_popups));


            /*
             * Apply the complete configuration.
             */
            article_isolation_apply_settings(app);

            /*
             * Regenerate the source display so the user can
             * immediately see the resulting WebKit filter.
             */
            char *new_source =
                logic_get_article_only_filter_json();

            if (new_source) {
                gtk_text_buffer_set_text(
                    source_buffer,
                    new_source,
                    -1
                );

                g_free(new_source);
            }
        }

    } while (response == GTK_RESPONSE_REJECT);

    logic_update_dialog_closed_message(app); 
    gtk_widget_destroy(dialog);
}

/* Display a random assistant message when a dialog closes. */
static void logic_update_dialog_closed_message(AppState *app)
{
    if (!app)
        return;

    static const char *closed_messages[] = {
		" Dialog closed.", 
        " Dialog closed. Back to the cat's business!",
        " All done! What's our next adventure?",
        " The rabbit says thanks for stopping by!",
        " Another window down. Systems nominal.",
        " Okay, we're finished here. Carry on!",
        " Changes reviewed. Ready for the next task!",
        " One less window in the universe.",
        " The cat has returned to its workstation.",
        " Dialog dismissed. Awaiting your next command.",
        " That went smoothly. Has anyone seen my carrot?",
        " Back to browsing. The web awaits!",
        " Window closed. Nothing escapes the cat's attention!",
        " Dialog dismissed.", 
        " Awaiting your next command.",
        " Window closed. Nothing escapes the cat's attention!", 
        " The cat is back on patrol across the web.", 
        " One small step for a cat, one giant leap for browsing!", 
        " Purr-fect! That dialog is out of the way.", 
        " Paw-sitively finished. What's next?", 
        " The cat says, 'Finally, some room on the screen!'", 
        " Black cat mode: silently returning to business.", 
        " The whiskers are twitching. Another task awaits!", 
        " A little less dialog, a little more browsing!", 
        " The rabbit says thanks for stopping by!", 
        " Dialog dismissed. The rabbit has reclaimed its carrot.", 
        " Hop along! There's more web to explore.", 
        " Another window hopped away!", 
        " The rabbit approves of this decision.", 
        " All clear! Back down the rabbit hole we go.", 
        " Quick as a bunny, that dialog is gone!", 
        " The rabbit is standing by for another adventure.", 
        " All done! What's our next adventure?", 
        " Another window down. Systems nominal.", 
        " Okay, we're finished here. Carry on!", 
        " One less window in the universe.", 
        " Dialog dismissed. Awaiting your next command.", 
        " Back to browsing. The web awaits!", 
        " Nicely done! Ready when you are.", 
        " Task wrapped up. What's on the agenda?", 
        " Dialog closed. Where shall we explore next?", 
        " That was productive. Time for the next task!", 
        " All finished here. I'm listening!", 
        " Another little victory for the browser!", 
        " Smooth sailing from here. What's next?", 
        " One less distraction, one more possibility.", 
        " And just like that, the dialog disappears!", 
        " Assistant standing by. Awaiting instructions.", 
        " Back to the so called open web maybe?", 
        " Navigation restored. Adventure awaits!", 
        " Dialog closed. Curiosity may continue.", 
        " Back to collecting knowledge, one page at a time.", 
        " The web is calling. Let's get back to it!", 
        " Browser systems are ready for your next destination.", 
        " Another stop on our journey through the internet.", 
        " Closing dialogs, opening possibilities.", 
        " Window dismissed. The browsing view is clear.", 
        " Connection steady. Standing by for the next request.", 
        " The browser is ready to turn another page.", 
        " Dialog closed. The digital map is yours again.", 
        " Changes reviewed. Ready for the next task!", 
        " Dialog teardown complete. All systems go!", 
        " One less UI obstacle. Back to development!", 
        " Dialog closed. Your workflow continues.", 
        " Another piece of the interface puzzle handled.", 
        " Interface cleared. Awaiting the next operation.", 
        " Dialog dismissed successfully.", 
        " Review complete. Standing by.", 
        " UI cleanup complete. Ready for action!", 
        " Status: idle, alert, and ready.", 
        " Processing complete. What's the next challenge?", 
        " Another little component of the day taken care of.", 
        " Interface standing by. Safety checks remain important!", 
        " Tools are ready whenever you are.", 
        " Dialog closed. Back to the main workflow.", 
        " No more dialog here. Let's keep building!", 
        " Test window dismissed. On to the next experiment!", 
        " The gears keep turning behind the scenes.", 
        " No loose screws detected. Carry on!", 
        " UI is quiet. The assistant is listening.", 
        " That went smoothly. Has anyone seen my carrot?", 
        "The dialog has left the building. Quack!", 
        " Abducting this window... mission accomplished!", 
        " The disappearing-window trick worked again!", 
        " Dialog closed. You have earned an imaginary cookie.", 
        " Wise choice. The owl approves.", 
        " No rush. The next task isn't going anywhere.", 
        " Clever move! What shall we do next?", 
        " The raccoon has inspected the dialog and approved its removal.", 
        " That dialog has floated off into the digital sky.", 
        " The dialog has been banished to the realm of closed windows.", 
        " Even dinosaurs know when to close a dialog.", 
        " Dialog dismissed. Pizza break optional.", 
        " Refreshed, recharged, and ready for another task!", 
        " Swept that dialog right off the screen.", 
        " Level cleared! Ready for the next challenge?", 
        " No magic required. Just a successful close!", 
        " A rare sighting: a dialog that knows when to leave!", 
        " And the dialog quietly exits stage left.", 
        " Another dialog has been returned to the laundry pile.", 
        " Done and dusted!", " All set!", " Nice and tidy!", " Ready for more!", 
        " Standing by!", " What's next?", " At your service!", 
        " Back to browsing or editing code!", " Ready to build!", " Listening...", 
        " Mission complete!", " Paws ready. Let's go!"
    };
    srand(time(NULL)); 
    const size_t count =
        sizeof(closed_messages) / sizeof(closed_messages[0]);

    const char *message = closed_messages[rand() % count];

    logic_update_assistant_message(app, message);
}

/* Browser console log book idea */
static void ui_show_webkit_console_dialog(
    GtkMenuItem *menuitem,
    gpointer user_data)
{
    (void)menuitem;

    AppState *app = (AppState *)user_data;

    if (!app || !app->window || !app->webkit_log_buffer)
        return;

    /* Reuse the existing dialog if it is already open. */
    if (app->webkit_log_dialog &&
        GTK_IS_WIDGET(app->webkit_log_dialog)) {
        gtk_window_present(
            GTK_WINDOW(app->webkit_log_dialog));
        return;
    }

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "WebKit JavaScript Console Logs",
        GTK_WINDOW(app->window),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Close", GTK_RESPONSE_CLOSE,
        NULL);

    app->webkit_log_dialog = dialog;
    gtk_window_set_default_size(GTK_WINDOW(dialog), 850, 500);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content), 10);
    gtk_box_set_spacing(GTK_BOX(content), 8);
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    GtkWidget *view = gtk_text_view_new_with_buffer(app->webkit_log_buffer);

    gtk_text_view_set_editable(GTK_TEXT_VIEW(view), TRUE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(view), TRUE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(view), TRUE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_NONE);

    gtk_container_add(GTK_CONTAINER(scroll), view);
    gtk_box_pack_start(GTK_BOX(content), scroll, TRUE, TRUE, 0);
    GtkWidget *remove_button = gtk_button_new_with_label("Remove Selected Text");

    GtkWidget *clear_button =
        gtk_button_new_with_label(
            "Clear All Logs");

    GtkWidget *buttons = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(buttons), GTK_BUTTONBOX_START);

    gtk_container_add(GTK_CONTAINER(buttons), remove_button);
    gtk_container_add(GTK_CONTAINER(buttons), clear_button);

    gtk_box_pack_start(
        GTK_BOX(content), buttons,
        FALSE, FALSE, 0);

    g_signal_connect(
        remove_button, "clicked",
        G_CALLBACK(cb_webkit_console_remove_selected),
        app);

    g_signal_connect(
        clear_button, "clicked",
        G_CALLBACK(cb_webkit_console_clear),
        app);

    g_signal_connect(
        dialog, "response",
        G_CALLBACK(cb_webkit_console_dialog_response),
        app);

    g_signal_connect(
        dialog, "destroy",
        G_CALLBACK(cb_webkit_console_dialog_destroy),
        app);

    gtk_widget_show_all(dialog);
}


    
    /* Receive JavaScript console messages from WebKit. */
static void
cb_webkit_console_message(WebKitUserContentManager *manager,
                          WebKitJavascriptResult *result,
                          gpointer user_data)
{
    AppState *app = (AppState *)user_data;
    JSCValue *value;
    gchar *message;
    GtkTextIter end;

    (void)manager;

    if (app == NULL || app->webkit_log_buffer == NULL || result == NULL)
        return;

    value = webkit_javascript_result_get_js_value(result);
    if (value == NULL)
        return;

    message = jsc_value_to_string(value);
    if (message == NULL)
        return;

    gtk_text_buffer_get_end_iter(app->webkit_log_buffer, &end);
    gtk_text_buffer_insert(app->webkit_log_buffer, &end,
                           message, -1);
    gtk_text_buffer_insert(app->webkit_log_buffer, &end,
                           "\n", 1);

    g_free(message);
}


/* Clear all captured console messages. */
static void
cb_webkit_console_clear(GtkButton *button, gpointer user_data)
{
    AppState *app = (AppState *)user_data;

    (void)button;

    if (app == NULL || app->webkit_log_buffer == NULL)
        return;

    gtk_text_buffer_set_text(app->webkit_log_buffer, "", -1);
}


/* Remove the selected text from the console log. */
static void
cb_webkit_console_remove_selected(GtkButton *button,
                                  gpointer user_data)
{
    AppState *app = (AppState *)user_data;
    GtkTextIter start;
    GtkTextIter end;

    (void)button;

    if (app == NULL || app->webkit_log_buffer == NULL)
        return;

    if (gtk_text_buffer_get_selection_bounds(
            app->webkit_log_buffer, &start, &end))
    {
        gtk_text_buffer_delete(app->webkit_log_buffer,
                               &start, &end);
    }
}


/* Keep the dialog pointer valid when the window is destroyed. */
static void
cb_webkit_console_dialog_destroy(GtkWidget *widget,
                                 gpointer user_data)
{
    AppState *app = (AppState *)user_data;

    if (app != NULL && app->webkit_log_dialog == widget)
        app->webkit_log_dialog = NULL;
}


/* Close the console dialog when its response requests closure. */
static void
cb_webkit_console_dialog_response(GtkDialog *dialog,
                                  gint response_id,
                                  gpointer user_data)
{
    (void)response_id;
    (void)user_data;

    gtk_widget_destroy(GTK_WIDGET(dialog));
}

/* Menu Bar time label callback function */
static gboolean cb_update_menu_bar_time(gpointer data) {
    AppState *app = (AppState *)data;
    if (!app || !app->menu_time_label || !GTK_IS_WIDGET(app->menu_time_label)) {
        return G_SOURCE_REMOVE;
    }
    
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
    "label { color: limegreen; }", -1, NULL);
    GtkStyleContext *context = gtk_widget_get_style_context(app->menu_time_label);
    gtk_style_context_add_provider(context,
    GTK_STYLE_PROVIDER(provider),
    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    char time_buffer[64];
    strftime(time_buffer, sizeof(time_buffer), "%I:%M:%S / %H:%M:%S %P %A %D", timeinfo);
    gtk_label_set_text(GTK_LABEL(app->menu_time_label), time_buffer);
    return G_SOURCE_CONTINUE; 
}     

static void cb_on_toggle_menu_time(GtkCheckMenuItem *check_item, gpointer user_data) {
    AppState *app = (AppState *)user_data;
    if (!app || !app->menu_time_label) return;

    gboolean show_time = gtk_check_menu_item_get_active(check_item);
    
    if (show_time) {
        gtk_widget_show(app->menu_time_label);
        logic_update_assistant_message(app, "The Time/Date is active.");
    } else {
        gtk_widget_hide(app->menu_time_label);
        logic_update_assistant_message(app, "The Time/Date is now hidden."); 
    }
}

static char *get_short_url(const char *uri) {
    if (!uri) return g_strdup("");

    const char *p = uri;
    if (strncmp(p, "https://", 8) == 0) p += 8;
    else if (strncmp(p, "http://", 7) == 0) p += 7;

    if (strncmp(p, "www.", 4) == 0) p += 4;

    size_t len = strlen(p);
    /* Truncate if longer than 45 characters to keep the status bar tidy */
    if (len > 45) {
        return g_strdup_printf("%.42s", p);
    }
    return g_strdup(p);
}

/* Have the Assistant display the URL site we are surfing on */
static void on_browser_uri_notify(WebKitWebView *web_view, GParamSpec *pspec, gpointer user_data) {
    AppState *app = (AppState *)user_data;
    if (!app) return;

    /* Guard: Ignore signals if the assistant UI hasn't been built yet */
    if (!app->assistant_label || !GTK_IS_LABEL(app->assistant_label)) {
        return;
    }    
    const char *uri = webkit_web_view_get_uri(web_view);
    if (uri && strlen(uri) > 0) {
        char *short_url = get_short_url(uri);
        char *msg = g_strdup_printf("Thinking .oO(%s)", short_url);
        logic_update_assistant_message(app, msg);
        g_free(msg);
        g_free(short_url);
    }
}

/*
 *  Article information Search by dialog only idea  
 * 
 * */

/* Inject Crystalcat dark mode CSS stylesheet */ 
static void apply_dark_mode_stylesheet(WebKitWebView *web_view) {
    WebKitUserContentManager *manager = webkit_web_view_get_user_content_manager(web_view);
    
    const gchar *dark_css = 
        "body {"
        "   background-color: #121212 !important;"
        "   color: #e0e0e0 !important;"
        "   font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif !important;"
        "   padding: 40px !important;"
        "   line-height: 1.6 !important;"
        "   max-width: 700px !important;"
        "   margin: 0 auto !important;"
        "}"
        "a {"
        "   color: #bb86fc !important;"
        "}"
        "pre, code {"
        "   background-color: #1e1e1e !important;"
        "   color: #03dac6 !important;"
        "   padding: 2px 4px !important;"
        "   border-radius: 4px !important;"
        "}"
        "img {"
        "   max-width: 100% !important;"
        "   height: auto !important;"
        "   filter: brightness(0.9) !important;"
        "}";

    WebKitUserStyleSheet *style_sheet = webkit_user_style_sheet_new(
        dark_css,
        WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
        WEBKIT_USER_STYLE_LEVEL_USER,
        NULL, NULL
    );

    webkit_user_content_manager_add_style_sheet(manager, style_sheet);
    webkit_user_style_sheet_unref(style_sheet);
}

/* JavaScript injection to isolate article content and strip site chrome */ 
static void on_article_load_changed(WebKitWebView *web_view, WebKitLoadEvent load_event, gpointer user_data) {
    if (load_event == WEBKIT_LOAD_FINISHED) {
        const gchar *js_cleaner = 
            "(function() {"
            "   let article = document.querySelector('article') || document.querySelector('[role=\"main\"]') || document.querySelector('main');"
            "   if (article) {"
            "       document.body.innerHTML = '';"
            "       document.body.appendChild(article);"
            "   }"
            "})();";

     webkit_web_view_evaluate_javascript(web_view, js_cleaner, (gssize)-1, NULL, NULL, NULL, NULL, NULL);  
    }

}

/* Callback when search button is clicked or Enter is pressed in entry */
static void on_search_button_clicked(GtkButton *button, gpointer user_data) {
    ArticleSearchData *data = (ArticleSearchData *)user_data;
    const gchar *query = gtk_entry_get_text(GTK_ENTRY(data->entry));
    
    if (query && *query != '\0') {
        gchar *escaped_query = g_uri_escape_string(query, NULL, TRUE);
        gchar *url = g_strdup_printf("https://html.duckduckgo.com/html/?q=%s", escaped_query);       
        webkit_web_view_load_uri(WEBKIT_WEB_VIEW(data->web_view), url);        
        g_free(escaped_query);
        g_free(url);
    }
}

static void on_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data) {
    if (response_id == GTK_RESPONSE_CLOSE || response_id == GTK_RESPONSE_DELETE_EVENT) {
        gtk_widget_destroy(GTK_WIDGET(dialog));
    }
}

void ui_show_article_search_dialog(GtkWindow *parent, WebKitWebView *parent_web_view) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Advanced Article Search",
        (parent && GTK_IS_WINDOW(parent)) ? GTK_WINDOW(parent) : NULL,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Close", GTK_RESPONSE_CLOSE,
        NULL
    );
    if (!dialog) return; /* Safety guard if creation fails */ 
    
    gtk_window_set_default_size(GTK_WINDOW(dialog), 900, 700);

    /* Apply a dark theme stylesheet to the dialog window/widgets */
    GtkCssProvider *dark_dialog_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(dark_dialog_provider,
        "dialog, window {"
        "  background-color: #1e1e1e;"
        "  color: #d4d4d4;"
        "}"
        "entry {"
        "  background-color: #2d2d2d;"
        "  color: #ffffff;"
        "  border-color: #3f3f3f;"
        "}"
        "button {"
        "  background-color: #333333;"
        "  color: #000000;"
        "  border-color: #4f4f4f;"
        "}"
        "button:hover {"
        "  background-color: #3f3f3f;"
        "}"
        "a:hover {"
        "  background-color: #ffff00;"
        "  color: #000000;"
        "}"
        "article:hover, .article:hover, .search-result:hover, .result-item:hover {"
        "  background-color: #3a3a3a;"
        "  color: #000000;"
        "}"
        "label {"
        "  color: #d4d4d4;"
        "}",
        -1, NULL
    );
    gtk_style_context_add_provider(
        gtk_widget_get_style_context(dialog),
        GTK_STYLE_PROVIDER(dark_dialog_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    g_object_unref(dark_dialog_provider);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content_area), 10);
    
    ArticleSearchData *data = g_new0(ArticleSearchData, 1);
    data->dialog = dialog;

    /* Informational helping text label */
    GtkWidget *info_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(info_label),
        "<span foreground='#00ff00'><big><b>[Advanced Article/keyword Search]</b></big></span>\n"
        "Enter your keywords, topics, or precise query phrases below to search for relevant articles."
    );
    gtk_label_set_line_wrap(GTK_LABEL(info_label), TRUE);
    gtk_widget_set_halign(info_label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(info_label, 8);
    gtk_widget_set_margin_bottom(info_label, 8);
    gtk_box_pack_start(GTK_BOX(content_area), info_label, FALSE, FALSE, 0);

    /* Toolbar box */
    GtkWidget *toolbar_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(toolbar_box), 4);
    gtk_box_pack_start(GTK_BOX(content_area), toolbar_box, FALSE, FALSE, 0);

    /* Search entry */
    data->entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(data->entry), "Enter search query or article topic...");
    gtk_widget_set_hexpand(data->entry, TRUE);
    gtk_box_pack_start(GTK_BOX(toolbar_box), data->entry, TRUE, TRUE, 0);

    /* Search button */
    GtkWidget *search_btn = gtk_button_new_with_label("Search");
    gtk_box_pack_start(GTK_BOX(toolbar_box), search_btn, FALSE, FALSE, 0);

    /* Scrolled window & WebView */
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_widget_set_hexpand(scrolled, TRUE);
    
    /* Prevent negative space allocation bug warnings during full-screen/resizing passes */
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrolled), 350);
    
    data->results_view = scrolled;
    
    data->web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    apply_dark_mode_stylesheet(WEBKIT_WEB_VIEW(data->web_view));
    
    /* Inject custom WebKit CSS for hover effects (Articles: dark gray bg + green text, Links: yellow highlight) */
    WebKitUserContentManager *content_manager = webkit_web_view_get_user_content_manager(data->web_view);
    WebKitUserStyleSheet *hover_style_sheet = webkit_user_style_sheet_new(
        "body, html, #content_internal, .main-layout, .results--main, div[data-testid='main-layout'] {"
        "  background-color: #000000 !important;"
        "  color: #d4d4d4 !important;"
        "}"
        "a {"
        "  color: #00ff00 !important;"
        "}"
        "a:hover {"
        "  background-color: #ffff00 !important;"
        "  color: #000000 !important;"
        "}"
        "article:hover, .article:hover, .search-result:hover, .result-item:hover, .result:hover, div[data-testid='result']:hover {"
        "  background-color: #3a3a3a !important;"
        "  color: #00ff00 !important;"
        "}"
        ".result__snippet, .result__title, .result__url, span[data-testid='result-snippet'] {"
        "  color: #b0b0b0 !important;"
        "  background-color: #000000 !important;"
        "}"
        ".badge:hover, .tag:hover, .chip:hover, .pill:hover, "
        "span[class*='tag']:hover, div[class*='tag']:hover, "
        "span[class*='badge']:hover, div[class*='badge']:hover {"
        "  background-color: #2d2d2d !important;"
        "  color: #00ff00 !important;"
        "  border-color: #4f4f4f !important;"
        "}",
        WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
        WEBKIT_USER_STYLE_LEVEL_USER,
        NULL, NULL
    );
    webkit_user_content_manager_add_style_sheet(content_manager, hover_style_sheet);
    webkit_user_style_sheet_unref(hover_style_sheet);

    gtk_container_add(GTK_CONTAINER(scrolled), GTK_WIDGET(data->web_view));
    gtk_box_pack_start(GTK_BOX(content_area), scrolled, TRUE, TRUE, 0);

    g_signal_connect(data->web_view, "load-changed", G_CALLBACK(on_article_load_changed), NULL);
    g_signal_connect(search_btn, "clicked", G_CALLBACK(on_search_button_clicked), data);
    g_signal_connect(data->entry, "activate", G_CALLBACK(on_search_button_clicked), data);
    g_signal_connect(dialog, "response", G_CALLBACK(on_dialog_response), NULL);
    g_signal_connect_swapped(dialog, "destroy", G_CALLBACK(g_free), data);

    gtk_widget_show_all(dialog);
}

static void on_tools_article_search_activated(GtkMenuItem *menuitem, gpointer user_data) {
AppState *app = (AppState *)user_data;
   if (!app) {
        g_warning("Article search: app state pointer is NULL.");
        return;
    }    
    if (!app->window) {
        g_warning("Article search failed: app->window is NULL.");
        return;
    }
    if (!GTK_IS_WINDOW(app->window)) {
        g_warning("Article search failed: app->window is not a valid GtkWindow.");
        return;
    }    
    if (!app->browser_view) {
        g_warning("Article search failed: app->browser_view is NULL.");
        return;
    }
    if (!WEBKIT_IS_WEB_VIEW(app->browser_view)) {
        g_warning("Article search failed: app->browser_view is not a valid WebKitWebView.");
        return;
   } 
   ui_show_article_search_dialog(GTK_WINDOW(app->window), WEBKIT_WEB_VIEW(app->browser_view));
}

static void cb_on_destroy(GtkWidget *widget, gpointer data G_GNUC_UNUSED) { gtk_main_quit(); }
